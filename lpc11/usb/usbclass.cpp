#ifdef SERIAL
  #include "comp_desc.h"
#else
  #include "usb-desc.h"
#endif // SERIAL

#include "usbclass.h"

UsbClass * UsbClass::instance = 0;

UsbClass::UsbClass () {
  if (instance) return;
  instance = this;
  
  USB_Init ();
  //asm volatile ("bkpt 0"); // debug - skoncilo OK ?
}
// Volat jen u jednoho elementu, dediciho usbclass (slo by osetrit, ale nema to smysl)
void UsbClass::connect (void) {
  if (isConfigured()) return;
  USB_Connect(1);
  while (!isConfigured());
}
