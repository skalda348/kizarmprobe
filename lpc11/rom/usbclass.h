#ifndef USBCLASS_H
#define USBCLASS_H

#include <stdint.h>
extern "C" {
#include "LPC11Uxx.h"
#include "power_api.h"
#include "mw_usbd_rom_api.h"
}
// Resize pameti provede Init()
class UsbMem {
  public:
    void init (void) {
//      membase = 0x20004000;
//      memsize = 0x0800;
      membase = 0x10001000;
      memsize = 0x0800;
    };
    uint32_t membase;
    uint32_t memsize;
};

class UsbClass {
  public:
    UsbClass ();
    void connect  (void);
    static USBD_HANDLE_T getHandle (void) {
      return hUsb;
    };
    static UsbMem        mem;
  protected:
    void PinInit  (void);
  private:
    static USBD_HANDLE_T hUsb;           //!< dáno driverem
};

#endif // USBCLASS_H
