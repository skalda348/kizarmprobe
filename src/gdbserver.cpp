#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "utils.h"
#include "gdbpacket.h"
#include "gdbserver.h"
#include "target.h"
#include "monitor.h"
#include "stm32f1.h"

GdbServer::GdbServer() : BaseLayer(), mon ("Basic"), lock(),
  led (LEDPIN) {
  single_step   = false;
  last_activity = 0;
  target        = 0;
  active        = false;
  mon.setServer (this);
  led.set       ();
}
void GdbServer::Fini (void) {
  if (target) delete target;
  lock.Fini ();
}
void GdbServer::OldTargetDestroy (void) {
  if (!target) return;
  if ( target->attached) target->detach();
  target->remove();
  delete target;
  target = 0;
}
void GdbServer::SetActive (void) {
  active = true;
  lock.Reset ();
}
/*
GdbServer::~GdbServer() {
  if (target) delete target;
}
*/
bool GdbServer::probe (Target *n) {
  if (n->probe()) {
    target = n;
    mon.cScan.reply ("Target: %s\n", target->getName());
    return true;
  }
  n->remove();
  delete n;
  return false;
}
/* Presunuto do main.cpp - muze se menit.
void GdbServer::Scan (void) {
  if (probe (new STM32F1  (this, "STM32FXX")))      return;
  if (probe (new CortexMx (this, "ARM Cortex-Mx"))) return;
}
*/
bool GdbServer::target_check (void) {
  if (!target) {
    gdb_putpacketz ("EFF");
    return false;
  }
  return true;
}

void GdbServer::Polling (void) {
  if (!active)        return;
  if (lock.NotDone()) return;
  lock.lock ();
  signal = target->halt_wait ();
  led.change ();
#ifdef DEBUG
  static int count = 0;
  debug ("%d Wait for signal=%d\r", count++, signal);
  fflush(stdout);
#endif // DEBUG
  if (signal) {
    debug ("\n");
    active = false;
    uint32_t watch_addr;
    /* Negative signal indicates we're in a syscall */
    if (signal < 0) {
      lock.unlock();
      return;
    }
    led.set ();
    /* Report reason for halt */
    if (target->check_hw_wp (&watch_addr)) {
      /* Watchpoint hit */
      gdb_putpacket_f ("T%02Xwatch:%08X;", signal, watch_addr);
    } else {
      gdb_putpacket_f ("T%02X", signal);
    }
  }
  lock.unlock();
}

