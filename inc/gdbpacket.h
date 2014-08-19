#ifndef GDBPACKET_H
#define GDBPACKET_H

/**
 * @file
 * @brief Mezivrsta paketů gdb.
 * */
#include "baselayer.h"

#define PACKETBUFLEN 256
/// Enumerace pro konečný automat
enum GdbPacketState {
  PacketStateIdle = 0,
  PacketStatePending,
  PacketStateEscape,
  PacketStateCS0,
  PacketStateCS1
};

/**
 * @brief Mezivrsta paketů gdb.
 * Ořezává a vytváří prefix, zpracuje escape sekvence a vyhodnocuje (a doplňuje)
 * kontrolní součty. Zároveň vyhodnocuje a vytváří odpovědi +/-.
 * */
class GdbPacket : public BaseLayer {

  public:
    /// Konstruktor
    GdbPacket();
    /// Parser příjmu
    bool Parse    (unsigned char c);
    /// Akce na příjem Ctrl-C
    void RecEnd   (void);
    /// Akce na příjem Ctrl-D
    void RecDel   (void);
    /// reakce na +/- (fakticky zatím nic nedělá)
    void RecAck   (bool ack);
    /// Vyšle potvrzení +/-
    void SendAck  (bool ack);
    /// zapojení do řetězce nahoru
    uint32_t Up   (char *data, uint32_t len);
    /// zapojení do řetězce dolu
    uint32_t Down (char *data, uint32_t len);
  private:
    /// do tohoto buferu se štosují čistá data
    uint8_t        pbuf [PACKETBUFLEN];
    /// stav konečného automatu
    GdbPacketState state;
    /// index pro zápis dat
    uint32_t       index;
    /// kontrolní sumy
    uint8_t        csum, rsum, tsum;
    
    // uint32_t maxsize;
};

#endif // GDBPACKET_H
