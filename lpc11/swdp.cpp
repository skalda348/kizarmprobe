#include "swdp.h"
#include "LPC11Uxx.h"

extern "C" void *memcpy (void *dest, const void *src, size_t n);

#undef  debug
#define debug(...)

Swdp::Swdp () : BaseLayer() {

}
void Swdp::Fini (void) {
}
uint32_t Swdp::Up (char *data, uint32_t len) {
  if (!len) {
    return swdptap_init();
  }
  memcpy (&UxD, data, len);
  uint8_t res = swdptap_low_access (UxD.APnDP, UxD.RnW, UxD.adr, & UxD.val);
  UxD.RnW = res;
  memcpy (data, &UxD, len);
  return len;
}
uint32_t Swdp::Init (void) {
  return 0;
}
// Bit delay
#define BIT_DELAY 1
static inline void bit_delay (void) {
  volatile char i = BIT_DELAY;
  while (i--);
}

char HostBus = 0; // SWD bus is controlled by the host or target

// Switch to host-controlled bus
//__attribute__ ((section(".ramFunc")))
static void swd_turnaround_host() {
  if (!HostBus) { // clock is idle (low)
    bit_delay();
    // Set up 11U port bitmask so we can write data + clock
    // at the same time. 1s mean ignore this bit.
    LPC_GPIO->MASK[0] = ~ (SWDIO_BIT_MSK | SWCLK_BIT_MSK);
    // Drive clock high and set SWDIO
    LPC_GPIO->MPIN[0] =    SWDIO_BIT_MSK | SWCLK_BIT_MSK;
    // Take over SWDIO pin
    LPC_GPIO->DIR[0] |=    SWDIO_BIT_MSK | SWCLK_BIT_MSK;

    HostBus = 1;
    // clock is idle (high)
  }
}

// Switch to target-controlled bus
//__attribute__ ((section(".ramFunc")))
static void swd_turnaround_target() {
  if (HostBus) { // clock is idle (high)
    bit_delay();
    // Set up 11U port bitmask so we can write data + clock
    // at the same time. 1s mean ignore this bit.
    LPC_GPIO->MASK[0] = ~ (SWDIO_BIT_MSK | SWCLK_BIT_MSK);
    // Drive clock low and set SWDIO
    LPC_GPIO->MPIN[0] = SWDIO_BIT_MSK;

    bit_delay();
    // Drive clock high
    LPC_GPIO->SET[0] = SWCLK_BIT_MSK;
    bit_delay();
    // Drive clock low
    LPC_GPIO->CLR[0] = SWCLK_BIT_MSK;

    // Release SWDIO pin
    LPC_GPIO->DIR[0] &= ~SWDIO_BIT_MSK; // tristate data pin

    HostBus = 0;
  }
}

// Sends bits out the wire. This is a low-level function that does not
// implement any SWD protocol. The bits are clocked out on the clock falling
// edge for the SWD state machine to latch on the rising edge. After it
// is done this function leaves the clock low.
//__attribute__ ((section(".ramFunc")))
static void write_bits (int nbits, const unsigned long *bits) {
  int i;
  unsigned long wbuf = *bits;

  swd_turnaround_host();
  // clock is idle (high)

  // Set up 11U port bitmask so we can write data + clock
  // at the same time. Done in swd_turnaround_host.
  //LPC_GPIO->MASK[0] = ~(SWDIO_BIT_MSK | SWCLK_BIT_MSK);

  i = 0;
  while (nbits) {
    bit_delay();

    // drive SWCLK low and output next bit to SWDIO line
    LPC_GPIO->MPIN[0] = ( ( (wbuf) & 1) << SWDIO_BIT);
    bit_delay();

    // prepare for next bit
    nbits--;
    i++;
    if (i >= 32)
      bits++, wbuf = *bits, i = 0;
    else
      (wbuf) >>= 1;

    // drive SWCLK high
    LPC_GPIO->SET[0] = SWCLK_BIT_MSK;
  }
}

// Reads bits from the wire. This is a low-level function that does not
// implement any SWD protocol. The bits are expected to be clocked out on
// the clock rising edge and then they are latched on the clock falling edge.
// After it is done this function leaves the clock low.
//__attribute__ ((section(".ramFunc")))
static void read_bits (int nbits, volatile unsigned long *bits) {
  int i;

  swd_turnaround_target();
  // clock is idle (low)

  *bits = 0;
  i = 0;
  while (nbits) {
    bit_delay();
    // read bit from SWDIO line
    (*bits) |= ( (LPC_GPIO->PIN[0] >> SWDIO_BIT) & 1) << i;

    // drive SWCLK high
    LPC_GPIO->SET[0] = SWCLK_BIT_MSK;

    bit_delay();

    // drive SWCLK low
    LPC_GPIO->CLR[0] = SWCLK_BIT_MSK;

    // prepare for next bit
    i++;
    if (i >= 32) {
      bits++;
      if (nbits > 1)
        *bits = 0; // prevent hard faults
      i = 0;
    }
    nbits--;
  }
}

// Send 8 0 bits to flush SWD state
static inline void swd_flush (void) {
  unsigned long data = 0;

  write_bits (8, &data);
}