uint32_t GdbServer::Up (char *pbuf, uint32_t plen) {
  debug ("Up::%d:\"%s\"\n", plen, pbuf);
  lock.lock();
  size = plen;
continue_activity:
  switch (pbuf[0]) {
    case 0: {           // Ctrl - C
      debug ("GdbServer::Up Ctrl-C\n");
      target->halt_request ();
      last_activity = 's';
      break;
    }
      /* Implementation of these is mandatory! */
    case 'g': { /* 'g': Read general registers */
      if (!target_check ()) break;
      int rsize = target->get_regs_size();
      debug ("RSIZE = %d\n", rsize);
      uint8_t arm_regs [rsize];
      target->regs_read (arm_regs);
      /* Výstup po blocích délky max. 240 bytů byl vynucen větší déklou dat registrů
       * CortexM4F. Zdá se, že to funguje.
       */
      const int  chunk = 120;
      int block, start = 0;
      for (;;) {
        if (rsize > chunk) block = chunk; 
        else               block = rsize;
        char* tptr =  (char*) hexify (pbuf, (const unsigned char*) arm_regs + start, block);
        gdb_putpacket (tptr , block * 2);
        start += block;
        rsize -= block;
        if (!rsize) break;
      }
      break;
    }
    case 'm': { /* 'm addr,len': Read len bytes from addr */
      unsigned long addr, len;
      if (!target_check ()) break;
      sscanf ((char*)pbuf, "m%08lx,%08lx", &addr, &len);
      debug ("m packet: addr = %08lX, len = %08lX\n", addr, len);
      uint8_t mem[len];
      if ( ( (addr & 3) == 0) && ( (len & 3) == 0))
        target->mem_read_words ((uint32_t*) mem, addr, len);
      else
        target->mem_read_bytes (mem, addr, len);
      if (target->check_error ())
        gdb_putpacketz ("E01");
      else
        gdb_putpacket ((char*)hexify (pbuf, mem, len), len*2);
      break;
    }
    case 'G': { /* 'G XX': Write general registers */
      if (!target_check ()) break;
      int rsize = target->get_regs_size();
      uint8_t arm_regs [rsize];
      unhexify (arm_regs, (unsigned char*) &pbuf[1], rsize);
      target->regs_write (arm_regs);
      gdb_putpacketz ("OK");
      break;
    }
    case 'M': { /* 'M addr,len:XX': Write len bytes to addr */
      unsigned long addr, len;
      int hex;
      if (!target_check ()) break;
      sscanf ((char*)pbuf, "M%08lx,%08lx:%n", &addr, &len, &hex);
      debug ("M packet: addr = %08lX, len = %08lX\n", addr, len);
      uint8_t mem[len];
      unhexify (mem, (unsigned char*)pbuf + hex, len);
      if ( ( (addr & 3) == 0) && ( (len & 3) == 0))
        target->mem_write_words (addr, (const uint32_t*) mem, len);
      else
        target->mem_write_bytes (addr, mem, len);
      if (target->check_error ())
        gdb_putpacketz ("E01");
      else
        gdb_putpacketz ("OK");
      break;
    }
    
    case 's': /* 's [addr]': Single step [start at addr] */
      single_step = true;
      // Fall through to resume target
    case 'c': /* 'c [addr]': Continue [at addr] */
      if (!target) {
        gdb_putpacketz ("X1D");
        break;
      }
      target->halt_resume (single_step);
      single_step = false;
      // Fall through to wait for target halt
    case '?': { /* '?': Request reason for target halt */
      /* This packet isn't documented as being mandatory,
       * but GDB doesn't work without it. */

      if (!target) {
        /* Report "target exited" if no target */
        gdb_putpacketz ("W00");
        break;
      }
      if (!target->attached) {
        /* Report "target exited" if no target */
        gdb_putpacketz ("W00");
        break;
      }

      last_activity = pbuf[0];
      /* Wait for target halt */
      debug("? WAIT HALT\n");
      SetActive ();            // timto se prepne do mainloop, kde ceka na zastaveni
      break;
    }
    case 'F': { /* Semihosting call finished */
      int retcode, errcode, items;
      char c, *p;
      if (pbuf[1] == '-')
        p = (char*) &pbuf[2];
      else
        p = (char*) &pbuf[1];
      items = sscanf (p, "%x,%x,%c", &retcode, &errcode, &c);
      if (pbuf[1] == '-')
        retcode = -retcode;

      target->hostio_reply (retcode, errcode);

      /* if break is requested */
      if (items == 3 && c == 'C') {
        gdb_putpacketz ("T02");
        break;
      }

      pbuf[0] = last_activity;
      goto continue_activity;
    }

    /* Optional GDB packet support */
    case '!': /* Enable Extended GDB Protocol. */
      /* This doesn't do anything, we support the extended
       * protocol anyway, but GDB will never send us a 'R'
       * packet unless we answer 'OK' here.
       */
      gdb_putpacketz ("OK");
      break;

    case 0x04: // ???
      debug ("CASE 4 packet ???\n");
    case 'D': /* GDB 'detach' command. */
      if (target) {
        if (target->attached) {
          target->detach ();
          target->reset  ();
        }
      }
      gdb_putpacketz ("OK");
      break;

    case 'k': /* Kill the target */
      if (target) {
        target->detach ();
        target->reset  ();
      }
      break;

    case 'r': /* Reset the target system */
    case 'R': /* Restart the target program */
      if (target && target->attach()) {
        target->reset ();
        SetActive ();
      }
      break;

    case 'X': { /* 'X addr,len:XX': Write binary data to addr */
      unsigned long addr, len;
      int bin, icp;
      if (!target_check ()) break;
      sscanf ((char*)pbuf, "X%08lx,%08lx:%n", &addr, &len, &bin);
      // On Cortex-M0 word align param to write_words
      for (icp=0; icp<(int)len; icp++) pbuf[icp] = pbuf[icp + bin];
      
      debug ("X packet: addr = %08lX, len = %08lX\n", addr, len);
      if ( ( (addr & 3) == 0) && ( (len & 3) == 0))
        target->mem_write_words (addr, (const uint32_t*)  pbuf, len);
      else
        target->mem_write_bytes (addr, (unsigned char*) pbuf, len);
      if (target->check_error   ())
        gdb_putpacketz ("E01");
      else
        gdb_putpacketz ("OK");
      break;
    }

    case 'q': /* General query packet */
      handle_q_packet ((char*)pbuf, size);
      break;

    case 'v': /* General query packet */
      handle_v_packet ((char*)pbuf, size);
      break;

      /* These packet implement hardware break-/watchpoints */
    case 'Z': /* Z type,addr,len: Set breakpoint packet */
    case 'z': /* z type,addr,len: Clear breakpoint packet */
      if (!target_check ()) break;
      handle_z_packet ((char*)pbuf, size);
      break;

    default:  /* Packet not implemented */
      debug ("*** Unsupported packet: %s\n", pbuf);
      gdb_putpacketz ("");
  }
  lock.unlock();
  //return BaseLayer::Up (pbuf, plen);
  return plen;
}

