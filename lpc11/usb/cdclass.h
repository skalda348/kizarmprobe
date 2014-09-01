#ifndef CDCLASS_H
#define CDCLASS_H

#ifdef __cplusplus

#include "usbclass.h"
#include "baselayer.h"
#include "fifo.h"
extern "C" {
#include "cdc.h"
#ifdef SERIAL
  #include "comp_desc.h"
#else
  #include "usb-desc.h"
#endif // SERIAL
}
class CDCIndividual;

class CDClass : public UsbClass, public BaseLayer {
  public:
    CDClass       (const CDCIndividual * ip);
    uint32_t Down (char* buf, uint32_t len);
    /// Pomocná metoda pro kompatibilitu se PC
    void     Init (void) {
      connect();
    };
    /// Pomocná metoda pro kompatibilitu se PC
    bool     Fini (void) {
      return true;
    };
  protected:
    static void VCOM_EpHandler (void* data, uint32_t event);
    void        Send           (void);
    void        Recv           (uint32_t len);
  private:
    uint32_t          bulkIn,
                      bulkOut;
    uint8_t           rxBuf [USB_MAX_NON_ISO_SIZE];
    uint8_t           txBuf [USB_MAX_NON_ISO_SIZE];
    volatile uint32_t usbtx_rdy;  //!< semafor pro čekání na kompletní výstup dat
    Fifo<char>        tx;
};

#endif // __cplusplus
#endif // CDCLASS_H
