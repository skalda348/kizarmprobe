#include "packet.h"

Packet::Packet() : BaseLayer(), rx() {
  index = 0;
}

uint32_t Packet::Up (char *data, uint32_t len) {
  uint32_t i;
  for (i=0; i<len; i++) {
    if (!rx.Write (data[i])) return i; // to by nemelo nastat
  }
  for (;;) {
    if (!rx.Read  (pkt.b [index])) return i;
    index += 1;
    if (index == sizeof (pkt)) {
      wrap_echo();
      index = 0;
    }
  }
}

void Packet::wrap_echo (void) {
  if (pkt.p.cmd == cmdInit) {
    uint32_t res;
    res = BaseLayer::Up (pkt.b, 0);
    pkt.p.val = res;
  }
  else {
    BaseLayer::Up (pkt.b, sizeof (pkt));
  }
  Down (pkt.b, sizeof (pkt));
}
