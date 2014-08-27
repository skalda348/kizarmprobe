#include "cdclass.h"
#include "usb-desc.h"

extern "C" {
  extern void memset (void* dst, int v, int len);
  extern void memcpy (void* dst, const void* src, int len);
}
/// Dáno výrobcem
extern USBD_API_T* pUsbApi;

ErrorCode_t CDClass::VCOM_bulk_in_hdlr (USBD_HANDLE_T hUsb, void* data, uint32_t event) {
  CDClass* pVcom = (CDClass*) data;
  if (event == USB_EVT_IN) {    // přerušení od vysílače - předešlá data odeslána
    pVcom->usbtx_rdy = 1;       // takže mohou být poslána další
    pVcom->Send();
  }
  return LPC_OK;
}
ErrorCode_t CDClass::VCOM_bulk_out_hdlr (USBD_HANDLE_T hUsb, void* data, uint32_t event) {
  CDClass* pVcom = (CDClass*) data;
  if (event == USB_EVT_OUT) {
    uint32_t rlen = pUsbApi->hw->ReadEP (hUsb, pVcom->bulkOut, pVcom->rxBuf);
    pVcom->Recv (rlen);
  }
  return LPC_OK;
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
CDClass::CDClass (const CDCIndividual * ip) :
  UsbClass(), BaseLayer(), tx (ip->depth) {
  bulkOut = ip->ep;
  bulkIn  = ip->ep | 0x80;
  USBD_CDC_INIT_PARAM_T cdc_param;
  ErrorCode_t ret = LPC_OK;
  uint32_t ep_indx;
  /* init CDC params */
  memset (&cdc_param, 0, sizeof (USBD_CDC_INIT_PARAM_T));

  /* user defined functions */
  cdc_param.SetLineCode      = ip->SetLineCode;
  cdc_param.SetCtrlLineState = ip->SetCtrlLineState;
  // cdc_param.SendBreak = VCOM_SendBreak;
  // init CDC params
  cdc_param.mem_base = mem.membase;
  cdc_param.mem_size = mem.memsize;
  cdc_param.cif_intf_desc = (uint8_t*) ip->if0; 
  cdc_param.dif_intf_desc = (uint8_t*) ip->if1;
  
  ret = pUsbApi->cdc->init (getHandle(), &cdc_param, &hCdc);
  
  if (ret != LPC_OK) return;

  /* allocate transfer buffers */
  rxBuf = (uint8_t*) (cdc_param.mem_base + (0 * USB_HS_MAX_BULK_PACKET));
  txBuf = (uint8_t*) (cdc_param.mem_base + (1 * USB_HS_MAX_BULK_PACKET));

  /* register endpoint interrupt handler */
  ep_indx = (((bulkIn & 0x0F) << 1) + 1);
  ret = pUsbApi->core->RegisterEpHandler (getHandle(), ep_indx, VCOM_bulk_in_hdlr, this);
  if (ret != LPC_OK) return;
  /* register endpoint interrupt handler */
  ep_indx = ((bulkOut & 0x0F) << 1);
  ret = pUsbApi->core->RegisterEpHandler (getHandle(), ep_indx, VCOM_bulk_out_hdlr, this);
  if (ret != LPC_OK) return;
  // resize memory
  mem.membase = cdc_param.mem_base;
  mem.memsize = cdc_param.mem_size;
  
  // asm volatile ("bkpt 0");
  usbtx_rdy = 1;
}
uint32_t CDClass::Down (char* buf, uint32_t len) {
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
  for (n=0; n<USB_HS_MAX_BULK_PACKET; n++) if (!tx.Read(ptr[n])) break;
  if  (!n) return;
  n = pUsbApi->hw->WriteEP (getHandle(), bulkIn, txBuf, n);
  usbtx_rdy = 0;
}
