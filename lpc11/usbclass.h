#ifndef USBCLASS_H
#define USBCLASS_H
/**
 * @file
 * @brief USB Základ.
 */
#include <stdint.h>
extern "C" {
  #include "LPC11Uxx.h"
  #include "power_api.h"
  #include "mw_usbd_rom_api.h"
}
/// @brief Práce s pamětí
class UsbMem {
  public:
    /// Základní nastavení
    void init (void) {
      membase = 0x10001000;
      memsize = 0x0800;
    };
    /// Jen vrací bázi @return membase
    uint32_t base (void) {
      return membase;
    };
    /// Jen vrací velikost @return memsize
    uint32_t size (void) {
      return memsize;
    }
    /// Takhle nepochopitelně to NXP dělá.
    void resize (uint32_t s) {
      membase += memsize - s;
      memsize  = s;
    }
  private:
    /// membase
    uint32_t membase;
    /// memsize
    uint32_t memsize;
};

class UsbClass {
  public:
    /// Konstruktor
    UsbClass ();
    /// Spojení po enumeraci
    void connect  (void);
    /// Getter pro @return hUsb
    static USBD_HANDLE_T getHandle (void) {
      return hUsb;
    };
    /// Třída pro práci s pamětí.
    static UsbMem        mem;
  protected:
    /// Taky se musí nějak nastavit piny.
    void PinInit  (void);
  private:
    static USBD_HANDLE_T hUsb;           //!< dáno driverem
};

#endif // USBCLASS_H
