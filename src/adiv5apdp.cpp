#include <stdio.h>
#include "string.h"
#include "baselayer.h"
#include "swdp.h"
#include "adiv5apdp.h"
#include "target.h"

#undef  debug
#define debug(...)

ADIv5DP::ADIv5DP() {
}

/*
#ifndef DO_RESET_SEQ
#define DO_RESET_SEQ 0
#endif
*/
void ADIv5DP::dp_init (void) {
  uint32_t ctrlstat;

  ctrlstat = dp_read (ADIV5_DP_CTRLSTAT);

  /* Write request for system and debug power up */
  dp_write (ADIV5_DP_CTRLSTAT,
            ctrlstat |= ADIV5_DP_CTRLSTAT_CSYSPWRUPREQ |
                        ADIV5_DP_CTRLSTAT_CDBGPWRUPREQ);
  /* Wait for acknowledge */
  while ( ( (ctrlstat = dp_read (ADIV5_DP_CTRLSTAT)) &
            (ADIV5_DP_CTRLSTAT_CSYSPWRUPACK | ADIV5_DP_CTRLSTAT_CDBGPWRUPACK)) !=
          (ADIV5_DP_CTRLSTAT_CSYSPWRUPACK | ADIV5_DP_CTRLSTAT_CDBGPWRUPACK));

  // TODO: zatim nevim, ale z nasledujiciho muze zbyt asi toto:
  // Ta puvodni smycka byla asi pro vice jader na cipu, IDR se objevuje jen jednou.
  ap->idr = ap->ap_read (ADIV5_AP_IDR);
  if (!ap->idr) return;
  debug ("   IDR = %08X\n", ap->idr);

  ap->cfg  = ap->ap_read (ADIV5_AP_CFG);
  ap->base = ap->ap_read (ADIV5_AP_BASE);
  ap->csw  = ap->ap_read (ADIV5_AP_CSW) &
            ~ (ADIV5_AP_CSW_SIZE_MASK | ADIV5_AP_CSW_ADDRINC_MASK);
  debug ("ap->cfg=%08X, ap->base=%08X, ap->csw=%08X\n", ap->cfg, ap->base, ap->csw);
#if 0  
  /* Probe for APs on this DP */
  for (int i = 0; i < 256; i++) {
    ADIv5AP *tap, tmpap;
    //Target *t;

    /* Assume valid and try to read IDR */
    memset (&tmpap, 0, sizeof (tmpap));
    tmpap.dp    = this;
    tmpap.apsel = i;
    tmpap.idr   = tmpap.ap_read (ADIV5_AP_IDR);
    
    debug ("   IDR = %08X\n", tmpap.idr);
    if (!tmpap.idr) /* IDR Invalid - Should we not continue here? */
      break;

    ap->idr = tmpap.idr;
    /* It's valid to so create a heap copy */
    tap = new ADIv5AP();
    memcpy (tap, &tmpap, sizeof (*tap));
    //adiv5_dp_ref (dp);

    tap->cfg  = tap->ap_read (ADIV5_AP_CFG);
    tap->base = tap->ap_read (ADIV5_AP_BASE);
    tap->csw  = tap->ap_read (ADIV5_AP_CSW) &
              ~ (ADIV5_AP_CSW_SIZE_MASK | ADIV5_AP_CSW_ADDRINC_MASK);
    debug ("tap->cfg=%08X, tap->base=%08X, tap->csw=%08X\n", tap->cfg, tap->base, tap->csw);
    ap->cfg  = tap->cfg;
    ap->base = tap->base;
    ap->csw  = tap->csw;
    delete tap;
    /* Should probe further here to make sure it's a valid target.
     * AP should be unref'd if not valid.
     */

    /* Prepend to target list... */
    //t = new  Target();
    //adiv5_ap_ref (ap);
    //t->priv = ap;
    //t->priv_free = (void ( *) (void *)) adiv5_ap_unref;

    //t->driver = adiv5_driver_str;
    //t->check_error = ap_check_error;

    //t->mem_read_words = ap_mem_read_words;
    //t->mem_write_words = ap_mem_write_words;
    //t->mem_read_bytes = ap_mem_read_bytes;
    //t->mem_write_bytes = ap_mem_write_bytes;

    /* The rest sould only be added after checking ROM table */
    //cortexm_probe (t);
  }
#endif
}
#define SWDP_ACK_OK    0x01
#define SWDP_ACK_WAIT  0x02
#define SWDP_ACK_FAULT 0x04

  uint32_t ADIv5DP::dp_read (uint8_t addr) {
    debug ("%s\n", __func__);
    return low_access (ADIV5_LOW_DP, ADIV5_LOW_READ, addr, 0);
  }
  void ADIv5DP::dp_write (uint8_t addr, uint32_t value) {
    debug ("%s\n", __func__);
    low_access (ADIV5_LOW_DP, ADIV5_LOW_WRITE, addr, value);
  }
  uint32_t ADIv5DP::error (void) {
    debug ("%s\n", __func__);
    uint32_t err, clr = 0;

    err = dp_read (ADIV5_DP_CTRLSTAT) &
          (ADIV5_DP_CTRLSTAT_STICKYORUN | ADIV5_DP_CTRLSTAT_STICKYCMP |
           ADIV5_DP_CTRLSTAT_STICKYERR  | ADIV5_DP_CTRLSTAT_WDATAERR);

    if (err & ADIV5_DP_CTRLSTAT_STICKYORUN)
      clr |= ADIV5_DP_ABORT_ORUNERRCLR;
    if (err & ADIV5_DP_CTRLSTAT_STICKYCMP)
      clr |= ADIV5_DP_ABORT_STKCMPCLR;
    if (err & ADIV5_DP_CTRLSTAT_STICKYERR)
      clr |= ADIV5_DP_ABORT_STKERRCLR;
    if (err & ADIV5_DP_CTRLSTAT_WDATAERR)
      clr |= ADIV5_DP_ABORT_WDERRCLR;

    dp_write (ADIV5_DP_ABORT, clr);
    fault = 0;

    return err;
  }
  uint32_t ADIv5DP::low_access (uint8_t APnDP, uint8_t RnW, uint8_t addr, uint32_t value) {
    debug ("%s\n", __func__);
    // Nutno převést data na swdPacket už tady.
    swdPacket TxD;
    TxD.cmd   = cmdLowAccess;
    TxD.APnDP = APnDP;
    TxD.RnW   = RnW;
    TxD.adr   = addr;
    TxD.val   = value;
    if (gdb) gdb->BaseLayer::Up ( (char *) & TxD, sizeof (TxD));
    debug ("LA res = %d, AnD=%d, RW=%d, Adr=0x%02X, Value=0x%08X\n", TxD.RnW, APnDP, RnW, addr, TxD.val);
    if (RnW) value  = TxD.val;
    uint8_t  result = TxD.RnW;
    if (result != SWDP_ACK_OK) {
      //PLATFORM_FATAL_ERROR (1);
      fault = 1;          // TODO: osetri ostatni chyby, gdb pak keca nesmysly
      return 0;
    }
    return value;
  }

