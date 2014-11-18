#ifndef CDCLASS_H
#define CDCLASS_H

#include "baselayer.h"
#include "fifo.h"

/// @file
/// @brief Virtuální sériový port


/**
 * @brief Virtuální sériový port jako třída
 * 
 **/
class CDClass : public BaseLayer {
  public:
    /// Konstruktor
    CDClass (const int port);
    /// přetížení BaseLayer::Down()
    uint32_t Down (char* buf, uint32_t len);
    /// Pomocná metoda
    //  void     Send (void);
    /// Pomocná metoda
    void     Recv (uint32_t len);
    void     connect (void);
    /// Pomocná metoda pro kompatibilitu s PC
    void     Init (void) {
      connect();
    };
    /// Pomocná metoda pro kompatibilitu s PC
    bool     Fini (void) {
      return true;
    };
    /// statické metody pro spojení s C kódem (nemohou být veřejné)
    static uint16_t VCP_Init   ( void );
    static uint16_t VCP_DeInit ( void );
    static uint16_t VCP_Ctrl   ( uint32_t Cmd, uint8_t* Buf, uint32_t Len );
    static uint16_t VCP_DataTx ( uint8_t* Buf, uint32_t Len );
    static uint16_t VCP_DataRx ( uint8_t* Buf, uint32_t Len );
  protected:
  private:
    Fifo<uint8_t>       tx;
};

#endif // CDCLASS_H
