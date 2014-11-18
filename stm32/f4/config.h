#ifndef CONFIG_H
#define CONFIG_H

/** Všechno sedí na portu B.
 * Je to čistě experimentální konfigurace, pro praktické
 * použití se to moc nehodí.
 * */

#define SWDPORT GpioPortB

/// GPIO pins used for SWD - CLK
#define SWCLK_BIT 0
/// GPIO pins used for SWD - DATA
#define SWDIO_BIT 1

#define LEDPIN 2

#endif // CONFIG_H
