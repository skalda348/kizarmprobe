#ifndef GPIO_H
#define GPIO_H

#include "LPC11Uxx.h"

typedef enum {
  GPIO_Mode_IN   = 0x00, /*!< GPIO Input Mode              */
  GPIO_Mode_OUT  = 0x01, /*!< GPIO Output Mode             */
} GPIODir_TypeDef;

/** @file
  * @brief Obecný GPIO pin.
  * 
  * @class GpioClass
  * @brief Obecný GPIO pin.
  * 
  * Všechny metody jsou konstantní, protože nemění data uvnitř třídy.
  * Vlastně ani nemohou, protože data jsou konstantní.
*/
class GpioClass {
  public:
    /** Konstruktor
    @param no   číslo pinu na portu 0
    @param type IN, OUT, AF, AN default OUT 
    */
    GpioClass (const uint32_t no, const GPIODir_TypeDef type = GPIO_Mode_OUT) :
      pos(1UL << no) {
      // Povol hodiny
      LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);
      // A nastav pin.
      setDir  (type);
    };
    /// Nastav pin @param b na tuto hodnotu
    const void set (const bool b) const {
      if (b) LPC_GPIO->SET[0] = pos;
      else   LPC_GPIO->CLR[0] = pos;
    };
    /// Nastav pin na log. H
    const void set (void) const {
      LPC_GPIO->SET[0] = pos;
    };
    /// Nastav pin na log. L
    const void clr (void) const {
      LPC_GPIO->CLR[0] = pos;
    };
    /// Změň hodnotu pinu
    const void change (void) const {
      LPC_GPIO->NOT[0] = pos;
    };
    /// Načti logickou hodnotu na pinu
    const bool get (void) const {
      if (LPC_GPIO->PIN[0] & pos) return true;
      else                        return false;
    };
    void setDir (GPIODir_TypeDef p) {
      if (p) LPC_GPIO->DIR[0] |=  pos;
      else   LPC_GPIO->DIR[0] &= ~pos;
    }
  private:
    /// A pozice pinu na něm
    const uint32_t       pos;
  
};



#endif // GPIO_H
