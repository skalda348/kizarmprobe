#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
#include <stdint.h>
#include "mw_usbd.h"
#include "mw_usbd_rom_api.h"
#include "app_usbd_cfg.h"

#define CDC_FUNCTION_SIZE 0x13

struct UsbConfig {
  uint8_t cfg[USB_CONFIGUARTION_DESC_SIZE];
  uint8_t if0[USB_INTERFACE_DESC_SIZE];
  uint8_t hfd[CDC_FUNCTION_SIZE];
  uint8_t ep1[USB_ENDPOINT_DESC_SIZE];
  uint8_t if1[USB_INTERFACE_DESC_SIZE];
  
  uint8_t ep2[USB_ENDPOINT_DESC_SIZE];
  uint8_t ep3[USB_ENDPOINT_DESC_SIZE];
  
  uint8_t term;
};

struct UsbDescriptors {
  const uint8_t* device;
  const uint8_t* config;
  const uint8_t* string;
};

extern const struct UsbDescriptors CdcUsbDescriptors;
extern const struct UsbConfig      CdcConfig;

#ifdef __cplusplus
}
#endif //__cplusplus
#endif // USB_DESCRIPTORS_H