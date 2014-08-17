#ifndef STM32F4_H
#define STM32F4_H

#include "cortexmx.h"
#include "command.h"
#include "commandset.h"


class STM32F4 : public CortexMx {
  public:
    STM32F4 (GdbServer* s, const char* name);
   ~STM32F4 ();
    bool cmd_erase_mass (void);
    bool cmd_option     (int argc, const char *argv[]);
    /* Flash memory access functions */
    int flash_erase     (uint32_t addr, int len);
    int flash_write     (uint32_t dest, const uint8_t *src, int len);
    
    int  Handler        (int argc, const char* argv[]);
    bool probe          (void);
  protected:
    void        flash_unlock            (void);
    bool        option_write            (uint32_t value);

  private:
    Command     cErase, cOption;
    
    uint32_t    pagesize;
};

#endif // STM32F4_H
