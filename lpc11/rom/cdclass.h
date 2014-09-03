#ifndef CDCLASS_H
#define CDCLASS_H

#ifdef SERIAL
  #include "comp_desc.h"
#else
  #include "usb-desc.h"
#endif

#include "usbclass.h"
#include "baselayer.h"
#include "fifo.h"

/// @file
/// @brief Virtuální sériový port

/// Forward declaration
class CDCIndividual;

/**
 * @brief Virtuální sériový port jako třída
 * 
 * Používá ROM driver a dědí UsbClass. Byla snaha udělat to tak, aby se dalo vytvořit případně
 * více instancí a propojit je s deskriptory, ale ROM driver to patrně nepodporuje. Bulk
 * endpointy fungují bez problémů, ale endpoint 0 zdá se nějaký problém má. Bude to chtít
 * podívat se, co vlastně znamená to USBD_HANDLE_T hCDC. V dokumentaci to ale není.
 **/
class CDClass : public UsbClass, public BaseLayer {
  public:
    /// Konstruktor
    CDClass (const CDCIndividual * ip);
    /// přetížení BaseLayer::Down()
    uint32_t Down (char* buf, uint32_t len);
    /// Pomocná metoda
    void     Send (void);
    /// Pomocná metoda
    void     Recv (uint32_t len);
    /// Pomocná metoda pro kompatibilitu se PC
    void     Init (void) {
      connect();
    };
    /// Pomocná metoda pro kompatibilitu se PC
    bool     Fini (void) {
      return true;
    };
  protected:
    /// Handler BULK IN endpointu
    static ErrorCode_t VCOM_bulk_in_hdlr  (USBD_HANDLE_T hUsb, void* data, uint32_t event);
    /// Handler BULK OUT endpointu
    static ErrorCode_t VCOM_bulk_out_hdlr (USBD_HANDLE_T hUsb, void* data, uint32_t event);
  private:
    uint32_t bulkIn,              //!< adresa BULK IN endpointu
             bulkOut;             //!< adresa BULK OUT endpointu
    USBD_HANDLE_T hCdc;           //!< dáno driverem
    uint8_t* rxBuf;               //!< bufery jsou v USB oblasti dane driverem
    uint8_t* txBuf;               //!< prostě to tak je.
    volatile uint32_t usbtx_rdy;  //!< semafor pro čekání na kompletní výstup dat
    Fifo<char>        tx;
};

#endif // CDCLASS_H
