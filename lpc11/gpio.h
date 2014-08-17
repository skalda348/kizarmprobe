#ifndef GPIO_H
#define GPIO_H

#include "LPC11Uxx.h"

typedef enum {
  GPIO_Mode_IN   = 0x00, /*!< GPIO Input Mode              */
  GPIO_Mode_OUT  = 0x01, /*!< GPIO Output Mode             */
} GPIODir_TypeDef;

/// Enum pro PortNumber
typedef enum {
  GpioPortA = 0,
  GpioPortB,
  GpioPortC,
  GpioPortD,
  GpioPortF
} GpioPortNum;
/** @file
  * @brief Obecný GPIO pin.
  * 
  * @class GpioClass
  * @brief Obecný GPIO pin.
  * 
  *  Ukázka přetížení operátorů. Návratové hodnoty jsou v tomto případě celkem zbytečné,
  * ale umožňují řetězení, takže je možné napsat např.
    @code
    +-+-+-led;
    @endcode
  * a máme na led 3 pulsy. Je to sice blbost, ale funguje.
  * Všechny metody jsou konstantní, protože nemění data uvnitř třídy.
  * Vlastně ani nemohou, protože data jsou konstantní.
*/
class GpioClass {
  public:
    /** Konstruktor
    @param port GpioPortA | GpioPortB | GpioPortC | GpioPortD | GpioPortF
    @param no   číslo pinu na portu
    @param type IN, OUT, AF, AN default OUT 
    */
    GpioClass (GpioPortNum const port, const uint32_t no, const GPIODir_TypeDef type = GPIO_Mode_OUT);
    /// Nastav pin @param b na tuto hodnotu
    const GpioClass& operator<< (const bool b) const {
      if (b) LPC_GPIO->SET[io] = pos;
      else   LPC_GPIO->CLR[io] = pos;
      return *this;
    }
//![Gpio example]
    /// Nastav pin na log. H
    const GpioClass& operator+ (void) const {
      LPC_GPIO->SET[io] = pos;
      return *this;
    }
    /// Nastav pin na log. L
    const GpioClass& operator- (void) const {
      LPC_GPIO->CLR[io] = pos;
      return *this;
    }
    /// Změň hodnotu pinu
    const GpioClass& operator~ (void) const {
      LPC_GPIO->NOT[io] = pos;
      return *this;
    };
    /// Načti logickou hodnotu na pinu
    const bool get (void) const {
      if (LPC_GPIO->PIN[io] & pos) return true;
      else                         return false;
    };
    /// A to samé jako operátor
    const GpioClass& operator>> (bool& b) const {
      b = get();
      return *this;
    }
//![Gpio example]
    void setDir (GPIODir_TypeDef p) {
      if (p) LPC_GPIO->DIR[io] |=  pos;
      else   LPC_GPIO->DIR[io] &= ~pos;
    }
  private:
    /// Port.
    GpioPortNum const    io;
    /// A pozice pinu na něm
    const uint32_t       pos;
  
};

#endif // GPIO_H
