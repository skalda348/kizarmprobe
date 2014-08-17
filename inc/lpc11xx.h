#ifndef LPC11XX_H
#define LPC11XX_H

#include "cortexmx.h"

#define IAP_PGM_CHUNKSIZE 256 /* should fit in RAM on any device */

struct flash_param {
  uint16_t  opcodes[2]; /* two opcodes to return to after calling the ROM */
  uint32_t  command[5]; /* command operands */
  uint32_t  result [4]; /* result data */
};

struct flash_program {
  struct flash_param  p;
  uint8_t     data [IAP_PGM_CHUNKSIZE];
};


class LPC11XX : public CortexMx {
  public:
    LPC11XX (GdbServer* s, const char* name);
   ~LPC11XX ();
    /* Flash memory access functions */
    int flash_erase     (uint32_t addr, int len);
    int flash_write     (uint32_t dest, const uint8_t *src, int len);
    
    bool probe          (void);
  protected:
    void iap_call       (unsigned param_len);
    int  flash_prepare  (uint32_t addr, int len);
  private:
    flash_program flash_pgm;
};

#endif // LPC11XX_H