// Send magic number to switch to SWD mode. This function sends many
// zeros, 1s, then the magic number, then more 1s and zeros to try to
// get the SWD state machine's attention if it is connected in some
// unusual state.
static void swd_enable (void) {
  unsigned long data;

  // write 0s
  data = 0;
  write_bits (32, &data);
  write_bits (32, &data);
  write_bits (32, &data);
  write_bits (32, &data);

  // write FFs
  data = 0xFFFFFFFF;
  write_bits (32, &data);
  write_bits (32, &data);
  write_bits (32, &data);
  write_bits (32, &data);

  data = 0xE79E;
  write_bits (16, &data);

  // write FFs
  data = 0xFFFFFFFF;
  write_bits (32, &data);
  write_bits (32, &data);
  write_bits (32, &data);
  write_bits (32, &data);

  // write 0s
  data = 0;
  write_bits (32, &data);
  write_bits (32, &data);
  write_bits (32, &data);
  write_bits (32, &data);
}

// The SWD protocol consists of read and write requests sent from
// the host in the form of 8-bit packets. These requests are followed
// by a 3-bit status response from the target and then a data phase
// which is 32 bits of data + 1 bit of parity. This function reads
// the three bit status responses.
//__attribute__ ((section(".ramFunc")))
static int swd_get_target_response (void) {
  unsigned long data;

  read_bits (3, &data);

  return data;
}

// This function counts the number of 1s in an integer. It is used
// to calculate parity. This is the MIT HAKMEM (Hacks Memo) implementation.
//__attribute__ ((section(".ramFunc")))
static int count_ones (unsigned long n) {
  register unsigned int tmp;

  tmp = n - ( (n >> 1) & 033333333333)
        - ( (n >> 2) & 011111111111);
  return ( (tmp + (tmp >> 3)) & 030707070707) % 63;
}
#if 0
// This is one of the two core SWD functions. It can write to a debug port
// register or to an access port register. It implements the host->target
// write request, reading the 3-bit status, and then writing the 33 bits
// data+parity.
static uint8_t swd_write (uint8_t APnDP, uint8_t A, uint32_t data) {
  unsigned long wpr;
  uint8_t response;

  wpr = 0b10000001 | (APnDP << 1) | (A << 1); // A is a 32-bit address bits 0,1 are 0.
  if (count_ones (wpr&0x1E) % 2) // odd number of 1s
    wpr |= 1 << 5; // set parity bit
  write_bits (8, &wpr);
  // now read acknowledgement
  response = swd_get_target_response();
  if (response != SWD_ACK) {
    swd_turnaround_host();
    return response;
  }

  write_bits (32, &data); // send write data
  wpr = 0;
  if (count_ones (data) % 2) // odd number of 1s
    wpr = 1;   // set parity bit
  write_bits (1, &wpr); // send parity

  return SWD_ACK;
}
#endif
// This is one of the two core SWD functions. It can read from a debug port
// register or an access port register. It implements the host->target
// read request, reading the 3-bit status, and then reading the 33 bits
// data+parity.
static uint8_t swd_read (uint8_t APnDP, uint8_t A, volatile uint32_t *data) {
  unsigned long wpr;
  uint8_t response;

  wpr = 0b10000101 | (APnDP << 1) | (A << 1); // A is a 32-bit address bits 0,1 are 0.
  if (count_ones (wpr&0x1E) % 2) // odd number of 1s
    wpr |= 1 << 5; // set parity bit
  write_bits (8, &wpr);
  // now read acknowledgement
  response = swd_get_target_response();
  if (response != SWD_ACK) {
    swd_turnaround_host();
    return response;
  }

  read_bits (32, data); // read data
  read_bits (1, &wpr);  // read parity
  if (count_ones (wpr) % 2) { // odd number of 1s
    if (wpr != 1) {
      swd_turnaround_host();
      return SWD_PARITY; // bad parity
    }
  } else {
    if (wpr != 0) {
      swd_turnaround_host();
      return SWD_PARITY; // bad parity
    }
  }

  swd_turnaround_host();
  return SWD_ACK;
}
/* ******** blackmagic compatibility ***********************/
static uint8_t ReadDP (uint8_t address, uint32_t *data) {
  uint8_t status;

  do {
    status = swd_read (0, address, (unsigned long *) data);
  } while (status == SWD_WAIT);
  return status;
}

/************* Member functions ****************************/
uint32_t Swdp::swdptap_init       (void) {
  uint32_t CoreId;
  uint8_t  res;
  swd_enable();
  res = ReadDP (0, &CoreId);
  if (res != SWD_ACK) CoreId = 0;
  return CoreId;
}
uint8_t Swdp::swdptap_low_access (uint8_t APnDP, uint8_t RnW, uint8_t addr, uint32_t* value) {
  uint32_t request = 0x81;
  uint32_t wpr;
  uint8_t  ack;

  if(APnDP) request ^= 0x22;
  if(RnW)   request ^= 0x24;

  addr &= 0xC;
  request |= (addr << 1) & 0x18;
  if((addr == 4) || (addr == 8))
    request ^= 0x20;

  uint32_t tries = 100;
  do {
    write_bits (8, &request);
    ack = swd_get_target_response();
  } while(--tries && ack == SWD_WAIT);

  if(ack != SWD_ACK) {
    swd_turnaround_host();
    return ack;
  }

  if(RnW) {
    read_bits (32, value); // read data
    read_bits (1, &wpr);   // read parity
    if (count_ones (*value) % 2) { // odd number of 1s
      if (wpr != 1) {
        swd_turnaround_host();
        return SWD_PARITY; // bad parity
      }
    } else {
      if (wpr != 0) {
        swd_turnaround_host();
        return SWD_PARITY; // bad parity
      }
    }

    swd_turnaround_host();
  } else {
    write_bits (32, value);      // send write data
    wpr = 0;
    if (count_ones (*value) % 2) // odd number of 1s
      wpr = 1;                   // set parity bit
    write_bits (1, &wpr);        // send parity
  }

  /* REMOVE THIS 
  swdptap_seq_out(0, 8);*/

  return SWD_ACK;
}

