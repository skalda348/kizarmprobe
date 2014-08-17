#ifndef SWDP_H
#define SWDP_H

#include "baselayer.h"

// GPIO pins used for SWD
#define SWCLK_BIT 18
#define SWDIO_BIT 19

#define SWCLK_BIT_MSK (1<<SWCLK_BIT)
#define SWDIO_BIT_MSK (1<<SWDIO_BIT)

// SWD status responses. SWD_ACK is good.
#define SWD_ACK 0b001
#define SWD_WAIT 0b010
#define SWD_FAULT 0b100
#define SWD_PARITY 0b1000

enum swdCommands {
  cmdInit         ,
  cmdLowAccess    ,
};

struct swdPacket {
  enum swdCommands    cmd : 8;
  uint8_t             APnDP;
  uint8_t             RnW;
  uint8_t             adr;
  uint32_t            val;
}__attribute__((packed));
typedef struct swdPacket swdPacket_t;

class Swdp : public BaseLayer {

  public:
    Swdp ();
    void     Fini       (void);
    uint32_t Up         (char* data, uint32_t len);
  protected:
    uint32_t Init               (void);
    uint32_t swdptap_init       (void);
    uint8_t  swdptap_low_access (uint8_t APnDP, uint8_t RnW, uint8_t addr, uint32_t* value);
  private:
    swdPacket   UxD;
};

#endif // SWDP_H
