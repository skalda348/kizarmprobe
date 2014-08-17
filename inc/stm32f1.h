#ifndef STM32F1_H
#define STM32F1_H

#include "cortexmx.h"
#include "command.h"
#include "commandset.h"


class STM32F1 : public CortexMx {
  public:
    STM32F1 (GdbServer* s, const char* name);
   ~STM32F1 ();
    bool cmd_erase_mass (void);
    bool cmd_option     (int argc, const char *argv[]);
    /* Flash memory access functions */
    int flash_erase     (uint32_t addr, int len);
    int flash_write     (uint32_t dest, const uint8_t *src, int len);
    
    int  Handler        (int argc, const char* argv[]);
    bool probe          (void);
  protected:
    void        flash_unlock            (void);
    bool        option_erase            (void);
    bool        option_write_erased     (uint32_t addr, uint16_t value);
    bool        option_write            (uint32_t addr, uint16_t value);

  private:
    Command     cErase, cOption;
    
    uint32_t    pagesize;
};

#endif // STM32F1_H
