#include "stm32f4xx.h"
#include "gpio.h"
#include "config.h"
// Tabulka pár bytu ve flash, kód to zkrátí i zrychlí.
static const GpioAssocPort cPortTab[] = {
  {GPIOA, RCC_AHB1Periph_GPIOA},
  {GPIOB, RCC_AHB1Periph_GPIOB},
  {GPIOC, RCC_AHB1Periph_GPIOC},
  {GPIOD, RCC_AHB1Periph_GPIOD},
  {GPIOE, RCC_AHB1Periph_GPIOE},
  {GPIOF, RCC_AHB1Periph_GPIOF},
  {GPIOG, RCC_AHB1Periph_GPIOG},
  {GPIOH, RCC_AHB1Periph_GPIOH},
  {GPIOI, RCC_AHB1Periph_GPIOI},
};

GpioClass::GpioClass (const uint32_t no, const GPIOMode_TypeDef type) :
  io(cPortTab[SWDPORT].portAdr), pos(1UL << no), num(no) {
  // Povol hodiny
  RCC->AHB1ENR |= cPortTab[SWDPORT].clkMask;
  // A nastav pin (pořadí dle ST knihovny).
  setSpeed (GPIO_Speed_2MHz);
  setOType (GPIO_OType_PP);
  setMode  (type);
  setPuPd  (GPIO_PuPd_NOPULL);
}
/// F4 má ty DPIO moc rychlé, tak je zpomalíme.
static inline void delay (void) {
  register volatile uint32_t t = 10;
  while (--t);
}

void GpioClass::set (const bool b) const {
  if (b)
    io->BSRRL = pos;
  else
    io->BSRRH = pos;
  delay();
}
/// Nastav pin na log. H
void GpioClass::set (void) const {
  io->BSRRL = pos;
  delay();
}
/// Nastav pin na log. L
void GpioClass::clr (void) const {
  io->BSRRH = pos;
  delay();
}
/// Změň hodnotu pinu
void GpioClass::change (void) const {
  io->ODR ^= pos;
  delay();
};
/// Načti logickou hodnotu na pinu
const bool GpioClass::get (void) const {
  if (io->IDR & pos) return true;
  else               return false;
};
void GpioClass::setDir (GPIODir_TypeDef p) {
  uint32_t dno = num * 2;
  if (p)
    io->MODER   |=  (1UL << dno);
  else
    io->MODER   &= ~(3UL << dno);
}
/** ***************************************************************************/
void GpioClass::setMode (GPIOMode_TypeDef p) {
  uint32_t dno = num * 2;
  io->MODER   &= ~(3UL << dno);
  io->MODER   |=  (p   << dno);
}
void GpioClass::setOType (GPIOOType_TypeDef p) {
  io->OTYPER  &= ~(1UL << num);
  io->OTYPER  |=  (p   << num);
}
void GpioClass::setSpeed (GPIOSpeed_TypeDef p) {
  uint32_t dno = num * 2;
  io->OSPEEDR &= ~(3UL << dno);
  io->OSPEEDR |=  (p   << dno);
}
void GpioClass::setPuPd (GPIOPuPd_TypeDef p) {
  uint32_t dno = num * 2;
  io->PUPDR   &= ~(3UL << dno);
  io->PUPDR   |=  (p   << dno);
}
void GpioClass::setAF (unsigned af) {
  register unsigned int pd,pn = num;
  pd = (pn & 7) << 2; pn >>= 3;
  io->AFR[pn] &= ~(0xFU << pd);
  io->AFR[pn] |=  (  af << pd);
}

