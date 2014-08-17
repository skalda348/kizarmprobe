#ifndef ADIV5DP_H
#define ADIV5DP_H
#include <stdint.h>

/* ADIv5 DP Register addresses */
#define ADIV5_DP_IDCODE   0x0
#define ADIV5_DP_ABORT    0x0
#define ADIV5_DP_CTRLSTAT 0x4
#define ADIV5_DP_SELECT   0x8
#define ADIV5_DP_RDBUFF   0xC

/* AP Abort Register (ABORT) */
/* Bits 31:5 - Reserved */
#define ADIV5_DP_ABORT_ORUNERRCLR (1 << 4)
#define ADIV5_DP_ABORT_WDERRCLR   (1 << 3)
#define ADIV5_DP_ABORT_STKERRCLR  (1 << 2)
#define ADIV5_DP_ABORT_STKCMPCLR  (1 << 1)
/* Bits 5:1 - SW-DP only, reserved in JTAG-DP */
#define ADIV5_DP_ABORT_DAPABORT   (1 << 0)

/* Control/Status Register (CTRLSTAT) */
#define ADIV5_DP_CTRLSTAT_CSYSPWRUPACK  (1u << 31)
#define ADIV5_DP_CTRLSTAT_CSYSPWRUPREQ  (1u << 30)
#define ADIV5_DP_CTRLSTAT_CDBGPWRUPACK  (1u << 29)
#define ADIV5_DP_CTRLSTAT_CDBGPWRUPREQ  (1u << 28)
#define ADIV5_DP_CTRLSTAT_CDBGRSTACK    (1u << 27)
#define ADIV5_DP_CTRLSTAT_CDBGRSTREQ    (1u << 26)
/* Bits 25:24 - Reserved */
/* Bits 23:12 - TRNCNT */
#define ADIV5_DP_CTRLSTAT_TRNCNT
/* Bits 11:8 - MASKLANE */
#define ADIV5_DP_CTRLSTAT_MASKLANE
/* Bits 7:6 - Reserved in JTAG-DP */
#define ADIV5_DP_CTRLSTAT_WDATAERR      (1u << 7)
#define ADIV5_DP_CTRLSTAT_READOK        (1u << 6)
#define ADIV5_DP_CTRLSTAT_STICKYERR     (1u << 5)
#define ADIV5_DP_CTRLSTAT_STICKYCMP     (1u << 4)
#define ADIV5_DP_CTRLSTAT_TRNMODE_MASK  (3u << 2)
#define ADIV5_DP_CTRLSTAT_STICKYORUN    (1u << 1)
#define ADIV5_DP_CTRLSTAT_ORUNDETECT    (1u << 0)


/* ADIv5 MEM-AP Registers */
#define ADIV5_AP_CSW            0x00
#define ADIV5_AP_TAR            0x04
/* 0x08 - Reserved */
#define ADIV5_AP_DRW            0x0C
#define ADIV5_AP_DB(x)         (0x10 + (4*(x)))
/* 0x20:0xF0 - Reserved */
#define ADIV5_AP_CFG            0xF4
#define ADIV5_AP_BASE           0xF8
#define ADIV5_AP_IDR            0xFC

/* AP Control and Status Word (CSW) */
#define ADIV5_AP_CSW_DBGSWENABLE        (1u << 31)
/* Bits 30:24 - Prot, Implementation defined, for Cortex-M3: */
#define ADIV5_AP_CSW_MASTERTYPE_DEBUG   (1u << 29)
#define ADIV5_AP_CSW_HPROT1             (1u << 25)
#define ADIV5_AP_CSW_SPIDEN             (1u << 23)
/* Bits 22:12 - Reserved */
/* Bits 11:8 - Mode, must be zero */
#define ADIV5_AP_CSW_TRINPROG           (1u << 7)
#define ADIV5_AP_CSW_DEVICEEN           (1u << 6)
#define ADIV5_AP_CSW_ADDRINC_NONE       (0u << 4)
#define ADIV5_AP_CSW_ADDRINC_SINGLE     (1u << 4)
#define ADIV5_AP_CSW_ADDRINC_PACKED     (2u << 4)
#define ADIV5_AP_CSW_ADDRINC_MASK       (3u << 4)
/* Bit 3 - Reserved */
#define ADIV5_AP_CSW_SIZE_BYTE          (0u << 0)
#define ADIV5_AP_CSW_SIZE_HALFWORD      (1u << 0)
#define ADIV5_AP_CSW_SIZE_WORD          (2u << 0)
#define ADIV5_AP_CSW_SIZE_MASK          (7u << 0)

/* Constants to make RnW and APnDP parameters more clear in code */
#define ADIV5_LOW_WRITE         0
#define ADIV5_LOW_READ          1
#define ADIV5_LOW_DP            0
#define ADIV5_LOW_AP            1

// Forward declare
class ADIv5AP;
class GdbServer;

class ADIv5DP {

  public:
    ADIv5DP ();

    void dp_init                (void);
    /*
    void setServer              (GdbServer * s) {
      gdb = s;
    }
    */
    /*
     * Původně tyhle reference asi sloužily pro více targetů.
     * Protože zde bude ADIv5APDP součástí targetu, je to asi zbytečné.
     * */
    //void adiv5_dp_ref           (void);
    //void adiv5_dp_unref         (void);
    
    void        dp_write        (uint8_t addr, uint32_t value);
    uint32_t    dp_read         (uint8_t addr);
    uint32_t    error           (void);
    uint32_t    low_access      (uint8_t APnDP, uint8_t RnW,
                                 uint8_t addr, uint32_t value);
    void        dp_write_ap     (uint8_t addr, uint32_t value);
    uint32_t    dp_read_ap      (uint8_t addr);
    
  private:
  public:
    GdbServer * gdb;
    ADIv5AP   * ap;
    //int         refcnt;
    uint32_t    idcode;
    uint32_t    allow_timeout;
    // Asi by slo i uint32_t fault, puvodne takto, mozna kvuli typove kontrole.
    union {
      void  *   unused;
      uint8_t   fault;
    };
    
};
class ADIv5AP {
  public:
    ADIv5AP ();

    uint32_t    ap_mem_read             (uint32_t addr);
    void        ap_mem_write            (uint32_t addr, uint32_t value);
    uint16_t    ap_mem_read_halfword    (uint32_t addr);
    void        ap_mem_write_halfword   (uint32_t addr, uint16_t value);

    void        ap_write        (uint8_t  addr, uint32_t value);
    uint32_t    ap_read         (uint8_t  addr);

  private:
  public:

    ADIv5DP *dp;
    uint32_t apsel;

    uint32_t idr;
    uint32_t cfg;
    uint32_t base;
    uint32_t csw;
};
/**
 * Asi to mělo být trochu jinak, ale takhle jednoduše to bude také dobře.
 * Tato struktura bude součástí targetu.
 * */
class ADIv5APDP {
  public:
    ADIv5APDP ();

    ADIv5AP ap;
    ADIv5DP dp;
};

#endif // ADIV5APDP_H
