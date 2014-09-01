#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H
/// @file
/// @brief Hierarchické uspořádání USB deskriptorů.

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
#include <stdint.h>
#include "usb.h"
#include "usbdesc.h"
#include "cdc.h"
/// Délka CDC
#define CDC_FUNCTION_SIZE 0x13
/// Vnitřní struktury USB driveru
extern const uint8_t USB_DeviceDescriptor[];
extern const uint8_t USB_StringDescriptor[];

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
}__attribute__((packed));

// extern const int totalDescriptorLen;
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
  const uint32_t cif;

  uint32_t (*SetLineCode)      (CDC_LINE_CODING* line_coding);
  uint32_t (*SetCtrlLineState) (uint16_t state);
  
};
struct CDCAssocField {
  const uint32_t             number;     // pocet rozhrani (2)
  const struct CDCIndividual iface [2];  // a jejich popis
};

extern const struct CDCAssocField ciAssoc;

#ifdef __cplusplus
}
#endif //__cplusplus
#endif // USB_DESCRIPTORS_H