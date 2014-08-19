#ifndef CORTEXMX_H
#define CORTEXMX_H
/**
 * @file
 * @brief Části targetu společné pro všechny Cortex-M procesory.
 * */
#include "target.h"
#include "command.h"
#include "commandset.h"

#define CORTEXM_MAX_WATCHPOINTS 4       //!< architecture says up to 15, no implementation has > 4
#define CORTEXM_MAX_BREAKPOINTS 6       //!< architecture says up to 127, no implementation has > 6
/// Tohle je celé z Black Magic, blíže nekomentuji
struct cortexm_priv {
  bool stepping;
  bool on_bkpt;
  /// Watchpoint unit status
  struct wp_unit_s {
          uint32_t addr;
          uint8_t  type;
          uint8_t  size;
  } hw_watchpoint[CORTEXM_MAX_WATCHPOINTS];
  unsigned hw_watchpoint_max;
  /// Breakpoint unit status
  uint32_t hw_breakpoint[CORTEXM_MAX_BREAKPOINTS];
  unsigned hw_breakpoint_max;
  /// Copy of DEMCR for vector-catch
  uint32_t demcr;
  /// Semihosting state
  uint32_t syscall;
  uint32_t errno;
  uint32_t byte_count;
};

/**
 * @brief Části targetu společné pro všechny Cortex-M procesory.
 * Dědí jednak Target a jeho metody (většinou pure virtual), jednak sadu příkazů
 * CommandSet, kterou by bylo možná lépe zapouzdřit jako data. Takhle jí pak
 * používá i nadřazený Target.
 * 
 * Možná by bylo lepší sem z třídy Target přenést úplně vše a nechat jí jako
 * čistou bázovou třídu (nebo vynechat úplně), ale už to takto nechám.
 * Je v tom sice o něco větší guláš, ale pořád menší než v C.
 * */
class CortexMx : public Target, public CommandSet {

  public:
    /**
     * @brief Konstruktor.
     * I zde je potřeba zpětný přístup na GdbServer.
     * @param s ukazatel na GdbServer
     * @param name I tento target má své jméno
     **/
    CortexMx (GdbServer * s,  const char* name);
    /// Test, zda je jádro připojeno - jen na začátku (monitor scan)
    virtual bool probe  (void);
    /// Vyjmutí sady příkazů - dost důležité používat, pokud probe vrátí false
    void remove         (void);
    /// Vrátí privátní jméno jádra
    const char* getName (void);
    /// převzato z black magic
    bool vector_catch   (int argc, const char *argv[]);
    /// Připoj target ke gdb
    bool attach         (void);
    /// Odpoj target od gdb
    void detach         (void);
    /// načti registry targetu
    int  regs_read      (void       *data);
    /// zapiš zpět registry targetu
    int  regs_write     (const void *data);
    /// zapiš program counter do targetu
    int  pc_write       (const uint32_t val);
    /// přečti program counter z targetu
    uint32_t pc_read    (void);
    /// zrezetuj target
    void reset          (void);
    /// pokračuj ve vykonávání programu (příp. po instrukcích)
    void halt_resume    (bool step);
    /// target stojí ? (funkce nečeká, jen se ptá)
    int  halt_wait      (void);
    /// příkaz zastav target
    void halt_request   (void);
    /// Jakási obnova ???
    int  fault_unwind   (void);
    /// nastav breakpoint
    int  set_hw_bp      (uint32_t addr);
    /// zruš breakpoint
    int  clear_hw_bp    (uint32_t addr);
    /// nastav wathpoint
    int  set_hw_wp      (uint8_t type, uint32_t addr, uint8_t len);
    /// zruš wathpoint
    int  clear_hw_wp    (uint8_t type, uint32_t addr, uint8_t len);
    /// zjisti stav
    int  check_hw_wp    (uint32_t *addr);
    /// hostio_request
    int  hostio_request (void);
    /// hostio_reply
    void hostio_reply   (int32_t retcode, uint32_t errcode);
    /// handler pro příkaz
    virtual int  Handler  (int argc, const char* argv[]);
  private:
    /// příkaz catch-vector
    Command      cCatch;
    /// privátní data
    cortexm_priv priv;
};

#endif // CORTEXMX_H
