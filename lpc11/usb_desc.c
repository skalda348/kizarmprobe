/***********************************************************************
* $Id:: cdc_desc.c 211 2011-06-17 21:34:04Z usb06052                          $
*
* Project: USB device ROM Stack test module
*
* Description:
*     USB Communication Device Class User module.
*
***********************************************************************
*   Copyright(C) 2011, NXP Semiconductor
*   All rights reserved.
*
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
**********************************************************************/
#include <string.h>
#include "usb-desc.h"

#ifdef __GNUC__
#define ALIGN4 __attribute__ ((aligned(4)))
#else // Keil
#define ALIGN4 __align(4)
#endif

/* USB Standard Device Descriptor */
ALIGN4 const uint8_t USB_DeviceDescriptor[] = {
  USB_DEVICE_DESC_SIZE,              /* bLength */
  USB_DEVICE_DESCRIPTOR_TYPE,        /* bDescriptorType */
  WBVAL(0x0200), /* 2.0 */           /* bcdUSB */
  USB_DEVICE_CLASS_COMMUNICATIONS,   /* bDeviceClass CDC*/
  0x00,                              /* bDeviceSubClass */
  0x00,                              /* bDeviceProtocol */
  USB_MAX_PACKET0,                   /* bMaxPacketSize0 */
  WBVAL(0x1FC9),                     /* idVendor */
  WBVAL(0x0003),                     /* idProduct */
  WBVAL(0x0100), /* 1.00 */          /* bcdDevice */
  0x01,                              /* iManufacturer */
  0x02,                              /* iProduct */
  0x03,                              /* iSerialNumber */
  0x01                               /* bNumConfigurations: one possible configuration*/
};

#define WTOTALLENGHT WBVAL((uint16_t)(sizeof (struct UsbConfig)-1)) 
/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
ALIGN4 const struct UsbConfig CdcConfig = {
  .cfg = {
/* Configuration 1 */
  USB_CONFIGUARTION_DESC_SIZE,       /* bLength */
  USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType */
  WTOTALLENGHT,                      /* wTotalLength */
  0x02,                              /* bNumInterfaces */
  0x01,                              /* bConfigurationValue: 0x01 is used to select this configuration */
  0x00,                              /* iConfiguration: no string to describe this configuration */
  USB_CONFIG_BUS_POWERED /*|*/       /* bmAttributes */
/*USB_CONFIG_REMOTE_WAKEUP*/,
  USB_CONFIG_POWER_MA(100)           /* bMaxPower, device power consumption is 100 mA */
  },
  .if0 = {
/* Interface 0, Alternate Setting 0, Communication class interface descriptor */
  USB_INTERFACE_DESC_SIZE,           /* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,     /* bDescriptorType */
  USB_CDC_CIF_NUM,                   /* bInterfaceNumber: Number of Interface */
  0x00,                              /* bAlternateSetting: Alternate setting */
  0x01,                              /* bNumEndpoints: One endpoint used */
  CDC_COMMUNICATION_INTERFACE_CLASS, /* bInterfaceClass: Communication Interface Class */
  CDC_ABSTRACT_CONTROL_MODEL,        /* bInterfaceSubClass: Abstract Control Model */
  0x00,                              /* bInterfaceProtocol: no protocol used */
  0x04,                              /* iInterface: */
  },
  .hfd = {
/*Header Functional Descriptor*/
  0x05,                              /* bLength: Endpoint Descriptor size */
  CDC_CS_INTERFACE,                  /* bDescriptorType: CS_INTERFACE */
  CDC_HEADER,                        /* bDescriptorSubtype: Header Func Desc */
  WBVAL(CDC_V1_10), /* 1.10 */       /* bcdCDC */
/*Call Management Functional Descriptor*/
  0x05,                              /* bFunctionLength */
  CDC_CS_INTERFACE,                  /* bDescriptorType: CS_INTERFACE */
  CDC_CALL_MANAGEMENT,               /* bDescriptorSubtype: Call Management Func Desc */
  0x01,                              /* bmCapabilities: device handles call management */
  0x01,                              /* bDataInterface: CDC data IF ID */
/*Abstract Control Management Functional Descriptor*/
  0x04,                              /* bFunctionLength */
  CDC_CS_INTERFACE,                  /* bDescriptorType: CS_INTERFACE */
  CDC_ABSTRACT_CONTROL_MANAGEMENT,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,                              /* bmCapabilities: SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported */
/*Union Functional Descriptor*/
  0x05,                              /* bFunctionLength */
  CDC_CS_INTERFACE,                  /* bDescriptorType: CS_INTERFACE */
  CDC_UNION,                         /* bDescriptorSubtype: Union func desc */
  USB_CDC_CIF_NUM,                   /* bMasterInterface: Communication class interface is master */
  USB_CDC_DIF_NUM                    /* bSlaveInterface0: Data class interface is slave 0 */
  },
  .ep1 = {
/*Endpoint 1 Descriptor*/            /* event notification (optional) */
  USB_ENDPOINT_DESC_SIZE,            /* bLength */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType */
  USB_CDC_EP_INT_IN,                /* bEndpointAddress */
  USB_ENDPOINT_TYPE_INTERRUPT,       /* bmAttributes */
  WBVAL(0x0010),                     /* wMaxPacketSize */
  0x02           /* 2ms */           /* bInterval */
  },
  .if1 = {
/* Interface 1, Alternate Setting 0, Data class interface descriptor*/
  USB_INTERFACE_DESC_SIZE,           /* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,     /* bDescriptorType */
  USB_CDC_DIF_NUM,                   /* bInterfaceNumber: Number of Interface */
  0x00,                              /* bAlternateSetting: no alternate setting */
  0x02,                              /* bNumEndpoints: two endpoints used */
  CDC_DATA_INTERFACE_CLASS,          /* bInterfaceClass: Data Interface Class */
  0x00,                              /* bInterfaceSubClass: no subclass available */
  0x00,                              /* bInterfaceProtocol: no protocol used */
  0x04                               /* iInterface: */
  },
  
  .ep2 = {
/* Endpoint, EP2 Bulk Out */
  USB_ENDPOINT_DESC_SIZE,            /* bLength */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType */
  USB_CDC_EP_BULK_OUT,               /* bEndpointAddress */
  USB_ENDPOINT_TYPE_BULK,            /* bmAttributes */
  WBVAL(USB_HS_MAX_BULK_PACKET),     /* wMaxPacketSize */
  0x00                               /* bInterval: ignore for Bulk transfer */
  },
  .ep3 = {
/* Endpoint, EP3 Bulk In */
  USB_ENDPOINT_DESC_SIZE,            /* bLength */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType */
  USB_CDC_EP_BULK_IN,                /* bEndpointAddress */
  USB_ENDPOINT_TYPE_BULK,            /* bmAttributes */
  WBVAL(USB_HS_MAX_BULK_PACKET),     /* wMaxPacketSize */
  0x00                               /* bInterval: ignore for Bulk transfer */
  },
  .term = 0
};
//const uint8_t* USB_ConfigDescriptor = (const uint8_t*) &guConfig;

