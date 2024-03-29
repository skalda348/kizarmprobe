/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 * Name:    usbdesc.h
 * Purpose: USB Descriptors Definitions
 * Version: V1.20
 *----------------------------------------------------------------------------
 *      This software is supplied "AS IS" without any warranties, express,
 *      implied or statutory, including but not limited to the implied
 *      warranties of fitness for purpose, satisfactory quality and
 *      noninfringement. Keil extends you a royalty-free right to reproduce
 *      and distribute executable files created using this software for use
 *      on NXP Semiconductors LPC family microcontroller devices only. Nothing 
 *      else gives you the right to use this software.
 *
 * Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#ifndef __USBDESC_H__
#define __USBDESC_H__

#include "usb.h"

#define WBVAL(x) ((x) & 0xFF),(((x) >> 8) & 0xFF)

#define USB_DEVICE_DESC_SIZE        (sizeof(USB_DEVICE_DESCRIPTOR))
#define USB_CONFIGUARTION_DESC_SIZE (sizeof(USB_CONFIGURATION_DESCRIPTOR))
#define USB_INTERFACE_DESC_SIZE     (sizeof(USB_INTERFACE_DESCRIPTOR))
#define USB_ENDPOINT_DESC_SIZE      (sizeof(USB_ENDPOINT_DESCRIPTOR))
#define USB_BOS_DESC_SIZE	        (sizeof(USB_BOS_DESCRIPTOR))
#define USB_SS_DEVICE_CAPABILITY_SIZE	(sizeof(USB_SS_DEVICE_CAPABILITY_DESCRIPTOR))
#define USB_DEVICE_CAPABILITY_SIZE		(sizeof(USB_DEVICE_CAPABILITY_DESCRIPTOR))
#define USB_DEVICE_QUALI_SIZE       (sizeof(USB_DEVICE_QUALIFIER_DESCRIPTOR))
#define USB_INTERFACE_ASSOCIATION_DESC_SIZE (sizeof(USB_INTERFACE_ASSOCIATION_DESCRIPTOR))

#define DFU_DESC_OFFSET 		(USB_CONFIGUARTION_DESC_SIZE+USB_INTERFACE_DESC_SIZE)

// DFU+HID
#define HID_DESC_OFFSET              (USB_CONFIGUARTION_DESC_SIZE+2*USB_INTERFACE_DESC_SIZE+USB_DFU_DESCRIPTOR_SIZE)
// HID
//#define HID_DESC_OFFSET              (USB_CONFIGUARTION_DESC_SIZE+USB_INTERFACE_DESC_SIZE)

#define HID_DESC_SIZE               (sizeof(HID_DESCRIPTOR))
#define HID_REPORT_DESC_SIZE        (sizeof(HID_ReportDescriptor))

extern const uint8_t *pConfigDescriptor;
extern const uint8_t USB_dfuConfigDescriptor[];

extern const uint8_t USB_BOSDescriptor[];
extern const uint8_t USB_DeviceQualifier[];

extern const uint8_t HID_ReportDescriptor[];
extern const uint16_t HID_ReportDescSize;


#endif  /* __USBDESC_H__ */
