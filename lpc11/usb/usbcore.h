/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 * Name:    usbcore.h
 * Purpose: USB Core Definitions
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

#ifndef __USBCORE_H__
#define __USBCORE_H__

#include "usbcfg.h"

/* USB Endpoint Data Structure */
typedef struct _USB_EP_DATA {
  uint8_t  *pData;
  uint16_t Count;
} USB_EP_DATA;

/* USB Core Global Variables */
extern uint16_t USB_DeviceStatus;
extern uint8_t  USB_DeviceAddress;
extern uint8_t  USB_Configuration;
extern uint32_t USB_EndPointMask;
extern uint32_t USB_EndPointHalt;
extern uint32_t USB_EndPointStall;
extern uint8_t  USB_AltSetting[USB_IF_NUM];

/* USB Endpoint 0 Buffer */
//extern uint8_t  EP0Buf[USB_MAX_PACKET0];
typedef union {
  uint8_t  b[USB_MAX_PACKET0];
  uint16_t w[USB_MAX_PACKET0 / 2];
} u8x64u16x32;

extern u8x64u16x32 EP0Buf;

/* USB Endpoint 0 Data Info */
extern USB_EP_DATA EP0Data;

/* USB Setup Packet */
extern USB_SETUP_PACKET SetupPacket;

/* USB Core Functions */
extern void USB_ResetCore (void);



#endif  /* __USBCORE_H__ */
