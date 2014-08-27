#ifndef GPIO_H
#define GPIO_H

typedef enum {
  GPIO_Mode_IN   = 0x00, /*!< GPIO Input Mode              */
  GPIO_Mode_OUT  = 0x01, /*!< GPIO Output Mode             */
} GPIODir_TypeDef;

/**
  * @class GpioClass
  * @brief Obecný GPIO pin.
  * 
  * Pro kompatibilitu s firmware, zde nic nedělá.
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
    GpioClass (const uint32_t no, const GPIODir_TypeDef type = GPIO_Mode_OUT) :
      pos(1UL << no) {
      setDir  (type);
    };
    /// Nastav pin @param b na tuto hodnotu
    const void set (const bool b) const {
    };
    /// Nastav pin na log. H
    const void set (void) const {
    };
    /// Nastav pin na log. L
    const void clr (void) const {
    };
    /// Změň hodnotu pinu
    const void change (void) const {
    };
    /// Načti logickou hodnotu na pinu
    const bool get (void) const {
      return true;
    };
    void setDir (GPIODir_TypeDef p) {
    }
  private:
    /// A pozice pinu na něm
    const uint32_t       pos;
  
};



#endif // GPIO_H