extern const uint8_t USB_StringDescriptor[];
// Když to napíšeme v assemleru, nemusíme počítat délky stringů.
asm (
 "\n"
 ".set USB_STRING_DESCRIPTOR_TYPE, 3\n\t"
 ".global USB_StringDescriptor\n\t"
 ".global l_StringDescriptor\n\t"
 ".section  .rodata.VCOM_StringDescriptor,\"a\",%progbits\n\t"
 ".align 4\n"

// Index 0x00: LANGID Codes
 "USB_StringDescriptor:\n\t"
 ".byte  (label_1 - .)\n\t"       // <- délka následujícího pole, v C problém
 ".byte  USB_STRING_DESCRIPTOR_TYPE\n\t"
 ".byte  0x09, 0x04\n"
// Index 0x01:  Manufacturer string 
 "label_1:\n\t"                   // počítá se za překladu k této značce
 ".byte  (label_2 - .)\n\t"
 ".byte  USB_STRING_DESCRIPTOR_TYPE\n\t"
 ".byte  'M,0,'r,0,'a,0,'z,0,'i,0,'k,0,' ,0,'l,0,'a,0,'b,0,'s,0,'.,0\n"
// Index 0x02: Product 
 "label_2:\n\t"
 ".byte  (label_3 - .)\n\t"
 ".byte  USB_STRING_DESCRIPTOR_TYPE\n\t"
 ".byte  'K,0,'i,0,'z,0,'a,0,'r,0,'m,0,' ,0,'P,0,'r,0,'o,0,'b,0,'e,0,' ,0,'1,0,'.,0,'0,0\r\n"
// Index 0x03: Serial Number 
 "label_3:\n\t"
 ".byte  (label_4 - .)\n\t"
 ".byte  USB_STRING_DESCRIPTOR_TYPE\n\t"
 ".byte  '0,0,'0,0,'0,0,'0,0,'2,0\n"
// Index 0x04: Interface 0, Alternate Setting 0     
 "label_4:\n\t"
 ".byte  (label_5 - .)\n\t"
 ".byte  USB_STRING_DESCRIPTOR_TYPE\n\t"
 ".byte  'C,0,'D,0,'C,0\n"
 
 "label_5:\n\t"
 "l_StringDescriptor:\n"
 ".word . - USB_StringDescriptor\n\t"
);

#define ALIAS(f) __attribute__ ((weak, alias (#f)))

ErrorCode_t iA0LineCode  (USBD_HANDLE_T hCDC, CDC_LINE_CODING* line_coding) ALIAS (SetLineCodeDefaultHandler);
ErrorCode_t iA0LineState (USBD_HANDLE_T hCDC, uint16_t state)               ALIAS (SetCtrlLineStateDefaultHandler);

ErrorCode_t iA1LineCode  (USBD_HANDLE_T hCDC, CDC_LINE_CODING* line_coding) ALIAS (SetLineCodeDefaultHandler);
ErrorCode_t iA1LineState (USBD_HANDLE_T hCDC, uint16_t state)               ALIAS (SetCtrlLineStateDefaultHandler);

const struct UsbDescriptors CdcUsbDescriptors = {
  .device = USB_DeviceDescriptor,
  .config = (const uint8_t*) &CdcConfig,
  .string = USB_StringDescriptor
};

const struct CDCIndividual iAssoc0 = {
  .depth = 0x400,
  .ep    = 2,
  .if0   = CdcConfig.if0,
  .if1   = CdcConfig.if1,
  .SetLineCode      = iA0LineCode,
  .SetCtrlLineState = iA0LineState
  
};
/*
const struct CDCIndividual iAssoc1 = {
  .depth = 0x40,
  .ep    = 4,
  .if0   = CdcConfig.if2,
  .if1   = CdcConfig.if3,
  .SetLineCode      = iA1LineCode,
  .SetCtrlLineState = iA1LineState
  
};
*/
/// Lze uzivatelsky predefinovat podobne jako vektory.
ErrorCode_t SetLineCodeDefaultHandler      (USBD_HANDLE_T hCDC, CDC_LINE_CODING* line_coding) {
  return LPC_OK;
}
ErrorCode_t SetCtrlLineStateDefaultHandler (USBD_HANDLE_T hCDC, uint16_t state) {
  return LPC_OK;
}
