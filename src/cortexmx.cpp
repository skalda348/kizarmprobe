#include <stdio.h>
#include <string.h>
#include "resources.h"
#include "gdbserver.h"
#include "cortexmx.h"
#include "monitor.h"

/* target options recognised by the Cortex-M target */
#define TOPT_FLAVOUR_V6M        (1<<0)  /* if not set, target is assumed to be v7m */
#define TOPT_FLAVOUR_V7MF       (1<<1)  /* if set, floating-point enabled. */

/* Private peripheral bus base address */
#define CORTEXM_PPB_BASE        0xE0000000

#define CORTEXM_SCS_BASE        (CORTEXM_PPB_BASE + 0xE000)

#define CORTEXM_AIRCR           (CORTEXM_SCS_BASE + 0xD0C)
#define CORTEXM_CFSR            (CORTEXM_SCS_BASE + 0xD28)
#define CORTEXM_HFSR            (CORTEXM_SCS_BASE + 0xD2C)
#define CORTEXM_DFSR            (CORTEXM_SCS_BASE + 0xD30)
#define CORTEXM_CPACR           (CORTEXM_SCS_BASE + 0xD88)
#define CORTEXM_DHCSR           (CORTEXM_SCS_BASE + 0xDF0)
#define CORTEXM_DCRSR           (CORTEXM_SCS_BASE + 0xDF4)
#define CORTEXM_DCRDR           (CORTEXM_SCS_BASE + 0xDF8)
#define CORTEXM_DEMCR           (CORTEXM_SCS_BASE + 0xDFC)

#define CORTEXM_FPB_BASE        (CORTEXM_PPB_BASE + 0x2000)

/* ARM Literature uses FP_*, we use CORTEXM_FPB_* consistently */
#define CORTEXM_FPB_CTRL        (CORTEXM_FPB_BASE + 0x000)
#define CORTEXM_FPB_REMAP       (CORTEXM_FPB_BASE + 0x004)
#define CORTEXM_FPB_COMP(i)     (CORTEXM_FPB_BASE + 0x008 + (4*(i)))

#define CORTEXM_DWT_BASE        (CORTEXM_PPB_BASE + 0x1000)

#define CORTEXM_DWT_CTRL        (CORTEXM_DWT_BASE + 0x000)
#define CORTEXM_DWT_COMP(i)     (CORTEXM_DWT_BASE + 0x020 + (0x10*(i)))
#define CORTEXM_DWT_MASK(i)     (CORTEXM_DWT_BASE + 0x024 + (0x10*(i)))
#define CORTEXM_DWT_FUNC(i)     (CORTEXM_DWT_BASE + 0x028 + (0x10*(i)))

/* Application Interrupt and Reset Control Register (AIRCR) */
#define CORTEXM_AIRCR_VECTKEY           (0x05FA << 16)
/* Bits 31:16 - Read as VECTKETSTAT, 0xFA05 */
#define CORTEXM_AIRCR_ENDIANESS         (1 << 15)
/* Bits 15:11 - Unused, reserved */
#define CORTEXM_AIRCR_PRIGROUP          (7 << 8)
/* Bits 7:3 - Unused, reserved */
#define CORTEXM_AIRCR_SYSRESETREQ       (1 << 2)
#define CORTEXM_AIRCR_VECTCLRACTIVE     (1 << 1)
#define CORTEXM_AIRCR_VECTRESET         (1 << 0)

/* HardFault Status Register (HFSR) */
#define CORTEXM_HFSR_DEBUGEVT           (1 << 31)
#define CORTEXM_HFSR_FORCED             (1 << 30)
/* Bits 29:2 - Not specified */
#define CORTEXM_HFSR_VECTTBL            (1 << 1)
/* Bits 0 - Reserved */

/* Debug Fault Status Register (DFSR) */
/* Bits 31:5 - Reserved */
#define CORTEXM_DFSR_RESETALL           0x1F
#define CORTEXM_DFSR_EXTERNAL           (1 << 4)
#define CORTEXM_DFSR_VCATCH             (1 << 3)
#define CORTEXM_DFSR_DWTTRAP            (1 << 2)
#define CORTEXM_DFSR_BKPT               (1 << 1)
#define CORTEXM_DFSR_HALTED             (1 << 0)