void GdbServer::handle_q_string_reply (const char *str, const char *param) {
  unsigned long addr, len;

  if (sscanf (param, "%08lx,%08lx", &addr, &len) != 2) {
    gdb_putpacketz ("E01");
    return;
  }
  if (addr < strlen (str)) {
    char reply [len+2];
    reply[0] = 'm';
    strncpy (reply + 1, &str [addr], len);
    if (len > strlen (&str [addr]))
      len = strlen (&str [addr]);
    gdb_putpacket (reply, len + 1);
  } else if (addr == strlen (str)) {
    gdb_putpacketz ("l");
  } else
    gdb_putpacketz ("E01");
}


#define BUF_SIZE PACKETBUFLEN

void GdbServer::handle_q_packet (char *packet, int len) {
  unsigned long addr, alen;

  if (!strncmp (packet, "qRcmd,", 6)) {
    int datalen;

    /* calculate size and allocate buffer for command */
    datalen = (len - 6) / 2;
    char data [datalen + 1];
    /* dehexify command */
    unhexify ((unsigned char*) data, (const unsigned char*)packet + 6, datalen);
    data[datalen] = 0; /* add terminating null */

    int c = mon.command (data);
    if (c < 0)
      gdb_putpacketz ("");
    else if (c == 0)
      gdb_putpacketz ("OK");
    else
      gdb_putpacketz ("E");

  } else if (!strncmp (packet, "qSupported", 10)) {
    /* Query supported protocol features */
    gdb_putpacket_f ("PacketSize=%X;qXfer:memory-map:read+;qXfer:features:read+", BUF_SIZE);

  } else if (strncmp (packet, "qXfer:memory-map:read::", 23) == 0) {
    /* Read target XML memory map */
    if ((!target) || (!target->xml_mem_map)) {
      gdb_putpacketz ("E01");
      return;
    }
    handle_q_string_reply (target->xml_mem_map, packet + 23);

  } else if (strncmp (packet, "qXfer:features:read:target.xml:", 31) == 0) {
    /* Read target description */
    if ((!target) || (!target->tdesc)) {
      gdb_putpacketz ("E01");
      return;
    }
    handle_q_string_reply (target->tdesc, packet + 31);
  } else if (sscanf (packet, "qCRC:%08lx,%08lx", &addr, &alen) == 2) {
    if (!target) {
      gdb_putpacketz ("E01");
      return;
    }
    gdb_putpacket_f ("C%lx", target->generic_crc32 (addr, alen));

  } else {      
    debug ("*** Unsupported packet: %s\n", packet);
    gdb_putpacket ("", 0);
  }

}
void GdbServer::handle_v_packet (char *packet, int plen) {
  unsigned long addr, len;
  int bin;
  static uint32_t flash_mode = 0;

  if (sscanf (packet, "vAttach;%08lx", &addr) == 1) {
    /* Attach to remote target processor */
    if (target && target->attach())
      gdb_putpacketz ("T05");
    else
      gdb_putpacketz ("E01");

  } else if (!strcmp (packet, "vRun;")) {
    /* Run target program. For us (embedded) this means reset. */
    if (target && target->attach()) {
        target->reset ();
        SetActive ();
        gdb_putpacketz ("T05");
      }
      else
        gdb_putpacketz ("E01"); 

  } else if (sscanf (packet, "vFlashErase:%08lx,%08lx", &addr, &len) == 2) {
    /* Erase Flash Memory */
    debug ("Flash Erase %08lX %08lX\n", addr, len);
    if (!target) {
      gdb_putpacketz ("EFF");
      return;
    }

    if (!flash_mode) {
      /* Reset target if first flash command! */
      /* This saves us if we're interrupted in IRQ context */
      target->reset ();
      flash_mode = 1;
    }
    if (target->flash_erase (addr, len) == 0)
      gdb_putpacketz ("OK");
    else
      gdb_putpacketz ("EFF");

  } else if (sscanf (packet, "vFlashWrite:%08lx:%n", &addr, &bin) == 1) {
    /* Write Flash Memory */
    len = plen - bin;
    debug ("Flash Write %08lX %08lX\n", addr, len);
    if (target && target->flash_write (addr, (unsigned char*) packet + bin, len) == 0)
      gdb_putpacketz ("OK");
    else
      gdb_putpacketz ("EFF");

  } else if (!strcmp (packet, "vFlashDone")) {
    /* Commit flash operations. */
    gdb_putpacketz ("OK");
    flash_mode = 0;

  } else {
    debug ("*** Unsupported packet: %s\n", packet);
    gdb_putpacket ("", 0);
  }

}
void GdbServer::handle_z_packet (char *packet, int plen) {
  (void) plen;

  uint8_t set = (packet[0] == 'Z') ? 1 : 0;
  int type, len;
  unsigned long addr;
  int ret;

  /* I have no idea why this doesn't work. Seems to work
   * with real sscanf() though... */
  //sscanf(packet, "%*[zZ]%hhd,%08lX,%hhd", &type, &addr, &len);
  type = packet[1] - '0';
  sscanf (packet + 2, ",%08lx,%d", &addr, &len);
  switch (type) {
  case 1: /* Hardware breakpoint */
    /*
    if (!target->set_hw_bp()) { // Not supported
      gdb_putpacketz ("");
      return;
    }
    */
    if (set)
      ret = target->set_hw_bp   (addr);
    else
      ret = target->clear_hw_bp (addr);
    break;

  case 2:
  case 3:
  case 4:
    /*
    if (!cur_target->set_hw_wp) { // Not supported
      gdb_putpacketz ("");
      return;
    }
    */
    if (set)
      ret = target->set_hw_wp   (type, addr, len);
    else
      ret = target->clear_hw_wp (type, addr, len);
    break;

  default:
    gdb_putpacketz ("");
    return;
  }

  if (!ret)
    gdb_putpacketz ("OK");
  else
    gdb_putpacketz ("E01");

}

