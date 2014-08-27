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

class CDCIndividual;

class CDClass : public UsbClass, public BaseLayer {
  public:
    CDClass (const CDCIndividual * ip);
    uint32_t Down (char* buf, uint32_t len);
    void     Send (void);
    void     Recv (uint32_t len);
    void     Init (void) {
      connect();
    };
    bool     Fini (void) {
      return true;
    };
  protected:
    static ErrorCode_t VCOM_bulk_in_hdlr  (USBD_HANDLE_T hUsb, void* data, uint32_t event);
    static ErrorCode_t VCOM_bulk_out_hdlr (USBD_HANDLE_T hUsb, void* data, uint32_t event);
  private:
    uint32_t bulkIn,
             bulkOut;
    USBD_HANDLE_T hCdc;           //!< dáno driverem
    uint8_t* rxBuf;               //!< bufery jsou v USB oblasti dane driverem
    uint8_t* txBuf;               //!< prostě to tak je.
    volatile uint32_t usbtx_rdy;  //!< semafor pro čekání na kompletní výstup dat
    Fifo<char>        tx;
};

#endif // CDCLASS_H
