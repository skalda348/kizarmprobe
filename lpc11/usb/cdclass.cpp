#include <stdio.h>
#include "cdclass.h"
extern "C" {
  #include "type.h"
  //#include "usb-desc.h"
  //#include "comp_desc.h"
  #include "usbuser.h"
  #include "usbcore.h"
};

static CDClass *CDCInstance = 0;

void CDClass::VCOM_EpHandler (void* data, uint32_t event) {
  CDClass* pVcom = (CDClass *) data;
  uint32_t rlen;
  switch (event) {
    case USB_EVT_OUT:
      rlen = USB_ReadEP (pVcom->bulkOut, pVcom->rxBuf);
      pVcom->Recv (rlen);
      break;
    case USB_EVT_IN:
      pVcom->usbtx_rdy = 1;       // takže mohou být poslána další
      pVcom->Send();
      break;
    default: break;
  }
}

void CDClass::Recv (uint32_t len) {
  uint32_t  n, ofs = 0;
  while (len) {
    n = Up ((char*) rxBuf + ofs, len);
    len -= n;
    ofs += n;
  }
}
/*
static inline void error (void) {
  asm volatile ("bkpt 0");
}
*/
CDClass::CDClass (const bool bl, const CDCIndividual * ip) :
  UsbClass(), BaseLayer(), tx(bl, ip->depth) {
  CDCInstance = this;
  bulkOut = ip->ep;
  bulkIn  = ip->ep | 0x80;
  EpHandlers[bulkOut].pEpn = VCOM_EpHandler;
  EpHandlers[bulkOut].data = this;
  
  // asm volatile ("bkpt 0");
  usbtx_rdy = 1;
}
uint32_t CDClass::Down (char* buf, uint32_t len) {
  // if (!isConfigured()) return len;   // stejně to čeká na spojení
  uint32_t n;
  for (n=0; n<len; n++) {
    if (!tx.Write(buf[n])) break;
  }
  Send ();
  return n;
}
void CDClass::Send (void) {
  char* ptr = (char*) txBuf;
  uint32_t n;
  
  if  (!usbtx_rdy) return;
  for (n=0; n<USB_MAX_NON_ISO_SIZE; n++) if (!tx.Read(ptr[n])) break;
  if  (!n) return;
  n = USB_WriteEP (bulkIn, txBuf, n);
  usbtx_rdy = 0;
}
/** ********************************************************************************/
#if 0
extern "C" {
  
  uint32_t iA0LineCode (CDC_LINE_CODING* line_coding) {
    debug ("A0 SetLineCoding: B:%d, f:%02X, p:%02X, d:%02X\n",
          line_coding->dwDTERate, line_coding->bCharFormat,
          line_coding->bParityType, line_coding->bDataBits);
  // cosi udelej se seriovym portem
    return (TRUE);
  }

  uint32_t iA0LineState (unsigned short wControlSignalBitmap) {
    debug ("A0 SetControlLineState: 0x%04X\n", wControlSignalBitmap);
    /* ... add code to handle request */
    return (TRUE);
  }

  
  uint32_t iA1LineCode (CDC_LINE_CODING* line_coding) {
    debug ("A1 SetLineCoding: B:%d, f:%02X, p:%02X, d:%02X\n",
          line_coding->dwDTERate, line_coding->bCharFormat,
          line_coding->bParityType, line_coding->bDataBits);
  // cosi udelej se seriovym portem
    return (TRUE);
  }

  uint32_t iA1LineState (unsigned short wControlSignalBitmap) {
    debug ("A1 SetControlLineState: 0x%04X\n", wControlSignalBitmap);
    /* ... add code to handle request */
    return (TRUE);
  }
  
};
#endif

