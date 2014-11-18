#ifndef GPIO_H
#define GPIO_H

//#include "stm32f4xx.h"

#define RCC_AHB1Periph_GPIOA             ((uint32_t)0x00000001)
#define RCC_AHB1Periph_GPIOB             ((uint32_t)0x00000002)
#define RCC_AHB1Periph_GPIOC             ((uint32_t)0x00000004)
#define RCC_AHB1Periph_GPIOD             ((uint32_t)0x00000008)
#define RCC_AHB1Periph_GPIOE             ((uint32_t)0x00000010)
#define RCC_AHB1Periph_GPIOF             ((uint32_t)0x00000020)
#define RCC_AHB1Periph_GPIOG             ((uint32_t)0x00000040)
#define RCC_AHB1Periph_GPIOH             ((uint32_t)0x00000080)
#define RCC_AHB1Periph_GPIOI             ((uint32_t)0x00000100)

/** 
  * @brief  GPIO Configuration Mode enumeration 
  */   
typedef enum { 
  GPIO_FMode_IN   = 0x00, /*!< GPIO Input Mode */
  GPIO_FMode_OUT  = 0x01, /*!< GPIO Output Mode */
  GPIO_FMode_AF   = 0x02, /*!< GPIO Alternate function Mode */
  GPIO_FMode_AN   = 0x03  /*!< GPIO Analog Mode */
}GPIOMode_TypeDef;

/** 
  * @brief  GPIO Output type enumeration 
  */  
typedef enum { 
  GPIO_OType_PP = 0x00,
  GPIO_OType_OD = 0x01
}GPIOOType_TypeDef;
/** 
  * @brief  GPIO Output Maximum frequency enumeration 
  */  
typedef enum { 
  GPIO_Speed_2MHz   = 0x00, /*!< Low speed */
  GPIO_Speed_25MHz  = 0x01, /*!< Medium speed */
  GPIO_Speed_50MHz  = 0x02, /*!< Fast speed */
  GPIO_Speed_100MHz = 0x03  /*!< High speed on 30 pF (80 MHz Output max speed on 15 pF) */
}GPIOSpeed_TypeDef;
/** 
  * @brief  GPIO Configuration PullUp PullDown enumeration 
  */ 
typedef enum { 
  GPIO_PuPd_NOPULL = 0x00,
  GPIO_PuPd_UP     = 0x01,
  GPIO_PuPd_DOWN   = 0x02
}GPIOPuPd_TypeDef;
/// Enum pro PortNumber
typedef enum {
  GpioPortA,
  GpioPortB,
  GpioPortC,
  GpioPortD,
  GpioPortE,
  GpioPortF,
  GpioPortG,
  GpioPortH,
  GpioPortI
} GpioPortNum;
/// Forward decl.
struct GPIO_Type_s;
/// Asociace port Adress a RCC clock
struct GpioAssocPort {
  GPIO_Type_s* portAdr;
  uint32_t      clkMask;
};

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
  * TODO : V Konstruktoru přidat port, ale to je zásah i do ostatních platform.
*/
class GpioClass {
  public:
    /** Konstruktor
    @param port GpioPortA | GpioPortB | GpioPortC | GpioPortD | GpioPortF
    @param no   číslo pinu na portu
    @param type IN, OUT, AF, AN default OUT 
    */
    GpioClass (const uint32_t no, const GPIOMode_TypeDef type = GPIO_FMode_OUT);
    /// Nastav pin @param b na tuto hodnotu
    void set (const bool b) const;
    /// Nastav pin na log. H
    void set (void) const;
    /// Nastav pin na log. L
    void clr (void) const;
    /// Změň hodnotu pinu
    void change (void) const;
    /// Načti logickou hodnotu na pinu
    const bool get (void) const;
    void setDir (GPIODir_TypeDef p);
    
    void setMode (GPIOMode_TypeDef p);
    void setOType (GPIOOType_TypeDef p);
    void setSpeed (GPIOSpeed_TypeDef p);
    void setPuPd (GPIOPuPd_TypeDef p);
    void setAF (unsigned af);
  private:
    /// Port.
    GPIO_Type_s* const io;
    /// A pozice pinu na něm, stačí 16.bit
    const uint16_t      pos;
    /// pro funkce setXXX necháme i číslo pinu
    const uint16_t      num;
  
};

#endif // GPIO_H
