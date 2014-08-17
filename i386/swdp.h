#ifndef SWDP_H
#define SWDP_H

#include "baselayer.h"

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
    //~Swdp();
    void     Fini       (void);
    uint32_t Up         (char* data, uint32_t len);
  protected:
    uint32_t Init       (void);
    void     Read       (void);
    void     Write      (void);
  private:
    int         dev;
    swdPacket   TxD, RxD;
};

#endif // SWDP_H
