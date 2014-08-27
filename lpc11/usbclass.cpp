#include "usb-desc.h"
#include "usbclass.h"

extern "C" void *memset(void *s, int c, unsigned n);

         USBD_API_T* pUsbApi = 0;
// static elements
UsbMem        UsbClass::mem;
USBD_HANDLE_T UsbClass::hUsb = 0;

extern "C" void USB_IRQHandler (void);

void USB_IRQHandler (void) {
  pUsbApi->hw->ISR (UsbClass::getHandle());
}
void UsbClass::PinInit  (void) {
  /* Enable AHB clock to the GPIO domain. */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 6);

  /* Enable AHB clock to the USB block and USB RAM. */
  LPC_SYSCON->SYSAHBCLKCTRL |= ( (0x1 << 14) | (0x1 << 27));

  /* Pull-down is needed, or internally, VBUS will be floating. This is to
  address the wrong status in VBUSDebouncing bit in CmdStatus register. It
  happens on the NXP Validation Board only that a wrong ESD protection chip is used. */
  LPC_IOCON->PIO0_3   &= ~0x1F;
//  LPC_IOCON->PIO0_3   |= ((0x1<<3)|(0x01<<0)); /* Secondary function VBUS */
  LPC_IOCON->PIO0_3   |= (0x01 << 0); /* Secondary function VBUS */
  LPC_IOCON->PIO0_6   &= ~0x07;
  LPC_IOCON->PIO0_6   |= (0x01 << 0); /* Secondary function SoftConn */
}

UsbClass::UsbClass () {
  USBD_API_INIT_PARAM_T usb_param;
  USB_CORE_DESCS_T desc;
  ErrorCode_t ret = LPC_OK;

  if (pUsbApi) return;
  /// musí být align 2048 - poslední 2 KB jsou tedy data pro ROM driver a stack 
  mem.init();
  /* get USB API table pointer */
  pUsbApi = (USBD_API_T*) ( (* (ROM **) (0x1FFF1FF8))->pUSBD);

  /* enable clocks and pinmux for usb0 */
  PinInit();

  /* initilize call back structures */
  memset (&usb_param, 0, sizeof (USBD_API_INIT_PARAM_T));
  usb_param.usb_reg_base = LPC_USB_BASE;
  // musi byt zarovnano na 2048 bytu
  usb_param.mem_base = mem.membase;
  usb_param.mem_size = mem.memsize;
  usb_param.max_num_ep = 3;             /// TODO: zmenit po pridani if na 6


  /* Initialize Descriptor pointers */
  memset (&desc, 0, sizeof (USB_CORE_DESCS_T));
  desc.device_desc     = (uint8_t*) CdcUsbDescriptors.device;
  desc.string_desc     = (uint8_t*) CdcUsbDescriptors.string;
  desc.full_speed_desc = (uint8_t*) CdcUsbDescriptors.config;
  desc.high_speed_desc = (uint8_t*) CdcUsbDescriptors.config;

  /* USB Initialization */
  ret = pUsbApi->hw->Init (&hUsb, &desc, &usb_param);
  if (ret != LPC_OK) return;
  // resize memory (static member)
  mem.membase = usb_param.mem_base;
  mem.memsize = usb_param.mem_size;
  
  //asm volatile ("bkpt 0"); // debug - skoncilo OK ?
}
// Volat jen u jednoho elementu, dediciho usbclass (slo by osetrit, ale nema to smysl)
void UsbClass::connect (void) {
  /* enable IRQ */
  NVIC_EnableIRQ (USB_IRQn); //  enable USB0 interrrupts
  /* USB Connect */
  pUsbApi->hw->Connect (hUsb, 1);
}