/////////////////////////////////////////////////////////////////////////////////////////////////
  ADIv5AP::ADIv5AP () {
  }

  uint32_t ADIv5AP::ap_read (uint8_t addr) {
    uint32_t ret;
    dp->dp_write (ADIV5_DP_SELECT,
                  ( (uint32_t) apsel << 24) | (addr & 0xF0));
    ret = dp->dp_read_ap (addr);
    debug ("%s:  a=0x%08x -- %08x\n", __func__, addr, ret);
    return ret;
  }
  void ADIv5AP::ap_write (uint8_t addr, uint32_t value) {
    debug ("%s: a=0x%08x, v=0x%08x\n", __func__, addr, value);
    dp->dp_write (ADIV5_DP_SELECT,
                  ( (uint32_t) apsel << 24) | (addr & 0xF0));
    dp->dp_write_ap (addr, value);
  }

  uint32_t ADIv5AP::ap_mem_read (uint32_t addr) {
    //debug ("%s:  a=0x%08x\n", __func__, addr);
    ap_write (ADIV5_AP_CSW, csw |
              ADIV5_AP_CSW_SIZE_WORD | ADIV5_AP_CSW_ADDRINC_SINGLE);
    ap_write (ADIV5_AP_TAR, addr);
    uint32_t res = ap_read (ADIV5_AP_DRW);
    debug ("%s:  a=0x%08x -> %08x\n", __func__, addr, res);
    return res;
  }
  uint16_t ADIv5AP::ap_mem_read_halfword (uint32_t addr) {
    ap_write (ADIV5_AP_CSW, csw |
              ADIV5_AP_CSW_SIZE_HALFWORD | ADIV5_AP_CSW_ADDRINC_SINGLE);
    ap_write (ADIV5_AP_TAR, addr);
    uint32_t v = ap_read (ADIV5_AP_DRW);
    debug ("%s:  a=0x%08x => %08x\n", __func__, addr, v);
    if (addr & 2)
      return v >> 16;
    else
      return v & 0xFFFF;
  }
  void ADIv5AP::ap_mem_write (uint32_t addr, uint32_t value) {
    debug ("%s: a=0x%08x, v=0x%08x\n", __func__, addr, value);
    ap_write (ADIV5_AP_CSW, csw |
              ADIV5_AP_CSW_SIZE_WORD | ADIV5_AP_CSW_ADDRINC_SINGLE);
    ap_write (ADIV5_AP_TAR, addr);
    ap_write (ADIV5_AP_DRW, value);
  }
  void ADIv5AP::ap_mem_write_halfword (uint32_t addr, uint16_t value) {
    debug ("%s: a=0x%08x, v=0x%08x\n", __func__, addr, value);
    uint32_t v = value;
    if (addr & 2)
      v <<= 16;

    ap_write (ADIV5_AP_CSW, csw |
              ADIV5_AP_CSW_SIZE_HALFWORD | ADIV5_AP_CSW_ADDRINC_SINGLE);
    ap_write (ADIV5_AP_TAR, addr);
    ap_write (ADIV5_AP_DRW, v);
  }

/////////////////////////////////////////////////////////////////////////////////////////////////
  ADIv5APDP::ADIv5APDP() : ap(), dp () {
    ap.dp = & dp;
    dp.ap = & ap;
  }

/////////////////////////////////////////////////////////////////////////////////////////////////

  uint32_t ADIv5DP::dp_read_ap (uint8_t addr) {
    debug ("%s\n", __func__);
    uint32_t ret;

    low_access (ADIV5_LOW_AP, ADIV5_LOW_READ, addr, 0);
    ret = low_access (ADIV5_LOW_DP, ADIV5_LOW_READ,
                      ADIV5_DP_RDBUFF, 0);

    return ret;
  }
  void ADIv5DP::dp_write_ap (uint8_t addr, uint32_t value) {
    debug ("%s\n", __func__);
    low_access (ADIV5_LOW_AP, ADIV5_LOW_WRITE, addr, value);
  }