/* Debug Halting Control and Status Register (DHCSR) */
/* This key must be written to bits 31:16 for write to take effect */
#define CORTEXM_DHCSR_DBGKEY            0xA05F0000
/* Bits 31:26 - Reserved */
#define CORTEXM_DHCSR_S_RESET_ST        (1 << 25)
#define CORTEXM_DHCSR_S_RETIRE_ST       (1 << 24)
/* Bits 23:20 - Reserved */
#define CORTEXM_DHCSR_S_LOCKUP          (1 << 19)
#define CORTEXM_DHCSR_S_SLEEP           (1 << 18)
#define CORTEXM_DHCSR_S_HALT            (1 << 17)
#define CORTEXM_DHCSR_S_REGRDY          (1 << 16)
/* Bits 15:6 - Reserved */
#define CORTEXM_DHCSR_C_SNAPSTALL       (1 << 5)        /* v7m only */
/* Bit 4 - Reserved */
#define CORTEXM_DHCSR_C_MASKINTS        (1 << 3)
#define CORTEXM_DHCSR_C_STEP            (1 << 2)
#define CORTEXM_DHCSR_C_HALT            (1 << 1)
#define CORTEXM_DHCSR_C_DEBUGEN         (1 << 0)

/* Debug Core Register Selector Register (DCRSR) */
#define CORTEXM_DCRSR_REGWnR            0x00010000
#define CORTEXM_DCRSR_REGSEL_MASK       0x0000001F
#define CORTEXM_DCRSR_REGSEL_XPSR       0x00000010
#define CORTEXM_DCRSR_REGSEL_MSP        0x00000011
#define CORTEXM_DCRSR_REGSEL_PSP        0x00000012

/* Debug Exception and Monitor Control Register (DEMCR) */
/* Bits 31:25 - Reserved */
#define CORTEXM_DEMCR_TRCENA            (1 << 24)
/* Bits 23:20 - Reserved */
#define CORTEXM_DEMCR_MON_REQ           (1 << 19)       /* v7m only */
#define CORTEXM_DEMCR_MON_STEP          (1 << 18)       /* v7m only */
#define CORTEXM_DEMCR_VC_MON_PEND       (1 << 17)       /* v7m only */
#define CORTEXM_DEMCR_VC_MON_EN         (1 << 16)       /* v7m only */
/* Bits 15:11 - Reserved */
#define CORTEXM_DEMCR_VC_HARDERR        (1 << 10)
#define CORTEXM_DEMCR_VC_INTERR         (1 << 9)        /* v7m only */
#define CORTEXM_DEMCR_VC_BUSERR         (1 << 8)        /* v7m only */
#define CORTEXM_DEMCR_VC_STATERR        (1 << 7)        /* v7m only */
#define CORTEXM_DEMCR_VC_CHKERR         (1 << 6)        /* v7m only */
#define CORTEXM_DEMCR_VC_NOCPERR        (1 << 5)        /* v7m only */
#define CORTEXM_DEMCR_VC_MMERR          (1 << 4)        /* v7m only */
/* Bits 3:1 - Reserved */
#define CORTEXM_DEMCR_VC_CORERESET      (1 << 0)

/* Flash Patch and Breakpoint Control Register (FP_CTRL) */
/* Bits 32:15 - Reserved    */
/* Bits 14:12 - NUM_CODE2   */       /* v7m only */
/* Bits 11:8  - NUM_LIT     */       /* v7m only */
/* Bits 7:4   - NUM_CODE1   */
/* Bits 3:2   - Unspecified */
#define CORTEXM_FPB_CTRL_KEY            (1 << 1)
#define CORTEXM_FPB_CTRL_ENABLE         (1 << 0)

/* Data Watchpoint and Trace Mask Register (DWT_MASKx) */
#define CORTEXM_DWT_MASK_BYTE           (0 << 0)
#define CORTEXM_DWT_MASK_HALFWORD       (1 << 0)
#define CORTEXM_DWT_MASK_WORD           (3 << 0)

/* Data Watchpoint and Trace Function Register (DWT_FUNCTIONx) */
#define CORTEXM_DWT_FUNC_MATCHED        (1 << 24)
#define CORTEXM_DWT_FUNC_DATAVSIZE_WORD (2 << 10)       /* v7m only */
#define CORTEXM_DWT_FUNC_FUNC_READ      (5 << 0)
#define CORTEXM_DWT_FUNC_FUNC_WRITE     (6 << 0)
#define CORTEXM_DWT_FUNC_FUNC_ACCESS    (7 << 0)

/* Signals returned by cortexm_halt_wait() */
#define SIGINT  2
#define SIGTRAP 5
#define SIGSEGV 11

//////////////////////////////////////////////////////////////////////////////////////////////
/* Register number tables */
static const uint32_t regnum_cortex_m[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,   /* standard r0-r15 */
  0x10,   /* xpsr */
  0x11,   /* msp */
  0x12,   /* psp */
  0x14    /* special */
};

