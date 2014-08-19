#ifndef SOCKET_H
#define SOCKET_H

/**
 * @file
 * @brief Jeden konec řetězce. Zprostředkuje výměnu dat s gdb.
 **/

//#define SOCKBUFLEN 64
#include "usbclass.h"
#include "baselayer.h"
#include "fifo.h"
/**
 * @brief Jeden konec řetězce. Zprostředkuje výměnu dat s gdb.
 * 
 * Jak bylo naznačeno, socket je to v PC. Ve firmware je to USB CDC class. Používá ROM driver
 * zabudovaný přímo v čipu, takže není zase tak složitý. Dědí obecné vlastnosti USB a zároveň
 * BaseLayer, takže to jde přímo zapojit do řetězce.
 **/

class Socket : public UsbClass, public BaseLayer {

  public:
    /// Konstruktor.
    Socket();
    /**
     * @brief Inicializace USB stacku.
     * Tato metoda musí být volána po zapojení do řetězce, čeká na připojení USB.
     **/
    void  Init     (void);
    /// Místo destruktoru.
    bool  Fini     (void);
    /// Pro kompatibilitu s PC
    void* Receiver (void);
    /**
     * @brief Přetížená metoda Down().
     * Voláme pro fyzický výstup dat na CDC.
     * @param data ukazatel na data
     * @param len jejich délka
     * @return uint32_t počet vrácených byte
     **/
    uint32_t Down  (char *data, uint32_t len);
    /// Vlastně vnitřní metoda, ale nechme tak
    void     Send (void);
    /// Pomocná metoda - čeká na připojení USB
    bool     WaitForLineCode (void) {
      if (wait) {  wait = 0; return true; };
      return false;
    };
  protected:
    /// Vnitřní metody driveru.
    static ErrorCode_t VCOM_bulk_in_hdlr  (USBD_HANDLE_T hUsb, void* data, uint32_t event);
    static ErrorCode_t VCOM_bulk_out_hdlr (USBD_HANDLE_T hUsb, void* data, uint32_t event);
    static ErrorCode_t VCOM_SetLineCode (USBD_HANDLE_T hCDC, CDC_LINE_CODING* line_coding);
    static ErrorCode_t VCOM_SetCtrlLineState (USBD_HANDLE_T hCDC, uint16_t state);
  private:
    static volatile int wait;     //!< zámek pro čekání připojení
    USBD_HANDLE_T hCdc;           //!< dáno driverem
    uint8_t* rxBuf;               //!< bufery jsou v USB oblasti dane driverem
    uint8_t* txBuf;               //!< prostě to tak je.
    volatile uint32_t usbtx_rdy;  //!< semafor pro čekání na kompletní výstup dat
    Fifo<char>        tx;         //!< Výstupní fronta
};

#endif // SOCKET_H
