#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H
/// @file
/// @brief Hierarchické uspořádání USB deskriptorů.

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
#include <stdint.h>
#include "mw_usbd.h"
#include "mw_usbd_rom_api.h"
#include "app_usbd_cfg.h"
/// Délka CDC
#define CDC_FUNCTION_SIZE 0x13
/// Vnitřní struktury USB driveru
struct UsbConfig {
  /// konfigurace
  uint8_t cfg[USB_CONFIGUARTION_DESC_SIZE];
  /// Interface 0
  uint8_t if0[USB_INTERFACE_DESC_SIZE];
  /// Něco divného
  uint8_t hfd[CDC_FUNCTION_SIZE];
  /// Endpoint 1
  uint8_t ep1[USB_ENDPOINT_DESC_SIZE];
  /// Interface 1
  uint8_t if1[USB_INTERFACE_DESC_SIZE];
  
  /// Endpoint 2
  uint8_t ep2[USB_ENDPOINT_DESC_SIZE];
  /// Endpoint 3
  uint8_t ep3[USB_ENDPOINT_DESC_SIZE];
  /// Ukončení
  uint8_t term;
};
/// Deskriptory USB systému
struct UsbDescriptors {
  /// Device deskriptor
  const uint8_t* device;
  /// Config deskriptor
  const uint8_t* config;
  /// String deskriptor
  const uint8_t* string;
};
/// Jsou definovány v usb-desc.c
extern const struct UsbDescriptors CdcUsbDescriptors;
/// Jsou definovány v usb-desc.c
extern const struct UsbConfig      CdcConfig;


/** ************************************************************************/
struct CDCIndividual {
  const uint32_t depth;
  const uint32_t ep;
  const uint8_t* if0;
  const uint8_t* if1;

  ErrorCode_t (*SetLineCode)      (USBD_HANDLE_T hCDC, CDC_LINE_CODING* line_coding);
  ErrorCode_t (*SetCtrlLineState) (USBD_HANDLE_T hCDC, uint16_t state);
  
};

extern const struct CDCIndividual iAssoc0;
extern const struct CDCIndividual iAssoc1;

#ifdef __cplusplus
}
#endif //__cplusplus
#endif // USB_DESCRIPTORS_H