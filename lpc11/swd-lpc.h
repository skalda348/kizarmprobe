/*
<Copyright 2011 mrd>

Usage of the works is permitted provided that this instrument is retained with the works, so that any entity that uses the works is notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#include <stdint.h>

// GPIO pins used for SWD
#define SWCLK_BIT 18
#define SWDIO_BIT 19

#define SWCLK_BIT_MSK (1<<SWCLK_BIT)
#define SWDIO_BIT_MSK (1<<SWDIO_BIT)

// Bit delay
#define BIT_DELAY 1

// Send magic number to switch to SWD mode. This function sends many
// zeros, 1s, then the magic number, then more 1s and zeros to try to
// get the SWD state machine's attention if it is connected in some
// unusual state.
void swd_enable(void);
void swd_flush(void);

// SWD status responses. SWD_ACK is good.
#define SWD_ACK 0b001
#define SWD_WAIT 0b010
#define SWD_FAULT 0b100
#define SWD_PARITY 0b1000

// This is one of the two core SWD functions. It can write to a debug port
// register or to an access port register. It implements the host->target
// write request, reading the 3-bit status, and then writing the 33 bits
// data+parity.
uint8_t swd_write(uint8_t APnDP, uint8_t A, uint32_t data);

// This is one of the two core SWD functions. It can read from a debug port
// register or an access port register. It implements the host->target
// read request, reading the 3-bit status, and then reading the 33 bits
// data+parity.
uint8_t swd_read (uint8_t APnDP, uint8_t A, volatile uint32_t *data);
// Kompatibita s Blackmagic - read i write
uint8_t swd_low_access (uint8_t APnDP, uint8_t RnW, uint8_t addr, uint32_t* value);

