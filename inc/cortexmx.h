#ifndef CORTEXMX_H
#define CORTEXMX_H

#include "target.h"
#include "command.h"
#include "commandset.h"

#define CORTEXM_MAX_WATCHPOINTS 4       /* architecture says up to 15, no implementation has > 4 */
#define CORTEXM_MAX_BREAKPOINTS 6       /* architecture says up to 127, no implementation has > 6 */

struct cortexm_priv {
  bool stepping;
  bool on_bkpt;
  /* Watchpoint unit status */
  struct wp_unit_s {
          uint32_t addr;
          uint8_t  type;
          uint8_t  size;
  } hw_watchpoint[CORTEXM_MAX_WATCHPOINTS];
  unsigned hw_watchpoint_max;
  /* Breakpoint unit status */
  uint32_t hw_breakpoint[CORTEXM_MAX_BREAKPOINTS];
  unsigned hw_breakpoint_max;
  /* Copy of DEMCR for vector-catch */
  uint32_t demcr;
  /* Semihosting state */
  uint32_t syscall;
  uint32_t errno;
  uint32_t byte_count;
};

class CortexMx : public Target, public CommandSet {

  public:
    CortexMx (GdbServer*s,  const char* name);

    virtual bool probe  (void);
    void remove         (void);
    const char* getName (void);
    bool vector_catch   (int argc, const char *argv[]);
    
    bool attach         (void);
    void detach         (void);

    int  regs_read      (void       *data);
    int  regs_write     (const void *data);
    int  pc_write       (const uint32_t val);
    uint32_t pc_read    (void);

    void reset          (void);
    void halt_resume    (bool step);
    int  halt_wait      (void);
    void halt_request   (void);
    
    int  fault_unwind   (void);

    int  set_hw_bp      (uint32_t addr);
    int  clear_hw_bp    (uint32_t addr);

    int  set_hw_wp      (uint8_t type, uint32_t addr, uint8_t len);
    int  clear_hw_wp    (uint8_t type, uint32_t addr, uint8_t len);

    int  check_hw_wp    (uint32_t *addr);
    
    int  hostio_request (void);
    void hostio_reply   (int32_t retcode, uint32_t errcode);
    
    virtual int  Handler  (int argc, const char* argv[]);
  private:
    Command      cCatch;
    cortexm_priv priv;
};

#endif // CORTEXMX_H
