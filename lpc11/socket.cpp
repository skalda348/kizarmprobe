#include <string.h>
#include "usb-desc.h"
#include "socket.h"

/// Dáno výrobcem
extern USBD_API_T* pUsbApi;

volatile int Socket::wait = 0;

ErrorCode_t Socket::VCOM_bulk_in_hdlr (USBD_HANDLE_T hUsb, void* data, uint32_t event) {
  Socket* pVcom = (Socket*) data;
  if (event == USB_EVT_IN) {    // přerušení od vysílače - předešlá data odeslána
    pVcom->usbtx_rdy = 1;       // takže mohou být poslána další
    pVcom->Send();
  }
  return LPC_OK;
}
ErrorCode_t Socket::VCOM_bulk_out_hdlr (USBD_HANDLE_T hUsb, void* data, uint32_t event) {
  Socket* pVcom = (Socket*) data;
  uint32_t  rlen, n, ofsb = 0;
  if (event == USB_EVT_OUT) {
    rlen = pUsbApi->hw->ReadEP (hUsb, USB_CDC_EP_BULK_OUT, pVcom->rxBuf);
    while (rlen) {
      n = pVcom->Up ((char*)pVcom->rxBuf + ofsb, rlen);
      rlen -= n;
      ofsb += n;
    }
  }
  return LPC_OK;
}
ErrorCode_t Socket::VCOM_SetLineCode (USBD_HANDLE_T hCDC, CDC_LINE_CODING* line_coding) {
  Socket::wait = 1;
#ifdef USE_USART
  // Uživatelská funkce
  if (Usart1::inst)
    Usart1::UsbInit(line_coding);
#endif //USE_USART
  return LPC_OK;
}
ErrorCode_t Socket::VCOM_SetCtrlLineState (USBD_HANDLE_T hCDC, uint16_t state) {
  return LPC_OK;
}

Socket::Socket () : UsbClass (), BaseLayer(), tx() {
  const uint8_t* if0 = CdcConfig.if0;
  const uint8_t* if1 = CdcConfig.if1;
  USBD_CDC_INIT_PARAM_T cdc_param;
  ErrorCode_t ret = LPC_OK;
  uint32_t ep_indx;
  /* init CDC params */
  memset (&cdc_param, 0, sizeof (USBD_CDC_INIT_PARAM_T));

  /* user defined functions */
  cdc_param.SetLineCode      = VCOM_SetLineCode;
  cdc_param.SetCtrlLineState = VCOM_SetCtrlLineState;
  // cdc_param.SendBreak = VCOM_SendBreak;
  // init CDC params
  cdc_param.mem_base = mem.base();
  cdc_param.mem_size = mem.size();
  cdc_param.cif_intf_desc = (uint8_t*) if0; 
  cdc_param.dif_intf_desc = (uint8_t*) if1;
  ret = pUsbApi->cdc->init (getHandle(), &cdc_param, &hCdc);

  if (ret != LPC_OK) return;

  /* allocate transfer buffers */
  rxBuf = (uint8_t*) (cdc_param.mem_base + (0 * USB_HS_MAX_BULK_PACKET));
  txBuf = (uint8_t*) (cdc_param.mem_base + (1 * USB_HS_MAX_BULK_PACKET));

  /* register endpoint interrupt handler */
  ep_indx = ( ( (USB_CDC_EP_BULK_IN & 0x0F) << 1) + 1);
  ret = pUsbApi->core->RegisterEpHandler (getHandle(), ep_indx, VCOM_bulk_in_hdlr, this);
  if (ret != LPC_OK) return;
  /* register endpoint interrupt handler */
  ep_indx = ( (USB_CDC_EP_BULK_OUT & 0x0F) << 1);
  ret = pUsbApi->core->RegisterEpHandler (getHandle(), ep_indx, VCOM_bulk_out_hdlr, this);
  if (ret != LPC_OK) return;
  // spočti kolik USB paměti zbylo
  mem.resize(cdc_param.mem_size);
  
  usbtx_rdy = 1;
}
void* Socket::Receiver (void) {
  return NULL;
}


void Socket::Init (void) {
  connect ();
  while   (WaitForLineCode()) __WFI();
}
bool Socket::Fini (void) {
  return true;
}

uint32_t Socket::Down (char *buf, uint32_t len) {
  uint32_t n;
  for (n=0; n<len; n++) {
    if (!tx.Write(buf[n])) break;
  }
  Send ();
  return n;
}
void Socket::Send (void) {
  char* ptr = (char*) txBuf;
  uint32_t n;
  
  if  (!usbtx_rdy) return;
  for (n=0; n<USB_HS_MAX_BULK_PACKET; n++) if (!tx.Read(ptr[n])) break;
  if  (!n) return;
  n = pUsbApi->hw->WriteEP (getHandle(), USB_CDC_EP_BULK_IN, txBuf, n);
  usbtx_rdy = 0;
}