static const uint32_t regnum_cortex_mf[] = {
  0x21,   /* fpscr */
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* s0-s7 */
  0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* s8-s15 */
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* s16-s23 */
  0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, /* s24-s31 */
};

//////////////////////////////////////////////////////////////////////////////////////////////
/*
void cmprint (void) {
  FILE* out = fopen ("stm32f4map.xml","w");
  fprintf (out, "%s\n", stm32f4_xml_memory_map);
  fclose (out);
}

*/
CortexMx::CortexMx (GdbServer* s, const char* name) : Target (s), CommandSet (name),
  cCatch ("vector_catch", "Catch exception vectors") {
  addCmd (cCatch);
  cCatch.setServer (s);
  
  xml_mem_map = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
bool CortexMx::probe (void) {
  if (!Target::probe()) return false;
  
  tdesc     = tdesc_cortex_m;
  regs_size = sizeof (regnum_cortex_m);
  /* Probe for FP extension */
  uint32_t cpacr = apdp.ap.ap_mem_read (CORTEXM_CPACR);
  cpacr |= 0x00F00000; /* CP10 = 0b11, CP11 = 0b11 */
  apdp.ap.ap_mem_write (CORTEXM_CPACR, cpacr);
  if (apdp.ap.ap_mem_read (CORTEXM_CPACR) == cpacr) {
    target_options |= TOPT_FLAVOUR_V7MF;
    regs_size += sizeof (regnum_cortex_mf);
    tdesc = tdesc_cortex_mf;
  }

  /* Default vectors to catch */
  priv.demcr = CORTEXM_DEMCR_TRCENA | CORTEXM_DEMCR_VC_HARDERR |
               CORTEXM_DEMCR_VC_CORERESET;

  /*
        PROBE(...);
  */
  getServer()->mon.addSet (this);
  cCatch.reply ("Core Id: 0x%08X\n", idcode);
  return true;

}

void CortexMx::remove (void) {
  debug ("Remove: %s\n", getName());
  CommandSet::remove();
}


const char* CortexMx::getName (void) {
  return CommandSet::getName();
}

bool CortexMx::attach (void) {
  if (attached) return true;
  debug ("%s\n", __func__);
  unsigned i;
  uint32_t r;

  /* Clear any pending fault condition */
  check_error ();

  halt_request ();
  
  while (! halt_wait()) {/* TODO: Nejake delay */} ;
  
  /* Request halt on reset */
  apdp.ap.ap_mem_write (CORTEXM_DEMCR, priv.demcr);

  /* Reset DFSR flags */
  apdp.ap.ap_mem_write (CORTEXM_DFSR, CORTEXM_DFSR_RESETALL);

  /* size the break/watchpoint units */
  priv.hw_breakpoint_max = CORTEXM_MAX_BREAKPOINTS;
  r = apdp.ap.ap_mem_read (CORTEXM_FPB_CTRL);
  if ( ( (r >> 4) & 0xf) < priv.hw_breakpoint_max) /* only look at NUM_COMP1 */
    priv.hw_breakpoint_max = (r >> 4) & 0xf;
  priv.hw_watchpoint_max = CORTEXM_MAX_WATCHPOINTS;
  r = apdp.ap.ap_mem_read (CORTEXM_DWT_CTRL);
  if ( (r >> 28) > priv.hw_watchpoint_max)
    priv.hw_watchpoint_max = r >> 28;

  /* Clear any stale breakpoints */
  for (i = 0; i < priv.hw_breakpoint_max; i++) {
    apdp.ap.ap_mem_write (CORTEXM_FPB_COMP (i), 0);
    priv.hw_breakpoint[i] = 0;
  }

  /* Clear any stale watchpoints */
  for (i = 0; i < priv.hw_watchpoint_max; i++) {
    apdp.ap.ap_mem_write (CORTEXM_DWT_FUNC (i), 0);
    priv.hw_watchpoint[i].type = 0;
  }

  /* Flash Patch Control Register: set ENABLE */
  apdp.ap.ap_mem_write (CORTEXM_FPB_CTRL,
                    CORTEXM_FPB_CTRL_KEY | CORTEXM_FPB_CTRL_ENABLE);
  attached = true;
  return attached;
}
void CortexMx::detach (void) {
  debug ("%s\n", __func__);

  unsigned i;

  /* Clear any stale breakpoints */
  for (i = 0; i < priv.hw_breakpoint_max; i++)
    apdp.ap.ap_mem_write (CORTEXM_FPB_COMP (i), 0);

  /* Clear any stale watchpoints */
  for (i = 0; i < priv.hw_watchpoint_max; i++)
    apdp.ap.ap_mem_write (CORTEXM_DWT_FUNC (i), 0);

  /* Disable debug */
  apdp.ap.ap_mem_write (CORTEXM_DHCSR, CORTEXM_DHCSR_DBGKEY);
  
  // remove();
  attached = false;
}
int CortexMx::check_hw_wp (uint32_t *addr) {
  //debug ("%s\n", __func__);
  unsigned i;

  for (i = 0; i < priv.hw_watchpoint_max; i++)
    /* if SET and MATCHED then break */
    if (priv.hw_watchpoint[i].type &&
        (apdp.ap.ap_mem_read (CORTEXM_DWT_FUNC (i)) &
         CORTEXM_DWT_FUNC_MATCHED))
      break;

  if (i == priv.hw_watchpoint_max) return 0;

  *addr = priv.hw_watchpoint[i].addr;
  return 1;
}
int CortexMx::clear_hw_bp (uint32_t addr) {
  debug ("%s\n", __func__);
  unsigned i;

  for (i = 0; i < priv.hw_breakpoint_max; i++)
    if ( (priv.hw_breakpoint[i] & ~1) == addr) break;

  if (i == priv.hw_breakpoint_max) return -1;

  priv.hw_breakpoint[i] = 0;

  apdp.ap.ap_mem_write (CORTEXM_FPB_COMP (i), 0);

  return 0;
}
int CortexMx::clear_hw_wp (uint8_t type, uint32_t addr, uint8_t len) {
  debug ("%s\n", __func__);
  unsigned i;

  switch (len) {
    case 1:
      len = CORTEXM_DWT_MASK_BYTE;
      break;
    case 2:
      len = CORTEXM_DWT_MASK_HALFWORD;
      break;
    case 4:
      len = CORTEXM_DWT_MASK_WORD;
      break;
    default:
      return -1;
  }

  switch (type) {
    case 2:
      type = CORTEXM_DWT_FUNC_FUNC_WRITE;
      break;
    case 3:
      type = CORTEXM_DWT_FUNC_FUNC_READ;
      break;
    case 4:
      type = CORTEXM_DWT_FUNC_FUNC_ACCESS;
      break;
    default:
      return -1;
  }

  for (i = 0; i < priv.hw_watchpoint_max; i++)
    if ( (priv.hw_watchpoint[i].addr == addr) &&
         (priv.hw_watchpoint[i].type == type) &&
         (priv.hw_watchpoint[i].size == len)) break;

  if (i == priv.hw_watchpoint_max) return -2;

  priv.hw_watchpoint[i].type = 0;

  apdp.ap.ap_mem_write (CORTEXM_DWT_FUNC (i), 0);

  return 0;
}
int CortexMx::set_hw_bp (uint32_t addr) {
  debug ("%s\n", __func__);
  uint32_t val = addr & 0x1FFFFFFC;
  unsigned i;

  val |= (addr & 2) ? 0x80000000:0x40000000;
  val |= 1;

  for (i = 0; i < priv.hw_breakpoint_max; i++)
    if ( (priv.hw_breakpoint[i] & 1) == 0) break;

  if (i == priv.hw_breakpoint_max) return -1;

  priv.hw_breakpoint[i] = addr | 1;

  apdp.ap.ap_mem_write (CORTEXM_FPB_COMP (i), val);

  return 0;
}
int CortexMx::set_hw_wp (uint8_t type, uint32_t addr, uint8_t len) {
  debug ("%s\n", __func__);
  unsigned i;

  switch (len) { /* Convert bytes size to mask size */
    case 1:
      len = CORTEXM_DWT_MASK_BYTE;
      break;
    case 2:
      len = CORTEXM_DWT_MASK_HALFWORD;
      break;
    case 4:
      len = CORTEXM_DWT_MASK_WORD;
      break;
    default:
      return -1;
  }

  switch (type) { /* Convert gdb type to function type */
    case 2:
      type = CORTEXM_DWT_FUNC_FUNC_WRITE;
      break;
    case 3:
      type = CORTEXM_DWT_FUNC_FUNC_READ;
      break;
    case 4:
      type = CORTEXM_DWT_FUNC_FUNC_ACCESS;
      break;
    default:
      return -1;
  }

  for (i = 0; i < priv.hw_watchpoint_max; i++)
    if ( (priv.hw_watchpoint[i].type == 0) &&
         ( (apdp.ap.ap_mem_read (CORTEXM_DWT_FUNC (i)) & 0xF) == 0))
      break;

  if (i == priv.hw_watchpoint_max) return -2;

  priv.hw_watchpoint[i].type = type;
  priv.hw_watchpoint[i].addr = addr;
  priv.hw_watchpoint[i].size = len;

  apdp.ap.ap_mem_write (CORTEXM_DWT_COMP (i), addr);
  apdp.ap.ap_mem_write (CORTEXM_DWT_MASK (i), len);
  apdp.ap.ap_mem_write (CORTEXM_DWT_FUNC (i), type |
                    ( (target_options & TOPT_FLAVOUR_V6M) ? 0: CORTEXM_DWT_FUNC_DATAVSIZE_WORD));

  return 0;
}

void CortexMx::reset (void) {
  debug ("%s\n", __func__);

  /* Read DHCSR here to clear S_RESET_ST bit before reset */
  apdp.ap.ap_mem_read (CORTEXM_DHCSR);

  /* Request system reset from NVIC: SRST doesn't work correctly */
  /* This could be VECTRESET: 0x05FA0001 (reset only core)
   *          or SYSRESETREQ: 0x05FA0004 (system reset)
   */
  apdp.ap.ap_mem_write (CORTEXM_AIRCR,
                    CORTEXM_AIRCR_VECTKEY | CORTEXM_AIRCR_SYSRESETREQ);

  /* Poll for release from reset */
  while (apdp.ap.ap_mem_read (CORTEXM_DHCSR) & CORTEXM_DHCSR_S_RESET_ST);

  /* Reset DFSR flags */
  apdp.ap.ap_mem_write (CORTEXM_DFSR, CORTEXM_DFSR_RESETALL);
}

void CortexMx::halt_request (void) {
  debug ("%s\n", __func__);

  apdp.ap.dp->allow_timeout = false;
  apdp.ap.ap_mem_write (CORTEXM_DHCSR,
                    CORTEXM_DHCSR_DBGKEY | CORTEXM_DHCSR_C_HALT | CORTEXM_DHCSR_C_DEBUGEN);

}
void CortexMx::halt_resume (bool step) {
  debug ("%s - %d\n", __func__, step);
  uint32_t dhcsr = CORTEXM_DHCSR_DBGKEY | CORTEXM_DHCSR_C_DEBUGEN;

  if (step) dhcsr |= CORTEXM_DHCSR_C_STEP | CORTEXM_DHCSR_C_MASKINTS;

  /* Disable interrupts while single stepping... */
  if (step != priv.stepping) {
    apdp.ap.ap_mem_write (CORTEXM_DHCSR, dhcsr | CORTEXM_DHCSR_C_HALT);
    priv.stepping = step;
  }

  if (priv.on_bkpt) {
    uint32_t pc = pc_read ();
    if ( (apdp.ap.ap_mem_read_halfword (pc) & 0xFF00) == 0xBE00)
      pc_write (pc + 2);
  }
  debug ("%s : DHCSR=0x%08X\n", __func__, dhcsr);

  apdp.ap.ap_mem_write (CORTEXM_DHCSR, dhcsr);
  apdp.ap.dp->allow_timeout = true;

}
int CortexMx::halt_wait (void) {
  //debug ("%s\n", __func__);
  if (! (apdp.ap.ap_mem_read (CORTEXM_DHCSR) & CORTEXM_DHCSR_S_HALT))
    return 0;

  apdp.ap.dp->allow_timeout = false;

  /* We've halted.  Let's find out why. */
  uint32_t dfsr = apdp.ap.ap_mem_read (CORTEXM_DFSR);
  apdp.ap.ap_mem_write (CORTEXM_DFSR, dfsr); /* write back to reset */

  if ( (dfsr & CORTEXM_DFSR_VCATCH) && fault_unwind ())
    return SIGSEGV;

  /* Remember if we stopped on a breakpoint */
  priv.on_bkpt = dfsr & (CORTEXM_DFSR_BKPT);
  if (priv.on_bkpt) {
    /* If we've hit a programmed breakpoint, check for semihosting
     * call. */
    uint32_t pc = pc_read ();
    uint16_t bkpt_instr;
    mem_read_bytes ( (uint8_t *) &bkpt_instr, pc, 2);
    if (bkpt_instr == 0xBEAB) {
      int n = hostio_request ();
      if (n > 0) {
        halt_resume (priv.stepping);
        return 0;
      } else if (n < 0) {
        return -1;
      }
    }
  }

  if (dfsr & (CORTEXM_DFSR_BKPT | CORTEXM_DFSR_DWTTRAP))
    return SIGTRAP;

  if (dfsr & CORTEXM_DFSR_HALTED)
    return priv.stepping ? SIGTRAP : SIGINT;

  return SIGTRAP;
}
uint32_t CortexMx::pc_read (void) {
  debug ("%s\n", __func__);

  apdp.ap.ap_mem_write (CORTEXM_DCRSR, 0x0F);
  return apdp.ap.ap_mem_read (CORTEXM_DCRDR);

  return 0;
}
int CortexMx::pc_write (const uint32_t val) {
  debug ("%s\n", __func__);

  apdp.ap.ap_mem_write (CORTEXM_DCRDR, val);
  apdp.ap.ap_mem_write (CORTEXM_DCRSR, CORTEXM_DCRSR_REGWnR | 0x0F);

  return 0;
}
int CortexMx::regs_read (void *data) {
  debug ("%s\n", __func__);
  uint32_t *regs = (uint32_t *) data;
  unsigned i;

  /* FIXME: Describe what's really going on here */
  apdp.ap.ap_write (ADIV5_AP_CSW, apdp.ap.csw |
                ADIV5_AP_CSW_SIZE_WORD | ADIV5_AP_CSW_ADDRINC_SINGLE);

  /* Map the banked data registers (0x10-0x1c) to the
   * debug registers DHCSR, DCRSR, DCRDR and DEMCR respectively */
  apdp.dp.low_access (1, 0, ADIV5_AP_TAR, CORTEXM_DHCSR);

  /* Walk the regnum_cortex_m array, reading the registers it
   * calls out. */
  apdp.ap.ap_write (ADIV5_AP_DB (1), regnum_cortex_m[0]); /* Required to switch banks */
  *regs++ = apdp.dp.dp_read_ap (ADIV5_AP_DB (2));
  for (i = 1; i < sizeof (regnum_cortex_m) / 4; i++) {
    apdp.dp.low_access (1, 0, ADIV5_AP_DB (1), regnum_cortex_m[i]);
    *regs++ = apdp.dp.dp_read_ap (ADIV5_AP_DB (2));
  }
  if (target_options & TOPT_FLAVOUR_V7MF)
    for (i = 0; i < sizeof (regnum_cortex_mf) / 4; i++) {
      apdp.dp.low_access (1, 0, ADIV5_AP_DB (1), regnum_cortex_mf[i]);
      *regs++ = apdp.dp.dp_read_ap (ADIV5_AP_DB (2));
    }

  return 0;
}
int CortexMx::regs_write (const void *data) {
  debug ("%s\n", __func__);
  const uint32_t *regs = (const uint32_t *) data;
  unsigned i;

  /* FIXME: Describe what's really going on here */
  apdp.ap.ap_write (ADIV5_AP_CSW, apdp.ap.csw |
                ADIV5_AP_CSW_SIZE_WORD | ADIV5_AP_CSW_ADDRINC_SINGLE);

  /* Map the banked data registers (0x10-0x1c) to the
   * debug registers DHCSR, DCRSR, DCRDR and DEMCR respectively */
  apdp.dp.low_access (1, 0, ADIV5_AP_TAR, CORTEXM_DHCSR);

  /* Walk the regnum_cortex_m array, writing the registers it
   * calls out. */
  apdp.ap.ap_write (ADIV5_AP_DB (2), *regs++); /* Required to switch banks */
  //adiv5_dp_low_access (apdp.ap.dp, 1, 0, ADIV5_AP_DB (1), 0x10000 | regnum_cortex_m[0]);
  apdp.dp.low_access (1, 0, ADIV5_AP_DB (1), 0x10000 | regnum_cortex_m[0]);
  for (i = 1; i < sizeof (regnum_cortex_m) / 4; i++) {
    apdp.dp.low_access (1, 0, ADIV5_AP_DB (2), *regs++);
    apdp.dp.low_access (1, 0, ADIV5_AP_DB (1),
                        0x10000 | regnum_cortex_m[i]);
  }
  if (target_options & TOPT_FLAVOUR_V7MF)
    for (i = 0; i < sizeof (regnum_cortex_mf) / 4; i++) {
      apdp.dp.low_access (1, 0, ADIV5_AP_DB (2), *regs++);
      apdp.dp.low_access (1, 0, ADIV5_AP_DB (1),
                          0x10000 | regnum_cortex_mf[i]);
    }

  return 0;
}
int CortexMx::fault_unwind (void) {
  uint32_t hfsr = apdp.ap.ap_mem_read (CORTEXM_HFSR);
  uint32_t cfsr = apdp.ap.ap_mem_read (CORTEXM_CFSR);
  apdp.ap.ap_mem_write (CORTEXM_HFSR, hfsr); /* write back to reset */
  apdp.ap.ap_mem_write (CORTEXM_CFSR, cfsr); /* write back to reset */
  /* We check for FORCED in the HardFault Status Register or
   * for a configurable fault to avoid catching core resets */
  if ( (hfsr & CORTEXM_HFSR_FORCED) || cfsr) {
    /* Unwind exception */
    uint32_t regs  [regs_size / 4];
    uint32_t stack [8];
    uint32_t retcode, framesize;
    /* Read registers for post-exception stack pointer */
    regs_read (regs);
    /* save retcode currently in lr */
    retcode = regs[14];
    /* Read stack for pre-exception registers */
    mem_read_words (stack, regs[13], sizeof (stack));
    regs[14] = stack[5];    /* restore LR to pre-exception state */
    regs[15] = stack[6];    /* restore PC to pre-exception state */

    /* adjust stack to pop exception state */
    framesize = (retcode & (1<<4)) ? 0x68 : 0x20;   /* check for basic vs. extended frame */
    if (stack[7] & (1<<9))                          /* check for stack alignment fixup */
      framesize += 4;
    regs[13] += framesize;

    /* FIXME: stack[7] contains xPSR when this is supported */
    /* although, if we caught the exception it will be unchanged */

    /* Reset exception state to allow resuming from restored
     * state.
     */
    apdp.ap.ap_mem_write (CORTEXM_AIRCR,
                      CORTEXM_AIRCR_VECTKEY | CORTEXM_AIRCR_VECTCLRACTIVE);

    /* Write pre-exception registers back to core */
    regs_write (regs);

    return 1;
  }
  return 0;
}
/* Semihosting support */
/* ARM Semihosting syscall numbers, from ARM doc DUI0471C, Chapter 8 */
#define SYS_CLOSE       0x02
#define SYS_CLOCK       0x10
#define SYS_ELAPSED     0x30
#define SYS_ERRNO       0x13
#define SYS_FLEN        0x0C
#define SYS_GET_CMDLINE 0x15
#define SYS_HEAPINFO    0x16
#define SYS_ISERROR     0x08
#define SYS_ISTTY       0x09
#define SYS_OPEN        0x01
#define SYS_READ        0x06
#define SYS_READC       0x07
#define SYS_REMOVE      0x0E
#define SYS_RENAME      0x0F
#define SYS_SEEK        0x0A
#define SYS_SYSTEM      0x12
#define SYS_TICKFREQ    0x31
#define SYS_TIME        0x11
#define SYS_TMPNAM      0x0D
#define SYS_WRITE       0x05
#define SYS_WRITEC      0x03
#define SYS_WRITE0      0x04

#define FILEIO_O_RDONLY         0
#define FILEIO_O_WRONLY         1
#define FILEIO_O_RDWR           2
#define FILEIO_O_APPEND         0x008
#define FILEIO_O_CREAT          0x200
#define FILEIO_O_TRUNC          0x400

#define FILEIO_SEEK_SET         0
#define FILEIO_SEEK_CUR         1
#define FILEIO_SEEK_END         2

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

void CortexMx::hostio_reply (int32_t retcode, uint32_t errcode) {
  uint32_t arm_regs [regs_size];

  debug ("syscall return ret=%d errno=%d\n", retcode, errcode);
  regs_read (arm_regs);
  if ( ( (priv.syscall == SYS_READ) || (priv.syscall == SYS_WRITE)) &&
       (retcode > 0))
    retcode = priv.byte_count - retcode;
  if ( (priv.syscall == SYS_OPEN) && (retcode != -1))
    retcode++;
  arm_regs[0] = retcode;
  regs_write (arm_regs);
  priv.errno = errcode;

}
int CortexMx::hostio_request (void) {
  GdbServer * serv = getServer();
  //struct cortexm_priv *priv = apdp.ap.priv;
  uint32_t arm_regs [regs_size];
  uint32_t params   [4];

  regs_read (arm_regs);
  mem_read_words (params, arm_regs[1], sizeof (params));
  priv.syscall = arm_regs[0];

  debug ("syscall 0x%x (%x %x %x %x)\n", priv.syscall,
         params[0], params[1], params[2], params[3]);
  switch (priv.syscall) {
    case SYS_OPEN: { /* open */
      /* Translate stupid fopen modes to open flags.
       * See DUI0471C, Table 8-3 */
      const uint32_t flags[] = {
        FILEIO_O_RDONLY,        /* r, rb */
        FILEIO_O_RDWR,          /* r+, r+b */
        FILEIO_O_WRONLY | FILEIO_O_CREAT | FILEIO_O_TRUNC,/*w*/
        FILEIO_O_RDWR   | FILEIO_O_CREAT | FILEIO_O_TRUNC,/*w+*/
        FILEIO_O_WRONLY | FILEIO_O_CREAT | FILEIO_O_APPEND,/*a*/
        FILEIO_O_RDWR   | FILEIO_O_CREAT | FILEIO_O_APPEND,/*a+*/
      };
      uint32_t pflag = flags[params[1] >> 1];
      char filename[4];

      mem_read_bytes ((uint8_t *) filename, params[0], sizeof (filename));
      /* handle requests for console i/o */
      if (!strcmp (filename, ":tt")) {
        if (pflag == FILEIO_O_RDONLY)
          arm_regs[0] = STDIN_FILENO;
        else if (pflag & FILEIO_O_TRUNC)
          arm_regs[0] = STDOUT_FILENO;
        else
          arm_regs[0] = STDERR_FILENO;
        arm_regs[0]++;
        regs_write (arm_regs);
        return 1;
      }

      serv->gdb_putpacket_f ("Fopen,%08X/%X,%08X,%08X",
                        params[0], params[2] + 1,
                        pflag, 0644);
      break;
    }
    case SYS_CLOSE: /* close */
      serv->gdb_putpacket_f ("Fclose,%08X", params[0] - 1);
      break;
    case SYS_READ:  /* read */
      priv.byte_count = params[2];
      serv->gdb_putpacket_f ("Fread,%08X,%08X,%08X",
                        params[0] - 1, params[1], params[2]);
      break;
    case SYS_WRITE: /* write */
      priv.byte_count = params[2];
      serv->gdb_putpacket_f ("Fwrite,%08X,%08X,%08X",
                        params[0] - 1, params[1], params[2]);
      break;
    case SYS_WRITEC: /* writec */
      serv->gdb_putpacket_f ("Fwrite,2,%08X,1", arm_regs[1]);
      break;
    case SYS_ISTTY: /* isatty */
      serv->gdb_putpacket_f ("Fisatty,%08X", params[0] - 1);
      break;
    case SYS_SEEK:  /* lseek */
      serv->gdb_putpacket_f ("Flseek,%08X,%08X,%08X",
                        params[0] - 1, params[1], FILEIO_SEEK_SET);
      break;
    case SYS_RENAME:/* rename */
      serv->gdb_putpacket_f ("Frename,%08X/%X,%08X/%X",
                        params[0] - 1, params[1] + 1,
                        params[2], params[3] + 1);
      break;
    case SYS_REMOVE:/* unlink */
      serv->gdb_putpacket_f ("Funlink,%08X/%X", params[0] - 1,
                        params[1] + 1);
      break;
    case SYS_SYSTEM:/* system */
      serv->gdb_putpacket_f ("Fsystem,%08X/%X", params[0] - 1,
                        params[1] + 1);
      break;

    case SYS_FLEN:  /* Not supported, fake success */
      priv.errno = 0;
      return 1;

    case SYS_ERRNO: /* Return last errno from GDB */
      arm_regs[0] = priv.errno;
      regs_write (arm_regs);
      return 1;

    case SYS_TIME:  /* gettimeofday */
      /* FIXME How do we use 's gettimeofday? */
    default:
      return 0;
  }

  return -1;
}

int CortexMx::Handler (int argc, const char *argv[]) {
  int res = CommandSet::Handler (argc, argv);
  if (res == 1) vector_catch (argc, argv);
  return res;
}


bool CortexMx::vector_catch (int argc, const char *argv[]) {
  static const char *vectors[] = {"reset", NULL, NULL, NULL, "mm", "nocp",
                                  "chk", "stat", "bus", "int", "hard"
                                 };
  uint32_t tmp = 0;
  unsigned i, j;

  if ( (argc < 3) || ( (argv[1][0] != 'e') && (argv[1][0] != 'd'))) {
    cCatch.reply ("usage: monitor vector_catch (enable|disable) "
                  "(hard|int|bus|stat|chk|nocp|mm|reset)\n");
  } else {
    for (j = 0; j < (unsigned) argc; j++)
      for (i = 0; i < sizeof (vectors) / sizeof (char *); i++) {
        if (vectors[i] && !strcmp (vectors[i], argv[j]))
          tmp |= 1 << i;
      }

    if (argv[1][0] == 'e')
      priv.demcr |= tmp;
    else
      priv.demcr &= ~tmp;

    apdp.ap.ap_mem_write (CORTEXM_DEMCR, priv.demcr);
  }

  cCatch.reply ("Catching vectors: ");
  for (i = 0; i < sizeof (vectors) / sizeof (char *); i++) {
    if (!vectors[i])
      continue;
    if (priv.demcr & (1 << i))
      cCatch.reply ("%s ", vectors[i]);
  }
  cCatch.reply ("\n");
  return true;
}
