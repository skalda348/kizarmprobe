#ifndef SOCKET_H
#define SOCKET_H

//#define SOCKBUFLEN 64
#include "usbclass.h"
#include "baselayer.h"
#include "fifo.h"

class Socket : public UsbClass, public BaseLayer {

  public:
    Socket();
    //~Socket();
    void  Init     (void);
    bool  Fini     (void);
    void* Receiver (void);
    uint32_t Down  (char *data, uint32_t len);

    void     Send (void);
    bool     WaitForLineCode (void) {
      if (wait) {  wait = 0; return true; };
      return false;
    };
  protected:
    static ErrorCode_t VCOM_bulk_in_hdlr  (USBD_HANDLE_T hUsb, void* data, uint32_t event);
    static ErrorCode_t VCOM_bulk_out_hdlr (USBD_HANDLE_T hUsb, void* data, uint32_t event);
    static ErrorCode_t VCOM_SetLineCode (USBD_HANDLE_T hCDC, CDC_LINE_CODING* line_coding);
    static ErrorCode_t VCOM_SetCtrlLineState (USBD_HANDLE_T hCDC, uint16_t state);
  private:
    static volatile int wait;
    USBD_HANDLE_T hCdc;           //!< dáno driverem
    uint8_t* rxBuf;               //!< bufery jsou v USB oblasti dane driverem
    uint8_t* txBuf;               //!< prostě to tak je.
    volatile uint32_t usbtx_rdy;  //!< semafor pro čekání na kompletní výstup dat
    Fifo<char>        tx;
};

#endif // SOCKET_H
