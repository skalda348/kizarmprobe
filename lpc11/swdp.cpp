#include "swdp.h"
#include "LPC11Uxx.h"

extern "C" void *memcpy (void *dest, const void *src, size_t n);

#undef  debug
#define debug(...)

Swdp::Swdp () : BaseLayer(),
  swdio (GpioPortA, SWDIO_BIT), swclk (GpioPortA, SWCLK_BIT) {
  olddir = false;
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
/************* Member functions ****************************/

void Swdp::turnaround (bool dir) {

  debug ("%s", dir ? "\n-> ":"\n<- ");

  /* Don't turnaround if direction not changing */
  if (dir == olddir) return;
  olddir = dir;

  if (dir) swdio.setDir (GPIO_Mode_IN);
  swclk.set ();
  swclk.clr ();
  if(!dir) swdio.setDir (GPIO_Mode_OUT);
}

bool Swdp::bit_in (void) {
  bool ret = swdio.get();
  swclk.set();
  swclk.clr();

  debug ("%d", ret ? 1:0);

  return ret;
}


void Swdp::bit_out (bool val) {

  debug ("%d", val);

  swdio.set (val);
  swclk.set ();
  swclk.clr ();
}

int Swdp::init (void) {
  /* This must be investigated in more detail.
   * As described in STM32 Reference Manual... */
  reset();
  seq_out(0xE79E, 16); /* 0b0111100111100111 */
  reset();
  seq_out(0, 16);

  return 0;

}
 
void Swdp::reset (void) {
  turnaround (false);
  /* 50 clocks with TMS high */
  for (int i = 0; i < 50; i++) bit_out (true);
}


uint32_t Swdp::seq_in (int ticks) {

  uint32_t index = 1;
  uint32_t ret   = 0;

  turnaround (true);

  while (ticks--) {
    if (bit_in ()) ret |= index;
    index <<= 1;
  }

  return ret;
}

bool Swdp::seq_in_parity (uint32_t* ret, int ticks) {

  uint32_t index  = 1;
  uint32_t parity = 0;
  
  *ret = 0;

  turnaround (true);

  while (ticks--) {
    if (bit_in ()) {
      *ret |= index;
      parity ^= 1;
    }
    index <<= 1;
  }
  if (bit_in()) parity ^= 1;

  return (bool) parity;
}

void Swdp::seq_out (uint32_t MS, int ticks) {

  turnaround (false);

  while(ticks--) {
    bit_out (MS & 1);
    MS >>= 1;
  }
}

void Swdp::seq_out_parity (uint32_t MS, int ticks) {

  uint32_t parity = 0;

  turnaround (false);

  while (ticks--) {
    bit_out (MS & 1);
    parity ^= MS;
    MS >>= 1;
  }
  bit_out (parity & 1);
}

/************* Member functions ****************************/
uint32_t Swdp::swdptap_init       (void) {
  uint32_t CoreId, ack;

  init ();
  /* Read the SW-DP IDCODE register to syncronise */
  /* This could be done with adiv_swdp_low_access(), but this doesn't
   * allow the ack to be checked here. */
  seq_out (0xA5, 8);
  ack = seq_in  (3);
  if ((ack != SWD_ACK) || seq_in_parity (&CoreId, 32)) {
    return 0;
  }
  
  return CoreId;
}
uint8_t Swdp::swdptap_low_access (uint8_t APnDP, uint8_t RnW, uint8_t addr, uint32_t* value) {
  uint8_t  request = 0x81;
  uint8_t  ack;

  if(APnDP) request ^= 0x22;
  if(RnW)   request ^= 0x24;

  addr &= 0xC;
  request |= (addr << 1) & 0x18;
  if((addr == 4) || (addr == 8))
    request ^= 0x20;

  size_t tries = 1000;
  do {
    seq_out (request, 8);
    ack = seq_in     (3);
  } while (--tries && ack == SWD_WAIT);


  if (RnW) {
    if (seq_in_parity  (value, 32)) {
      /* Give up on parity error */
      ack = SWD_PARITY;
    }
  } else {
    seq_out_parity (*value, 32);
  }

  /* REMOVE THIS */
  seq_out (0, 8);

  return ack;
}

