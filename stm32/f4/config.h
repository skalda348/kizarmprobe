#ifndef CONFIG_H
#define CONFIG_H

/** Všechno sedí na portu B.
 * Je to čistě experimentální konfigurace, pro praktické
 * použití se to moc nehodí.
 * */

/// GPIO pins used for SWD - CLK
#define SWCLK_BIT GpioPortB,0
/// GPIO pins used for SWD - DATA
#define SWDIO_BIT GpioPortB,1

#define LEDPIN GpioPortD,12

#endif // CONFIG_H
