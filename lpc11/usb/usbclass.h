#ifndef USBCLASS_H
#define USBCLASS_H

#include <stdint.h>
extern "C" {
  #include "LPC11Uxx.h"
  #include "usb.h"
  #include "usbcfg.h"
  #include "usbhw.h"
}
extern "C" volatile int UsbIsConfigured;

class UsbClass {
  public:
    UsbClass ();
    void connect  (void);
    bool isConfigured (void) {
      if (UsbIsConfigured) return true;
      return false;
    }
  protected:
  private:
    static UsbClass * instance;
};

#endif // USBCLASS_H
