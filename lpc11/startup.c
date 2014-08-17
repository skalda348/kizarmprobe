//*****************************************************************************
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// USE OF THIS SOFTWARE FOR COMMERCIAL DEVELOPMENT AND/OR EDUCATION IS SUBJECT
// TO A CURRENT END USER LICENSE AGREEMENT (COMMERCIAL OR EDUCATIONAL) WITH
// CODE RED TECHNOLOGIES LTD.
//
//*****************************************************************************
#if defined (__cplusplus)
extern "C" {
  extern void __libc_init_array (void);
}
#endif

#define WEAK __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((weak, alias (#f)))

#include "LPC11Uxx.h"

//*****************************************************************************
#if defined (__cplusplus)
extern "C" {
#endif

/// V linker skriptu
extern void (*__init_array_start)();
extern void (*__init_array_end)  ();
/// Inicializace statických konstruktorů - použít explicitně po celkové inicializaci systému
static inline void static_init() {
  void (**p)();
  // Tohle je sice konstrukce značně nepřehledná (nalezená na webu), leč funguje.
  for (p = &__init_array_start; p < &__init_array_end; p++) (*p)();
}


//*****************************************************************************
//
// Forward declaration of the default handlers. These are aliased.
// When the application defines a handler (with the same name), this will
// automatically take precedence over these weak definitions
//
//*****************************************************************************
  void ResetISR          (void);
  void NMI_Handler       (void) ALIAS (IntDefaultHandler);
  void HardFault_Handler (void) ALIAS (IntDefaultHandler);
  void SVC_Handler       (void) ALIAS (IntDefaultHandler);
  void PendSV_Handler    (void) ALIAS (IntDefaultHandler);
  void SysTick_Handler   (void) ALIAS (IntDefaultHandler);
//*****************************************************************************
//
// Forward declaration of the specific IRQ handlers. These are aliased
// to the IntDefaultHandler, which is a 'forever' loop. When the application
// defines a handler (with the same name), this will automatically take
// precedence over these weak definitions
//
//*****************************************************************************

  void FLEX_INT0_IRQHandler (void) ALIAS (IntDefaultHandler);
  void FLEX_INT1_IRQHandler (void) ALIAS (IntDefaultHandler);
  void FLEX_INT2_IRQHandler (void) ALIAS (IntDefaultHandler);
  void FLEX_INT3_IRQHandler (void) ALIAS (IntDefaultHandler);
  void FLEX_INT4_IRQHandler (void) ALIAS (IntDefaultHandler);
  void FLEX_INT5_IRQHandler (void) ALIAS (IntDefaultHandler);
  void FLEX_INT6_IRQHandler (void) ALIAS (IntDefaultHandler);
  void FLEX_INT7_IRQHandler (void) ALIAS (IntDefaultHandler);
  void GINT0_IRQHandler     (void) ALIAS (IntDefaultHandler);
  void GINT1_IRQHandler     (void) ALIAS (IntDefaultHandler);
  void SSP1_IRQHandler      (void) ALIAS (IntDefaultHandler);
  void I2C_IRQHandler       (void) ALIAS (IntDefaultHandler);
  void TIMER16_0_IRQHandler (void) ALIAS (IntDefaultHandler);
  void TIMER16_1_IRQHandler (void) ALIAS (IntDefaultHandler);
  void TIMER32_0_IRQHandler (void) ALIAS (IntDefaultHandler);
  void TIMER32_1_IRQHandler (void) ALIAS (IntDefaultHandler);
  void SSP0_IRQHandler      (void) ALIAS (IntDefaultHandler);
  void UART_IRQHandler      (void) ALIAS (IntDefaultHandler);
  void USB_IRQHandler       (void) ALIAS (IntDefaultHandler);
  void USB_FIQHandler       (void) ALIAS (IntDefaultHandler);
  void ADC_IRQHandler       (void) ALIAS (IntDefaultHandler);
  void WDT_IRQHandler       (void) ALIAS (IntDefaultHandler);
  void BOD_IRQHandler       (void) ALIAS (IntDefaultHandler);
  void FMC_IRQHandler       (void) ALIAS (IntDefaultHandler);
  void USBWakeup_IRQHandler (void) ALIAS (IntDefaultHandler);

//*****************************************************************************
// main() is the entry point for Newlib based applications
//
//*****************************************************************************
  extern int main (void);
//*****************************************************************************
//
// External declaration for the pointer to the stack top from the Linker Script
//
//*****************************************************************************
  typedef void (*pHandler)    (void);
  extern  void _vStackTop     (void);
  extern  void VectorCheckSum (void);

//*****************************************************************************
#if defined (__cplusplus)
} // extern "C"
#endif
//*****************************************************************************
//
// The vector table.  Note that the proper constructs must be placed on this to
// ensure that it ends up at physical address 0x0000.0000.
//
//*****************************************************************************
extern pHandler const g_pfnVectors[];
__attribute__ ( (section (".isr_vector")))
pHandler const g_pfnVectors[] = {
  _vStackTop,        // The initial stack pointer
  ResetISR,                         // The reset handler
  NMI_Handler,                      // The NMI handler
  HardFault_Handler,                // The hard fault handler
  0,                                // Reserved
  0,                                // Reserved
  0,                                // Reserved
  VectorCheckSum,                   // Reserved <- číslo zde musí být vypočteno
  // tak, aby součet předchozích, včetně tohoto byl celkem nulový
  0,                                // Reserved
  0,                                // Reserved
  0,                                // Reserved
  SVC_Handler,                   // SVCall handler
  0,                                // Reserved
  0,                                // Reserved
  PendSV_Handler,                   // The PendSV handler
  SysTick_Handler,                  // The SysTick handler

  // LPC11U specific handlers
  FLEX_INT0_IRQHandler,             //  0 - GPIO pin interrupt 0
  FLEX_INT1_IRQHandler,             //  1 - GPIO pin interrupt 1
  FLEX_INT2_IRQHandler,             //  2 - GPIO pin interrupt 2
  FLEX_INT3_IRQHandler,             //  3 - GPIO pin interrupt 3
  FLEX_INT4_IRQHandler,             //  4 - GPIO pin interrupt 4
  FLEX_INT5_IRQHandler,             //  5 - GPIO pin interrupt 5
  FLEX_INT6_IRQHandler,             //  6 - GPIO pin interrupt 6
  FLEX_INT7_IRQHandler,             //  7 - GPIO pin interrupt 7
  GINT0_IRQHandler,                 //  8 - GPIO GROUP0 interrupt
  GINT1_IRQHandler,                 //  9 - GPIO GROUP1 interrupt
  0,                                // 10 - Reserved
  0,                                // 11 - Reserved
  0,                                // 12 - Reserved
  0,                                // 13 - Reserved
  SSP1_IRQHandler,                  // 14 - SPI/SSP1 Interrupt
  I2C_IRQHandler,                   // 15 - I2C0
  TIMER16_0_IRQHandler,             // 16 - CT16B0 (16-bit Timer 0)
  TIMER16_1_IRQHandler,             // 17 - CT16B1 (16-bit Timer 1)
  TIMER32_0_IRQHandler,             // 18 - CT32B0 (32-bit Timer 0)
  TIMER32_1_IRQHandler,             // 19 - CT32B1 (32-bit Timer 1)
  SSP0_IRQHandler,                  // 20 - SPI/SSP0 Interrupt
  UART_IRQHandler,                  // 21 - UART0
  USB_IRQHandler,                   // 22 - USB IRQ
  USB_FIQHandler,                   // 23 - USB FIQ
  ADC_IRQHandler,                   // 24 - ADC (A/D Converter)
  WDT_IRQHandler,                   // 25 - WDT (Watchdog Timer)
  BOD_IRQHandler,                   // 26 - BOD (Brownout Detect)
  FMC_IRQHandler,                   // 27 - IP2111 Flash Memory Controller
  0,                                // 28 - Reserved
  0,                                // 29 - Reserved
  USBWakeup_IRQHandler,             // 30 - USB wake-up interrupt
  0,                                // 31 - Reserved
};
// Definováno v linker skriptu
extern unsigned int _sidata;
extern unsigned int _data;
extern unsigned int _edata;
extern unsigned int _bss;
extern unsigned int _ebss;


