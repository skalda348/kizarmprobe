#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

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
  uint8_t cfg[USB_CONFIGUARTION_DESC_SIZE];
  
  uint8_t as0 [USB_INTERFACE_ASSOCIATION_DESC_SIZE];
  uint8_t if0 [USB_INTERFACE_DESC_SIZE];
  uint8_t fd0 [CDC_FUNCTION_SIZE];
  uint8_t ep1 [USB_ENDPOINT_DESC_SIZE];
  
  uint8_t if1 [USB_INTERFACE_DESC_SIZE];
  uint8_t ep2 [USB_ENDPOINT_DESC_SIZE];
  uint8_t ep82[USB_ENDPOINT_DESC_SIZE];
  
  uint8_t as1 [USB_INTERFACE_ASSOCIATION_DESC_SIZE];
  uint8_t if2 [USB_INTERFACE_DESC_SIZE];
  uint8_t fd1 [CDC_FUNCTION_SIZE];
  uint8_t ep3 [USB_ENDPOINT_DESC_SIZE];
  
  uint8_t if3 [USB_INTERFACE_DESC_SIZE];
  uint8_t ep4 [USB_ENDPOINT_DESC_SIZE];
  uint8_t ep84[USB_ENDPOINT_DESC_SIZE];
  
  uint8_t term;
};

struct UsbDescriptors {
  const uint8_t* device;
  const uint8_t* config;
  const uint8_t* string;
};

extern const struct UsbDescriptors CdcUsbDescriptors;
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