/////////////////////////////////////////////////////////////////////////////////////////
/*
uint32_t GdbServer::Down (char *data, uint32_t len) {
  return BaseLayer::Down (data, len);
}
*/
void GdbServer::gdb_putpacket (const char *packet, int size) {
  BaseLayer::Down ((char*)packet, size);
}
void GdbServer::gdb_putpacketz (const char *packet) {
  int size = strlen (packet);
  gdb_putpacket (packet, size);
}
void GdbServer::gdb_putpacket_f (const char *fmt, ...) {
  va_list ap;
  char *buf;
  int size;

  va_start(ap, fmt);
  size = vasprintf (&buf, (const char*) fmt, ap);
  gdb_putpacket (buf, size);
  free(buf);
  va_end(ap);
}

void GdbServer::gdb_out(const char *buf) {
  int i = strlen (buf) * 2 + 1;

  char hexdata [i + 1];
  hexdata[0] = 'O';
  hexify (hexdata + 1, (unsigned char*) buf, strlen(buf));
  gdb_putpacket (hexdata, i);
}
/*
int GdbServer::gdb_outf(const char *fmt, ...) {
  va_list ap;
  char *buf;

  va_start(ap, fmt);
  int i = vasprintf(&buf, fmt, ap);
  gdb_out(buf);
  free(buf);
  va_end(ap);
  return i;
}
*/