//*****************************************************************************
// Reset entry point for your code.
// Sets up a simple runtime environment and initializes the C/C++
// library.
//*****************************************************************************
__attribute__ ( (section (".after_vectors")))
void ResetISR (void) {

  unsigned int *src, *dst;

  // Copy data section from flash to RAM
  src = &_sidata;
  dst = &_data;
  while (dst < &_edata) *dst++ = *src++;

  // Zero fill the bss section
  dst = &_bss;
  while (dst <  &_ebss) *dst++ = 0;

  SystemInit();
  // Načti skutečnou hodnotu proměnné SystemCoreClock
  SystemCoreClockUpdate ();
  static_init();

  main();
  // main() shouldn't return, but if it does, we'll just enter an infinite loop
  for (;;);
}

//*****************************************************************************
//
// Processor ends up here if an unexpected interrupt occurs or a specific
// handler is not present in the application code.
//
//*****************************************************************************
__attribute__ ( (section (".after_vectors")))
void IntDefaultHandler (void) {
  while (1) {
  }
}

// Většina IO není vůbec použita a protože to, co použito je, je skoro vše právě v main
// optimalizace to stejně odstraní, takže kód neroste.
LPC_I2C_Type            * const LPC_I2C            = ( (LPC_I2C_Type            *) LPC_I2C_BASE);
LPC_WWDT_Type           * const LPC_WWDT           = ( (LPC_WWDT_Type           *) LPC_WWDT_BASE);
LPC_USART_Type          * const LPC_USART          = ( (LPC_USART_Type          *) LPC_USART_BASE);
LPC_CTxxB0_Type         * const LPC_CT16B0         = ( (LPC_CTxxB0_Type         *) LPC_CT16B0_BASE);
LPC_CTxxB1_Type         * const LPC_CT16B1         = ( (LPC_CTxxB1_Type         *) LPC_CT16B1_BASE);
LPC_CTxxB0_Type         * const LPC_CT32B0         = ( (LPC_CTxxB0_Type         *) LPC_CT32B0_BASE);
LPC_CTxxB1_Type         * const LPC_CT32B1         = ( (LPC_CTxxB1_Type         *) LPC_CT32B1_BASE);
LPC_ADC_Type            * const LPC_ADC            = ( (LPC_ADC_Type            *) LPC_ADC_BASE);
LPC_PMU_Type            * const LPC_PMU            = ( (LPC_PMU_Type            *) LPC_PMU_BASE);
LPC_FLASHCTRL_Type      * const LPC_FLASHCTRL      = ( (LPC_FLASHCTRL_Type      *) LPC_FLASHCTRL_BASE);
LPC_SSPx_Type           * const LPC_SSP0           = ( (LPC_SSPx_Type           *) LPC_SSP0_BASE);
LPC_SSPx_Type           * const LPC_SSP1           = ( (LPC_SSPx_Type           *) LPC_SSP1_BASE);
LPC_IOCON_Type          * const LPC_IOCON          = ( (LPC_IOCON_Type          *) LPC_IOCON_BASE);
LPC_SYSCON_Type         * const LPC_SYSCON         = ( (LPC_SYSCON_Type         *) LPC_SYSCON_BASE);
LPC_GPIO_PIN_INT_Type   * const LPC_GPIO_PIN_INT   = ( (LPC_GPIO_PIN_INT_Type   *) LPC_GPIO_PIN_INT_BASE);
LPC_GPIO_GROUP_INTx_Type* const LPC_GPIO_GROUP_INT0= ( (LPC_GPIO_GROUP_INTx_Type*) LPC_GPIO_GROUP_INT0_BASE);
LPC_GPIO_GROUP_INTx_Type* const LPC_GPIO_GROUP_INT1= ( (LPC_GPIO_GROUP_INTx_Type*) LPC_GPIO_GROUP_INT1_BASE);
LPC_USB_Type            * const LPC_USB            = ( (LPC_USB_Type            *) LPC_USB_BASE);
LPC_GPIO_Type           * const LPC_GPIO           = ( (LPC_GPIO_Type           *) LPC_GPIO_BASE);

