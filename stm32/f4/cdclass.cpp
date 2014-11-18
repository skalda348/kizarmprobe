#include "cdclass.h"
//#include "gpio.h"

extern "C" {
  #include "stm32f4xx.h"
  #include "rcc.h"
  #include "usbd_cdc_core.h"
  #include "usbd_usr.h"
  #include "usbd_desc.h"
  extern USB_OTG_CORE_HANDLE USB_OTG_dev;
};
/*
static GpioClass ledg (GpioPortD, 12),
                 ledr (GpioPortD, 13);
*/
static CDClass * Instance = 0;

__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;

/* Private function prototypes -----------------------------------------------*/
uint16_t CDClass::VCP_Init   ( void ) {
    return USBD_OK;
}
uint16_t CDClass::VCP_DeInit ( void ) {
    return USBD_OK;
}
// Vycte max 64 B (Len) z fifo tx a preda do Buf.
uint16_t CDClass::VCP_DataTx ( uint8_t* Buf, uint32_t Len ) {
//  ~ledg;
    uint32_t n;
    for (n=0; n<Len; n++) {
      if (!Instance->tx.Read (Buf[n])) break;
    }
    return n;
}
uint16_t CDClass::VCP_DataRx ( uint8_t* Buf, uint32_t Len ) {
  // ~ledr;
  uint32_t  n, ofs = 0;
  while (Len) {
    n = Instance->Up ((char*) Buf + ofs, Len);
    if (!n) break; // pokud vrstva nad vraci 0, nesmi se to zacyklit
    Len -= n;
    ofs += n;
  }
  return USBD_OK;
}
extern "C" {
__attribute__((weak)) uint16_t Default_Serial_Port_Ctrl   ( uint32_t Cmd, uint8_t* Buf, uint32_t Len );
/// Tohle fakticky patří do obsluhy sériového portu. Zde nebudene obsluhovat.
                      uint16_t Default_Serial_Port_Ctrl   ( uint32_t Cmd, uint8_t* Buf, uint32_t Len ) {
  return USBD_OK;
}
/// A necháme to případně do třídy USART. Podobně jako vektory přerušení.
uint16_t Serial_Port_Ctrl           ( uint32_t Cmd, uint8_t* Buf, uint32_t Len )
      __attribute__((weak, alias("Default_Serial_Port_Ctrl")));
      
};
extern "C" const CDC_IF_Prop_TypeDef VCP_fops;

const CDC_IF_Prop_TypeDef VCP_fops = { CDClass::VCP_Init, CDClass::VCP_DeInit,
  Serial_Port_Ctrl, CDClass::VCP_DataTx, CDClass::VCP_DataRx };


CDClass::CDClass (const int port) :
  BaseLayer() , tx (0x400) {
    if (Instance) return;
    Instance = this;
}

void CDClass::connect ( void ) {
    USBD_Init ( &USB_OTG_dev,
                USB_OTG_FS_CORE_ID,
                &USR_desc,
                &USBD_CDC_cb,
                &USR_cb );
}

uint32_t CDClass::Down (char* buf, uint32_t len) {
  
  uint32_t n;
  for (n=0; n<len; n++) {
    if (!tx.Write(buf[n])) break;
  }
  return n;  
}
