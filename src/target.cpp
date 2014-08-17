#include <stdio.h>
#include "utils.h"
#include "baselayer.h"
#include "target.h"
#include "gdbserver.h"
#include "monitor.h"
#include "swdp.h"

Target::Target (GdbServer *s) : apdp() {
  regs_size = 20 * 4;
  attached  = false;
  // GdbServer bohužel potřebují snad všechny podtřídy.
  // Nechtělo se mi dělat jej globálně, takže ukazatelů je plno.
  gdb         = s;
  apdp.dp.gdb = s;
}

GdbServer *Target::getServer (void) {
  return gdb;
}
bool Target::probe (void) {

  idcode = gdb->BaseLayer::Up ((char*)"", 0);
  debug ("CoreId=0x%08X\n", idcode);
  if (!idcode) {
    debug ("No IDCODE\n");
    return false;
  }

  apdp.dp.error ();
  apdp.dp.dp_init ();


  return  true;

}

int Target::mem_read_bytes (uint8_t *dest, uint32_t src, int len) {
  debug ("%s: from %08X, len=%d\n", __func__, src, len);
  uint32_t tmp;
  uint32_t osrc = src;

  apdp.ap.ap_write (ADIV5_AP_CSW, apdp.ap.csw |
                ADIV5_AP_CSW_SIZE_BYTE | ADIV5_AP_CSW_ADDRINC_SINGLE);
  apdp.dp.low_access (ADIV5_LOW_AP, ADIV5_LOW_WRITE,
                      ADIV5_AP_TAR, src);
  apdp.dp.low_access (ADIV5_LOW_AP, ADIV5_LOW_READ,
                      ADIV5_AP_DRW, 0);
  while (--len) {
    tmp = apdp.dp.low_access (1, 1, ADIV5_AP_DRW, 0);
    *dest++ = (tmp >> ( (src & 0x3) << 3) & 0xFF);

    src++;
    /* Check for 10 bit address overflow */
    if ( (src ^ osrc) & 0xfffffc00) {
      osrc = src;
      apdp.dp.low_access (ADIV5_LOW_AP,
                          ADIV5_LOW_WRITE, ADIV5_AP_TAR, src);
      apdp.dp.low_access (ADIV5_LOW_AP,
                          ADIV5_LOW_READ, ADIV5_AP_DRW, 0);
    }
  }
  tmp = apdp.dp.low_access (0, 1, ADIV5_DP_RDBUFF, 0);
  *dest++ = (tmp >> ( (src++ & 0x3) << 3) & 0xFF);

  return 0;
}
int Target::mem_read_words (uint32_t *dest, uint32_t src, int len) {
  debug ("%s: from %08X, len=%d\n", __func__, src, len);
  uint32_t osrc = src;

  len >>= 2;

  apdp.ap.ap_write (ADIV5_AP_CSW, apdp.ap.csw |
                ADIV5_AP_CSW_SIZE_WORD | ADIV5_AP_CSW_ADDRINC_SINGLE);
  apdp.dp.low_access (ADIV5_LOW_AP, ADIV5_LOW_WRITE,
                      ADIV5_AP_TAR, src);
  apdp.dp.low_access (ADIV5_LOW_AP, ADIV5_LOW_READ,
                      ADIV5_AP_DRW, 0);
  while (--len) {
    *dest++ = apdp.dp.low_access (ADIV5_LOW_AP,
                                  ADIV5_LOW_READ, ADIV5_AP_DRW, 0);
    src += 4;
    /* Check for 10 bit address overflow */
    if ( (src ^ osrc) & 0xfffffc00) {
      osrc = src;
      apdp.dp.low_access (ADIV5_LOW_AP,
                          ADIV5_LOW_WRITE, ADIV5_AP_TAR, src);
      apdp.dp.low_access (ADIV5_LOW_AP,
                          ADIV5_LOW_READ, ADIV5_AP_DRW, 0);
    }

  }
  *dest++ = apdp.dp.low_access (ADIV5_LOW_DP, ADIV5_LOW_READ,
                                ADIV5_DP_RDBUFF, 0);

  return 0;
}
int Target::mem_write_bytes (uint32_t dest, const uint8_t *src, int len) {
  debug ("%s: to   %08X, len=%d\n", __func__, dest, len);
  uint32_t odest = dest;
  if (!len) return 0;

  apdp.ap.ap_write (ADIV5_AP_CSW, apdp.ap.csw |
                ADIV5_AP_CSW_SIZE_BYTE | ADIV5_AP_CSW_ADDRINC_SINGLE);
  apdp.dp.low_access (ADIV5_LOW_AP, ADIV5_LOW_WRITE,
                      ADIV5_AP_TAR, dest);
  while (len--) {
    uint32_t tmp = (uint32_t) *src++ << ( (dest++ & 3) << 3);
    apdp.dp.low_access (ADIV5_LOW_AP, ADIV5_LOW_WRITE,
                        ADIV5_AP_DRW, tmp);

    /* Check for 10 bit address overflow */
    if ( (dest ^ odest) & 0xfffffc00) {
      odest = dest;
      apdp.dp.low_access (ADIV5_LOW_AP,
                          ADIV5_LOW_WRITE, ADIV5_AP_TAR, dest);
    }
  }
  return 0;
}
int Target::mem_write_words (uint32_t dest, const uint32_t *src, int len) {
  debug ("%s: to   %08X, len=%d\n", __func__, dest, len);
  uint32_t odest = dest;

  len >>= 2;
  if (!len) return 0;

  apdp.ap.ap_write (ADIV5_AP_CSW, apdp.ap.csw |
                ADIV5_AP_CSW_SIZE_WORD | ADIV5_AP_CSW_ADDRINC_SINGLE);
  apdp.dp.low_access (ADIV5_LOW_AP, ADIV5_LOW_WRITE,
                      ADIV5_AP_TAR, dest);
  while (len--) {
    apdp.dp.low_access (ADIV5_LOW_AP, ADIV5_LOW_WRITE,
                        ADIV5_AP_DRW, *src++);
    dest += 4;
    /* Check for 10 bit address overflow */
    if ( (dest ^ odest) & 0xfffffc00) {
      odest = dest;
      apdp.dp.low_access (ADIV5_LOW_AP,
                          ADIV5_LOW_WRITE, ADIV5_AP_TAR, dest);
    }
  }

  return 0;
}
uint32_t Target::generic_crc32 (uint32_t base, int len) {
  debug ("%s: base=0x%08X,len=%d\n", __func__, base, len);
  uint32_t crc = -1;
  uint8_t  byte;

  while (len--) {
    if (mem_read_bytes (&byte, base, 1) != 0)
      return -1;

    crc = crc32_calc (crc, byte);
    base++;
  }
  return crc;

}

int Target::flash_erase (uint32_t addr, int len) {
  debug ("!!! %s\n", __func__);
  return 0;

}
int Target::flash_write (uint32_t dest, const uint8_t *src, int len) {
  debug ("!!! %s\n", __func__);
  return 0;

}
int Target::check_error (void) {
  debug ("%s\n", __func__);
  return apdp.dp.error(); // ???
}
