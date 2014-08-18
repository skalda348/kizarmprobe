#ifndef PACKET_H
#define PACKET_H

#include "swdp.h"
#include "fifo.h"

/**
 * @file
 * Mezivrstva pro přenášení debug paketů mezi CDC (class Socket) a Swdp.
 * Ten přenos v podstatě 2 typů zpráv na swd je zvolen poněkud nešťastně,
 * ale už to tak je, chodí to a tak s tím nebudu hýbat.
 */

class Packet : public BaseLayer {

  public:
    Packet();
    uint32_t Up (char *data, uint32_t len);
  protected:
    void wrap_echo (void);
  private:
    union {
      swdPacket p;
      char      b [sizeof (swdPacket)];
    } pkt;
    Fifo<char>  rx;
    uint32_t    index;
};

#endif // PACKET_H
