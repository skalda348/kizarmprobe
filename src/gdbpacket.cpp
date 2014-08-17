#include <stdio.h>
#include "gdbpacket.h"
#include "utils.h"

GdbPacket::GdbPacket() : BaseLayer() {
  index = 0;
  csum  = 0;
  state = PacketStateIdle;
  
  // maxsize = 0;
}

void GdbPacket::RecAck (bool ack) {

}
void GdbPacket::RecEnd (void) {
  debug ("Ctrl-C\n");
  char c = 0;
  BaseLayer::Up (&c, 1);
  SendAck (true);
}
void GdbPacket::RecDel (void) {
  debug ("Ctrl-D\n");
  char c = 4;
  BaseLayer::Up (&c, 1);
  SendAck (true);
}
void GdbPacket::SendAck (bool ack) {
  char c;
  if (ack) c = '+';
  else     c = '-';
  BaseLayer::Down (&c,1);
}

bool GdbPacket::Parse (unsigned char c) {
  switch (state) {
    case PacketStateIdle:
      switch (c) {
        case 0x03: RecEnd ();      break;
        case 0x04: RecDel ();      break;
        case '+':  RecAck (true);  break;
        case '-':  RecAck (false); break;
        case '$': 
          index = 0;
          csum  = 0;
          state = PacketStatePending;
          break;
        default: break;
      }
      break;
    case PacketStatePending:
      if (c == '#') {
        state = PacketStateCS0;
        break;
      }
      if (c == '}') {
        state = PacketStateEscape;
      } else {
        pbuf [index++] = c;
      }
      csum += c;
      break;
    case PacketStateEscape:
      csum          += c;
      pbuf [index++] = c | 0x20;
      state = PacketStatePending;
      break;
    case PacketStateCS0:
      rsum  = FromHex (c);
      state = PacketStateCS1;
      break;
    case PacketStateCS1:
      rsum *= 0x10;
      rsum += FromHex (c);
      //debug ("R%X=C%X, len=%d\n", rsum, csum, index);
      if (rsum == csum) {
        SendAck (true);
        pbuf [index] = 0;
        BaseLayer::Up ((char*) pbuf, index);
      }
      else {
        debug ("!!! Check Sum ERROR - %02X != %02X\n", rsum, csum);
        SendAck (false);
      }

      state = PacketStateIdle;
      break;
    default:
      break;
  }
  return true;
}

uint32_t GdbPacket::Up (char *data, uint32_t len) {
  uint32_t n;
  for (n=0; n<len; n++) {
    if (!Parse ((unsigned char) data [n])) break;
  }
  return n;
}
/* ARM po 1
uint32_t GdbPacket::Down (char *data, uint32_t len) {
  if (!len) return 0;
  uint32_t n;
  unsigned char tchar;
  unsigned char ket = '}';
  tsum = 0;
  tchar = '$';
  BaseLayer::Down ((char*)&tchar, 1);
  for (n=0; n<len; n++) {
    tchar = data [n];
    if((tchar == '$') || (tchar == '#') || (tchar == ket)) {
      BaseLayer::Down ((char*)&ket,   1);
      tsum  += ket;
      tchar ^= 0x20;
    }
    BaseLayer::Down ((char*)&tchar, 1);
    tsum  += tchar;
  }
  tchar = '#';
  BaseLayer::Down ((char*)&tchar, 1);
  tchar = toHex (tsum >>   4);
  BaseLayer::Down ((char*)&tchar, 1);
  tchar = toHex (tsum & 0x0F);
  BaseLayer::Down ((char*)&tchar, 1);
  return n;
}
*/
uint32_t GdbPacket::Down (char *data, uint32_t len) {
  // if (!len) return 0; // musi ven i prazdny paket !!!
  uint32_t n, i=0;
  unsigned char tchar;
  unsigned char ket = '}';
  char tbuf [PACKETBUFLEN + 16];        // s rezervou
  tsum  = 0;
  tchar = '$';
  tbuf [i++] = tchar;
  for (n=0; n<len; n++) {
    tchar = data [n];
    if((tchar == '$') || (tchar == '#') || (tchar == ket)) {
      tbuf [i++] = ket;
      tsum  += ket;
      tchar ^= 0x20;
    }
    tbuf [i++] = tchar;
    tsum      += tchar;
  }
  tchar = '#';
  tbuf [i++] = tchar;
  tchar = toHex (tsum >>   4);
  tbuf [i++] = tchar;
  tchar = toHex (tsum & 0x0F);
  tbuf [i++] = tchar;
  // Wait for ack ??? Zatim zahazuji.
  // if (i > maxsize) maxsize = i;
  return BaseLayer::Down (tbuf, i);
}
