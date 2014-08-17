#ifndef GDBPACKET_H
#define GDBPACKET_H

#include "baselayer.h"

#define PACKETBUFLEN 256

enum GdbPacketState {
  PacketStateIdle = 0,
  PacketStatePending,
  PacketStateEscape,
  PacketStateCS0,
  PacketStateCS1
};

class GdbPacket : public BaseLayer {

  public:
    GdbPacket();
    bool Parse    (unsigned char c);
    void RecEnd   (void);
    void RecDel   (void);
    void RecAck   (bool ack);
    void SendAck  (bool ack);
    uint32_t Up   (char *data, uint32_t len);
    uint32_t Down (char *data, uint32_t len);
  private:
    uint8_t        pbuf [PACKETBUFLEN];
    GdbPacketState state;
    uint32_t       index;
    uint8_t        csum, rsum, tsum;
    
    // uint32_t maxsize;
};

#endif // GDBPACKET_H
