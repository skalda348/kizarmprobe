/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 * Name:    usbcore.c
 * Purpose: USB Core Module
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
 *----------------------------------------------------------------------------
 * History:
 *          V1.20 Added vendor specific requests
 *                Changed string descriptor handling
 *                Reworked Endpoint0
 *          V1.00 Initial Version
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#if DEBUG
  #define debug printf
#else
  #define debug(...)
#endif

#include "type.h"

#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#ifdef SERIAL
  #include "comp_desc.h"
#else
  #include "usb-desc.h"
#endif // SERIAL
#include "usbuser.h"

#if (USB_CLASS)

#if (USB_AUDIO)
#include "audio.h"
#include "adcuser.h"
#endif

#if (USB_HID)
#include "hid.h"
#include "hiduser.h"
#endif

#if (USB_MSC)
#include "msc.h"
#include "mscuser.h"
extern MSC_CSW CSW;
#endif

#if (USB_CDC)
#include "cdc.h"
#include "cdclass.h"
#endif

#if (USB_UVC)
#include "uvc.h"
#include "uvcuser.h"
#endif

#if (USB_DFU)
#include "dfu.h"
#include "dfuuser.h"
extern uint32_t DFUDownloadDone;
extern uint8_t  DFUState;                /* DFU State */
extern uint8_t  DFUStatus;               /* DFU Status */
extern uint8_t  DFUBuffer[USB_DFU_XFER_SIZE];
extern uint8_t *pDfuBuffer;
extern DFU_STATUS_T DFUGetStatusReq;
extern USB_DFU_FUNC_DESCRIPTOR DFU_FunctionDescriptor;
#endif

#endif

#if (USB_VENDOR)
#include "vendor.h"
#endif

//#pragma diag_suppress 111,177,1441


uint16_t  USB_DeviceStatus;
uint8_t  USB_DeviceAddress;
uint8_t  USB_Configuration;
uint32_t USB_EndPointMask;
uint32_t USB_EndPointHalt;
uint32_t USB_EndPointStall;                         /* EP must stay stalled */
uint8_t  USB_NumInterfaces;
uint8_t  USB_AltSetting[USB_IF_NUM];

//uint8_t  EP0Buf[USB_MAX_PACKET0];
u8x64u16x32 EP0Buf;
USB_EP_DATA EP0Data;

USB_SETUP_PACKET SetupPacket;

volatile int UsbIsConfigured = 0;

/*
 *  Reset USB Core
 *    Parameters:      None
 *    Return Value:    None
 */
void USB_ResetCore (void) {

  USB_DeviceStatus  = USB_POWER;
  USB_DeviceAddress = 0;
  USB_Configuration = 0;
  USB_EndPointMask  = 0x00010001;
  USB_EndPointHalt  = 0x00000000;
  USB_EndPointStall = 0x00000000;
#if USB_CLASS
#if USB_DFU
  DFU_Reset();
#endif
#endif
}


/*
 *  USB Request - Setup Stage
 *    Parameters:      None (global SetupPacket)
 *    Return Value:    None
 */
void USB_SetupStage (void) {
  USB_ReadSetupEP (0x00, (uint8_t *) &SetupPacket);
}


/*
 *  USB Request - Data In Stage
 *    Parameters:      None (global EP0Data)
 *    Return Value:    None
 */
void USB_DataInStage (void) {
  uint32_t cnt;

  if (EP0Data.Count > USB_MAX_PACKET0) {
    cnt = USB_MAX_PACKET0;
  } else {
    cnt = EP0Data.Count;
  }
  cnt = USB_WriteEP (0x80, EP0Data.pData, cnt);
  EP0Data.pData += cnt;
  EP0Data.Count -= cnt;
}


/*
 *  USB Request - Data Out Stage
 *    Parameters:      None (global EP0Data)
 *    Return Value:    None
 */
void USB_DataOutStage (void) {
  uint32_t cnt;

  cnt = USB_ReadEP (0x00, EP0Data.pData);
  EP0Data.pData += cnt;
  EP0Data.Count -= cnt;
}


/*
 *  USB Request - Status In Stage
 *    Parameters:      None
 *    Return Value:    None
 */
void USB_StatusInStage (void) {
  USB_WriteEP (0x80, NULL, 0);
}


/*
 *  USB Request - Status Out Stage
 *    Parameters:      None
 *    Return Value:    None
 */
void USB_StatusOutStage (void) {
  USB_ReadEP (0x00, EP0Buf.b);
}


/*
 *  Get Status USB Request
 *    Parameters:      None (global SetupPacket)
 *    Return Value:    TRUE - Success, FALSE - Error
 */

__inline uint32_t USB_ReqGetStatus (void) {
  uint32_t n, m;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      EP0Data.pData = (uint8_t *) &USB_DeviceStatus;
      break;
    case REQUEST_TO_INTERFACE:
      if ( (USB_Configuration != 0) && (SetupPacket.wIndex.WB.L < USB_NumInterfaces)) {
        *EP0Buf.w = 0;
        EP0Data.pData = EP0Buf.b;
      } else {
        return (FALSE);
      }
      break;
    case REQUEST_TO_ENDPOINT:
      n = SetupPacket.wIndex.WB.L & 0x8F;
      m = (n & 0x80) ? ( (1 << 16) << (n & 0x0F)) : (1 << n);
      if ( ( (USB_Configuration != 0) || ( (n & 0x0F) == 0)) && (USB_EndPointMask & m)) {
        *EP0Buf.w = (USB_EndPointHalt & m) ? 1 : 0;
        EP0Data.pData = EP0Buf.b;
      } else {
        return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


/*
 *  Set/Clear Feature USB Request
 *    Parameters:      sc:    0 - Clear, 1 - Set
 *                            (global SetupPacket)
 *    Return Value:    TRUE - Success, FALSE - Error
 */
__inline uint32_t USB_ReqSetClrFeature (uint32_t sc) {
  uint32_t n, m;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      if (SetupPacket.wValue.W == USB_FEATURE_REMOTE_WAKEUP) {
        if (sc) {
          USB_WakeUpCfg (TRUE);
          USB_DeviceStatus |=  USB_GETSTATUS_REMOTE_WAKEUP;
        } else {
          USB_WakeUpCfg (FALSE);
          USB_DeviceStatus &= ~USB_GETSTATUS_REMOTE_WAKEUP;
        }
      } else {
        return (FALSE);
      }
      break;
    case REQUEST_TO_INTERFACE:
      return (FALSE);
    case REQUEST_TO_ENDPOINT:
      n = SetupPacket.wIndex.WB.L & 0x8F;
      m = (n & 0x80) ? ( (1 << 16) << (n & 0x0F)) : (1 << n);
      if ( (USB_Configuration != 0) && ( (n & 0x0F) != 0) && (USB_EndPointMask & m)) {
        if (SetupPacket.wValue.W == USB_FEATURE_ENDPOINT_STALL) {
          if (sc) {
            USB_SetStallEP (n);
            USB_EndPointHalt |=  m;
          } else {
            if ( (USB_EndPointStall & m) != 0) {
              return (TRUE);
            }
            USB_ClrStallEP (n);
            USB_EndPointHalt &= ~m;
          }
        } else {
          return (FALSE);
        }
      } else {
        return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


/*
 *  Set Address USB Request
 *    Parameters:      None (global SetupPacket)
 *    Return Value:    TRUE - Success, FALSE - Error
 */
__inline uint32_t USB_ReqSetAddress (void) {

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      USB_DeviceAddress = 0x80 | SetupPacket.wValue.WB.L;
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


/*
 *  Get Descriptor USB Request
 *    Parameters:      None (global SetupPacket)
 *    Return Value:    TRUE - Success, FALSE - Error
 */
__inline uint32_t USB_ReqGetDescriptor (void) {
  uint8_t  *pD;
  uint32_t len, n;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      switch (SetupPacket.wValue.WB.H) {
        case USB_DEVICE_DESCRIPTOR_TYPE:
          EP0Data.pData = (uint8_t *) USB_DeviceDescriptor;
          len = USB_DEVICE_DESC_SIZE;
          break;
        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
          pD = (uint8_t *) pConfigDescriptor/*USB_ConfigDescriptor*/;
          for (n = 0; n != SetupPacket.wValue.WB.L; n++) {
            if ( ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->bLength != 0) {
              pD += ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->wTotalLength;
            }
          }
          if ( ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->bLength == 0) {
            return (FALSE);
          }
          EP0Data.pData = pD;
          len = ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->wTotalLength;
          break;
#if LPM_SUPPORT
        case USB_BOS_TYPE:
          pD = (uint8_t *) USB_BOSDescriptor;
          for (n = 0; n != SetupPacket.wValue.WB.L; n++) {
            if ( ( (USB_BOS_DESCRIPTOR *) pD)->bLength != 0) {
              pD += ( (USB_BOS_DESCRIPTOR *) pD)->wTotalLength;
            }
          }
          if ( ( (USB_BOS_DESCRIPTOR *) pD)->bLength == 0) {
            return (FALSE);
          }
          EP0Data.pData = pD;
          len = ( (USB_BOS_DESCRIPTOR *) pD)->wTotalLength;
          break;
#endif
        case USB_STRING_DESCRIPTOR_TYPE:
          pD = (uint8_t *) USB_StringDescriptor;
          for (n = 0; n != SetupPacket.wValue.WB.L; n++) {
            if ( ( (USB_STRING_DESCRIPTOR *) pD)->bLength != 0) {
              pD += ( (USB_STRING_DESCRIPTOR *) pD)->bLength;
            }
          }
          if ( ( (USB_STRING_DESCRIPTOR *) pD)->bLength == 0) {
            return (FALSE);
          }
          EP0Data.pData = pD;
          len = ( (USB_STRING_DESCRIPTOR *) EP0Data.pData)->bLength;
          break;
        case USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE:
          /* USB Chapter 9. page 9.6.2 */
          /* This is a tricky one again. For USBCV compliance test,
          if we support LPM, bcdUSB should be 0x0201, the USBCV will ask
          for device qualifier info. If the device doesn't reply(return false),
          USBCV will fail. However, if the device replies, on the WIN XP system,
          the host would think it's a HS device attached. */
          pD = (uint8_t *) USB_DeviceDescriptor;
          //EP0Data.pData = (uint8_t *) USB_DeviceQualifier;    // TODO !!!
          len = USB_DEVICE_QUALI_SIZE;
          break;
        default:
          return (FALSE);
      }
      break;
    case REQUEST_TO_INTERFACE:
      switch (SetupPacket.wValue.WB.H) {
#if USB_HID
        case HID_HID_DESCRIPTOR_TYPE:
          if (SetupPacket.wIndex.WB.L != USB_HID_IF_NUM) {
            return (FALSE);    /* Only Single HID Interface is supported */
          }
          EP0Data.pData = (uint8_t *) pConfigDescriptor/*USB_ConfigDescriptor*/ + HID_DESC_OFFSET;
          len = HID_DESC_SIZE;
          break;
        case HID_REPORT_DESCRIPTOR_TYPE:
          if (SetupPacket.wIndex.WB.L != USB_HID_IF_NUM) {
            return (FALSE);    /* Only Single HID Interface is supported */
          }
          EP0Data.pData = (uint8_t *) HID_ReportDescriptor;
          len = HID_ReportDescSize;
          break;
        case HID_PHYSICAL_DESCRIPTOR_TYPE:
          return (FALSE);      /* HID Physical Descriptor is not supported */
#endif
#if 0 //USB_DFU
        case USB_DFU_DESCRIPTOR_TYPE:
          if (SetupPacket.wIndex.WB.L != USB_DFU_IF_NUM) {
            return (FALSE);    /* Only Single DFU Interface is supported */
          }
          EP0Data.pData = (uint8_t *) &DFU_FunctionDescriptor;
          EP0Data.Count = USB_DFU_DESCRIPTOR_SIZE;
          break;
#endif
        default:
          return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }

  if (EP0Data.Count > len) {
    EP0Data.Count = len;
  }

  return (TRUE);
}


/*
 *  Get Configuration USB Request
 *    Parameters:      None (global SetupPacket)
 *    Return Value:    TRUE - Success, FALSE - Error
 */

__inline uint32_t USB_ReqGetConfiguration (void) {

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:
      EP0Data.pData = &USB_Configuration;
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}

static inline void* AddOffset (void* p, uint32_t o) {
  uint8_t * op = p;
  op += o;
  return (void*) op;
}
/*
 *  Set Configuration USB Request
 *    Parameters:      None (global SetupPacket)
 *    Return Value:    TRUE - Success, FALSE - Error
 */
__inline uint32_t USB_ReqSetConfiguration (void) {
  USB_COMMON_DESCRIPTOR *pD;
  uint32_t alt = 0;
  uint32_t n, m;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_DEVICE:

      if (SetupPacket.wValue.WB.L) {

//          pD = (USB_COMMON_DESCRIPTOR *)USB_ConfigDescriptor;
        pD = (USB_COMMON_DESCRIPTOR *) pConfigDescriptor;

        while (pD->bLength) {
          switch (pD->bDescriptorType) {
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:

              if ( ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->bConfigurationValue == SetupPacket.wValue.WB.L) {
                USB_Configuration = SetupPacket.wValue.WB.L;
                USB_NumInterfaces = ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->bNumInterfaces;
                for (n = 0; n < USB_IF_NUM; n++) {
                  USB_AltSetting[n] = 0;
                }

                for (n = 1; n < USB_LOGIC_EP_NUM; n++) {
                  if (USB_EndPointMask & (1 << n)) {
                    USB_DisableEP (n);
                  }
                  if (USB_EndPointMask & ( (1 << 16) << n)) {
                    USB_DisableEP (n | 0x80);
                  }
                }
                USB_EndPointMask = 0x00010001;
                USB_EndPointHalt = 0x00000000;
                USB_EndPointStall= 0x00000000;
                USB_Configure (TRUE);
                UsbIsConfigured = 1;
                if ( ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->bmAttributes & USB_CONFIG_POWERED_MASK) {
                  USB_DeviceStatus |=  USB_GETSTATUS_SELF_POWERED;
                } else {
                  USB_DeviceStatus &= ~USB_GETSTATUS_SELF_POWERED;
                }
              } else {
                //(uint8_t *) pD += ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->wTotalLength;
                pD = AddOffset (pD, ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->wTotalLength);
                continue;
              }
              break;
            case USB_INTERFACE_DESCRIPTOR_TYPE:
              alt = ( (USB_INTERFACE_DESCRIPTOR *) pD)->bAlternateSetting;
              break;
            case USB_ENDPOINT_DESCRIPTOR_TYPE:
              if (alt == 0) {
                n = ( (USB_ENDPOINT_DESCRIPTOR *) pD)->bEndpointAddress & 0x8F;
                m = (n & 0x80) ? ( (1 << 16) << (n & 0x0F)) : (1 << n);
                USB_EndPointMask |= m;
                USB_ConfigEP ( (USB_ENDPOINT_DESCRIPTOR *) pD);
                USB_EnableEP (n);
                USB_ResetEP (n);
              }
              break;
          }
          //(uint8_t *) pD += pD->bLength;
          pD = AddOffset (pD, pD->bLength);
        }
      } else {
        USB_Configuration = 0;
        for (n = 1; n < USB_LOGIC_EP_NUM; n++) {
          if (USB_EndPointMask & (1 << n)) {
            USB_DisableEP (n);
          }
          if (USB_EndPointMask & ( (1 << 16) << n)) {
            USB_DisableEP (n | 0x80);
          }
        }
        USB_EndPointMask  = 0x00010001;
        USB_EndPointHalt  = 0x00000000;
        USB_EndPointStall = 0x00000000;
        USB_Configure (FALSE);
      }

      if (USB_Configuration != SetupPacket.wValue.WB.L) {
        return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


/*
 *  Get Interface USB Request
 *    Parameters:      None (global SetupPacket)
 *    Return Value:    TRUE - Success, FALSE - Error
 */

__inline uint32_t USB_ReqGetInterface (void) {

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_INTERFACE:
      if ( (USB_Configuration != 0) && (SetupPacket.wIndex.WB.L < USB_NumInterfaces)) {
        EP0Data.pData = USB_AltSetting + SetupPacket.wIndex.WB.L;
      } else {
        return (FALSE);
      }
      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}


/*
 *  Set Interface USB Request
 *    Parameters:      None (global SetupPacket)
 *    Return Value:    TRUE - Success, FALSE - Error
 */

__inline uint32_t USB_ReqSetInterface (void) {
  USB_COMMON_DESCRIPTOR *pD;
  uint32_t ifn = 0, alt = 0, old = 0, msk = 0;
  uint32_t n, m;
  uint32_t set;

  switch (SetupPacket.bmRequestType.BM.Recipient) {
    case REQUEST_TO_INTERFACE:
      if (USB_Configuration == 0) return (FALSE);
      set = FALSE;
      pD  = (USB_COMMON_DESCRIPTOR *) pConfigDescriptor/*USB_ConfigDescriptor*/;
      while (pD->bLength) {
        switch (pD->bDescriptorType) {
          case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            if ( ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->bConfigurationValue != USB_Configuration) {
              //(uint8_t *) pD += ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->wTotalLength;
              pD = AddOffset (pD, ( (USB_CONFIGURATION_DESCRIPTOR *) pD)->wTotalLength);
              continue;
            }
            break;
          case USB_INTERFACE_DESCRIPTOR_TYPE:
            ifn = ( (USB_INTERFACE_DESCRIPTOR *) pD)->bInterfaceNumber;
            alt = ( (USB_INTERFACE_DESCRIPTOR *) pD)->bAlternateSetting;
            msk = 0;
            if ( (ifn == SetupPacket.wIndex.WB.L) && (alt == SetupPacket.wValue.WB.L)) {
              set = TRUE;
              old = USB_AltSetting[ifn];
              USB_AltSetting[ifn] = (uint8_t) alt;
            }
            break;
          case USB_ENDPOINT_DESCRIPTOR_TYPE:
            if (ifn == SetupPacket.wIndex.WB.L) {
              n = ( (USB_ENDPOINT_DESCRIPTOR *) pD)->bEndpointAddress & 0x8F;
              m = (n & 0x80) ? ( (1 << 16) << (n & 0x0F)) : (1 << n);
              if (alt == SetupPacket.wValue.WB.L) {
                USB_EndPointMask |=  m;
                USB_EndPointHalt &= ~m;
                USB_ConfigEP ( (USB_ENDPOINT_DESCRIPTOR *) pD);
                USB_EnableEP (n);
                USB_ResetEP (n);
                msk |= m;
              } else if ( (alt == old) && ( (msk & m) == 0)) {
                USB_EndPointMask &= ~m;
                USB_EndPointHalt &= ~m;
                USB_DisableEP (n);
              }
            }
            break;
        }
        //(uint8_t *) pD += pD->bLength;
        pD = AddOffset (pD, pD->bLength);
      }
      break;
    default:
      return (FALSE);
  }

  return (set);
}

static inline void FillLineCode (CDC_LINE_CODING * lc) {
  lc->dwDTERate   =  (EP0Buf.b[0] <<  0)
                   | (EP0Buf.b[1] <<  8)
                   | (EP0Buf.b[2] << 16)
                   | (EP0Buf.b[3] << 24); 
  lc->bCharFormat =  EP0Buf.b[4];
  lc->bParityType =  EP0Buf.b[5];
  lc->bDataBits   =  EP0Buf.b[6];
}
/*
 *  USB Endpoint 0 Event Callback
 *    Parameters:      event
 *    Return Value:    none
 */

void USB_EndPoint0 (void* d, uint32_t event) {
#if USB_CDC
   CDC_LINE_CODING tlc;
   uint32_t        ifaceno = 0;
#endif

#if USB_DFU
  uint32_t eotflag = 0;
#endif
  switch (event) {
    case USB_EVT_SETUP:
      USB_SetupStage();
      USB_DirCtrlEP (SetupPacket.bmRequestType.BM.Dir);
      EP0Data.Count = SetupPacket.wLength;     /* Number of bytes to transfer */
      switch (SetupPacket.bmRequestType.BM.Type) {

        case REQUEST_STANDARD:
          switch (SetupPacket.bRequest) {
            case USB_REQUEST_GET_STATUS:
              if (!USB_ReqGetStatus()) {
                goto stall_i;
              }
              USB_DataInStage();
              break;

            case USB_REQUEST_CLEAR_FEATURE:
              if (!USB_ReqSetClrFeature (0)) {
                goto stall_i;
              }
              USB_StatusInStage();
#if USB_FEATURE_EVENT
              USB_Feature_Event();
#endif
              break;

            case USB_REQUEST_SET_FEATURE:
              if (!USB_ReqSetClrFeature (1)) {
                goto stall_i;
              }
              USB_StatusInStage();
#if USB_FEATURE_EVENT
              USB_Feature_Event();
#endif
              break;

            case USB_REQUEST_SET_ADDRESS:
              if (!USB_ReqSetAddress()) {
                goto stall_i;
              }
              USB_StatusInStage();
              break;

            case USB_REQUEST_GET_DESCRIPTOR:
              if (!USB_ReqGetDescriptor()) {
                goto stall_i;
              }
              USB_DataInStage();
              break;

            case USB_REQUEST_SET_DESCRIPTOR:
              /*stall_o:*/
              USB_SetStallEP (0x00);           /* not supported */
              EP0Data.Count = 0;
              break;

            case USB_REQUEST_GET_CONFIGURATION:
              if (!USB_ReqGetConfiguration()) {
                goto stall_i;
              }
              USB_DataInStage();
              break;

            case USB_REQUEST_SET_CONFIGURATION:
              if (!USB_ReqSetConfiguration()) {
                goto stall_i;
              }
              USB_StatusInStage();
#if USB_CONFIGURE_EVENT
              USB_Configure_Event();
#endif
              break;

            case USB_REQUEST_GET_INTERFACE:
              if (!USB_ReqGetInterface()) {
                goto stall_i;
              }
              USB_DataInStage();
              break;

            case USB_REQUEST_SET_INTERFACE:
              if (!USB_ReqSetInterface()) {
                goto stall_i;
              }
              USB_StatusInStage();
#if USB_INTERFACE_EVENT
              USB_Interface_Event();
#endif
              break;

            default:
              goto stall_i;
          }
          break;  /* end case REQUEST_STANDARD */
#if USB_CLASS
        case REQUEST_CLASS:
          switch (SetupPacket.bmRequestType.BM.Recipient) {

            case REQUEST_TO_DEVICE:
              goto stall_i;                                              /* not supported */

            case REQUEST_TO_INTERFACE:
#if USB_HID
              if (SetupPacket.wIndex.WB.L == USB_HID_IF_NUM) {           /* IF number correct? */
                switch (SetupPacket.bRequest) {
                  case HID_REQUEST_GET_REPORT:
                    if (HID_GetReport()) {
                      EP0Data.pData = EP0Buf.b;                            /* point to data to be sent */
                      USB_DataInStage();                                 /* send requested data */
                      goto setup_class_ok;
                    }
                    break;
                  case HID_REQUEST_SET_REPORT:
                    EP0Data.pData = EP0Buf.b;                              /* data to be received */
                    goto setup_class_ok;
                  case HID_REQUEST_GET_IDLE:
                    if (HID_GetIdle()) {
                      EP0Data.pData = EP0Buf.b;                            /* point to data to be sent */
                      USB_DataInStage();                                 /* send requested data */
                      goto setup_class_ok;
                    }
                    break;
                  case HID_REQUEST_SET_IDLE:
                    if (HID_SetIdle()) {
                      USB_StatusInStage();                               /* send Acknowledge */
                      goto setup_class_ok;
                    }
                    break;
                  case HID_REQUEST_GET_PROTOCOL:
                    if (HID_GetProtocol()) {
                      EP0Data.pData = EP0Buf.b;                            /* point to data to be sent */
                      USB_DataInStage();                                 /* send requested data */
                      goto setup_class_ok;
                    }
                    break;
                  case HID_REQUEST_SET_PROTOCOL:
                    if (HID_SetProtocol()) {
                      USB_StatusInStage();                               /* send Acknowledge */
                      goto setup_class_ok;
                    }
                    break;
                }
              }
#endif  /* USB_HID */
#if USB_MSC
              if (SetupPacket.wIndex.WB.L == USB_MSC_IF_NUM) {           /* IF number correct? */
                switch (SetupPacket.bRequest) {
                  case MSC_REQUEST_RESET:
                    if ( (SetupPacket.wValue.W == 0) &&                  /* RESET with invalid parameters -> STALL */
                         (SetupPacket.wLength  == 0)) {
                      if (MSC_Reset()) {
                        USB_StatusInStage();
                        goto setup_class_ok;
                      }
                    }
                    break;
                  case MSC_REQUEST_GET_MAX_LUN:
                    if ( (SetupPacket.wValue.W == 0) &&                  /* GET_MAX_LUN with invalid parameters -> STALL */
                         (SetupPacket.wLength  == 1)) {
                      if (MSC_GetMaxLUN()) {
                        EP0Data.pData = EP0Buf.b;
                        USB_DataInStage();
                        goto setup_class_ok;
                      }
                    }
                    break;
                }
              }
#endif  /* USB_MSC */
#if USB_AUDIO
              if ( (SetupPacket.wIndex.WB.L == USB_ADC_CIF_NUM)  ||      /* IF number correct? */
                   (SetupPacket.wIndex.WB.L == USB_ADC_SIF1_NUM) ||
                   (SetupPacket.wIndex.WB.L == USB_ADC_SIF2_NUM)) {
                switch (SetupPacket.bRequest) {
                  case AUDIO_REQUEST_GET_CUR:
                  case AUDIO_REQUEST_GET_MIN:
                  case AUDIO_REQUEST_GET_MAX:
                  case AUDIO_REQUEST_GET_RES:
                    if (ADC_IF_GetRequest()) {
                      EP0Data.pData = EP0Buf.b;                            /* point to data to be sent */
                      USB_DataInStage();                                 /* send requested data */
                      goto setup_class_ok;
                    }
                    break;
                  case AUDIO_REQUEST_SET_CUR:
//                case AUDIO_REQUEST_SET_MIN:
//                case AUDIO_REQUEST_SET_MAX:
//                case AUDIO_REQUEST_SET_RES:
                    EP0Data.pData = EP0Buf.b;                              /* data to be received */
                    goto setup_class_ok;
                }
              }
#endif  /* USB_AUDIO */
#if USB_CDC
/*
              if ( (SetupPacket.wIndex.WB.L == USB_CDC_CIF_NUM)  ||      // IF number correct?
                   (SetupPacket.wIndex.WB.L == USB_CDC_DIF_NUM)) {
*/
              
              ifaceno = SetupPacket.wIndex.WB.L >> 1;
              if (ifaceno < ciAssoc.number) {
                switch (SetupPacket.bRequest) {
                  case CDC_SEND_ENCAPSULATED_COMMAND:
                    EP0Data.pData = EP0Buf.b;                  /* data to be received, see USB_EVT_OUT */
                    goto setup_class_ok;
                  case CDC_GET_ENCAPSULATED_RESPONSE:
                    //if (CDC_GetEncapsulatedResponse()) {
                      EP0Data.pData = EP0Buf.b;                /* point to data to be sent */
                      USB_DataInStage();                       /* send requested data */
                      goto setup_class_ok;
                    //}
                    break;
                  case CDC_SET_COMM_FEATURE:
                    EP0Data.pData = EP0Buf.b;                  /* data to be received, see USB_EVT_OUT */
                    goto setup_class_ok;
                  case CDC_GET_COMM_FEATURE:
                    //if (CDC_GetCommFeature (SetupPacket.wValue.W)) {
                      EP0Data.pData = EP0Buf.b;                /* point to data to be sent */
                      USB_DataInStage();                       /* send requested data */
                      goto setup_class_ok;
                    //}
                    break;
                  case CDC_CLEAR_COMM_FEATURE:
                    //if (CDC_ClearCommFeature (SetupPacket.wValue.W)) {
                      USB_StatusInStage();                     /* send Acknowledge */
                      goto setup_class_ok;
                    //}
                    break;
                  case CDC_SET_LINE_CODING:
                    EP0Data.pData = EP0Buf.b;                  /* data to be received, see USB_EVT_OUT */
                    goto setup_class_ok;
                  case CDC_GET_LINE_CODING:
                    /*
                    if (CDC_GetLineCoding()) {
                      EP0Data.pData = EP0Buf.b;                // point to data to be sent
                      USB_DataInStage();                       // send requested data
                      goto setup_class_ok;
                    }
                    */
                    break;
                  case CDC_SET_CONTROL_LINE_STATE:
                    //if (CDC_SetControlLineState (SetupPacket.wValue.W)) {
                    if (ciAssoc.iface[ifaceno].SetCtrlLineState (SetupPacket.wValue.W)) {
                      USB_StatusInStage();                     /* send Acknowledge */
                      goto setup_class_ok;
                    }
                    break;
                  case CDC_SEND_BREAK:
                    //if (CDC_SendBreak (SetupPacket.wValue.W)) {
                      USB_StatusInStage();                     /* send Acknowledge */
                      goto setup_class_ok;
                    //}
                    break;
                }
              }
#endif  /* USB_CDC */
#if USB_UVC
              if ( (SetupPacket.wIndex.WB.L == USB_UVC_VCIF_NUM)  ||
                   (SetupPacket.wIndex.WB.L == USB_UVC_VSIF_NUM)) {
                switch (SetupPacket.bRequest) {
                  case SET_CUR:
                    EP0Data.pData = EP0Buf.b;      /* data to be stored, see USB_EVT_OUT */
                    goto setup_class_ok;
                    break;
                  case GET_CUR:
                    if (UVC_GetCur()) {
                      EP0Data.pData = EP0Buf.b;    /* data to be sent */
                      USB_DataInStage();         /* send data */
                      goto setup_class_ok;
                    }
                    break;
                  case GET_MIN:
                    if (UVC_GetMin()) {
                      EP0Data.pData = EP0Buf.b;    /* data to be sent */
                      USB_DataInStage();         /* send data */
                      goto setup_class_ok;
                    }
                    break;
                  case GET_MAX:
                    if (UVC_GetMax()) {
                      EP0Data.pData = EP0Buf.b;    /* data to be sent */
                      USB_DataInStage();         /* send data */
                      goto setup_class_ok;
                    }
                    break;
                  case GET_RES:
                    if (UVC_GetRes()) {
                      EP0Data.pData = EP0Buf.b;    /* data to be sent */
                      USB_DataInStage();         /* send data */
                      goto setup_class_ok;
                    }
                    break;
                  case GET_LEN:
                    if (UVC_GetLen()) {
                      EP0Data.pData = EP0Buf.b;    /* data to be sent */
                      USB_DataInStage();         /* send data */
                      goto setup_class_ok;
                    }
                    break;
                  case GET_INFO:
                    if (UVC_GetInfo()) {
                      EP0Data.pData = EP0Buf.b;    /* data to be sent */
                      USB_DataInStage();         /* send data */
                      goto setup_class_ok;
                    }
                    break;
                  case GET_DEF:
                    if (UVC_GetDef()) {
                      EP0Data.pData = EP0Buf.b;    /* data to be sent */
                      USB_DataInStage();         /* send data */
                      goto setup_class_ok;
                    }
                    break;
                }
              }
#endif  /* USB_UVC */
#if USB_DFU
              if (SetupPacket.wIndex.WB.L == USB_DFU_IF_NUM) {           /* IF number correct? */
                switch (SetupPacket.bRequest) {
                  case USB_REQ_DFU_DETACH:                               /* DFU_DETACH with invalid parameters -> STALL */
                    if (SetupPacket.wLength  == 0) {
                      if (DFU_Detach()) {
                        USB_StatusInStage();
                        goto setup_class_ok;
                      }
                    }
                    break;
                  case USB_REQ_DFU_DNLOAD:
                    if (DFU_SetupForDownload (SetupPacket.wValue.W, SetupPacket.wLength, &eotflag)) {
                      if (eotflag == 1) {
                        USB_StatusInStage();
                      }
                      goto setup_class_ok;
                    }
                    break;
                  case USB_REQ_DFU_UPLOAD:                               /* UPLOAD with invalid parameters -> STALL */
                    if (DFU_Upload (SetupPacket.wValue.W, SetupPacket.wLength)) {
                      USB_DataInStage();
                      /* tell user if upload finished */
                      if (DFUDownloadDone) {
                        /* wait for next download */
                        DFUDownloadDone = 0;
                      }
                      goto setup_class_ok;
                    }
                    break;
                  case USB_REQ_DFU_GETSTATUS:
                    if ( (SetupPacket.wValue.W == 0) &&                  /* GET_STATUS with invalid parameters -> STALL */
                         (SetupPacket.wLength  == 6)) {
                      if (DFU_GetStatus()) {
                        EP0Data.pData = EP0Buf.b;
                        EP0Data.Count = DFU_GET_STATUS_SIZE;
                        USB_DataInStage();
                        /* tell user if download finished */
                        if (DFUDownloadDone) {
                          /* wait for next download */
                          DFUDownloadDone = 0;
                        }
                        goto setup_class_ok;
                      }
                    }
                    break;
                  case USB_REQ_DFU_CLRSTATUS:
                    if ( (SetupPacket.wValue.W == 0) &&                  /* CLEAR_STATUS with invalid parameters -> STALL */
                         (SetupPacket.wLength  == 0)) {
                      if (DFU_ClearStatus()) {
                        USB_StatusInStage();
                        goto setup_class_ok;
                      }
                    }
                    break;
                  case USB_REQ_DFU_GETSTATE:
                    if ( (SetupPacket.wValue.W == 0) &&                  /* GET_STATE with invalid parameters -> STALL */
                         (SetupPacket.wLength  == 1)) {
                      if (DFU_GetState()) {
                        EP0Data.pData = EP0Buf.b;
                        EP0Data.Count = 1;
                        USB_DataInStage();
                        /* tell user if download finished */
                        if (DFUDownloadDone) {
                          /* wait for next download */
                          DFUDownloadDone = 0;
                        }
                        goto setup_class_ok;
                      }
                    }
                    break;
                  case USB_REQ_DFU_ABORT:
                    if ( (SetupPacket.wValue.W == 0) &&                  /* DFU_ABORT with invalid parameters -> STALL */
                         (SetupPacket.wLength  == 0)) {
                      if (DFU_Abort()) {
                        USB_StatusInStage();
                        goto setup_class_ok;
                      }
                    }
                    break;
                }
              }
#endif  /* USB_DFU */
              goto stall_i;                                              /* not supported */
              /* end case REQUEST_TO_INTERFACE */

            case REQUEST_TO_ENDPOINT:
#if USB_AUDIO
              switch (SetupPacket.bRequest) {
                case AUDIO_REQUEST_GET_CUR:
                case AUDIO_REQUEST_GET_MIN:
                case AUDIO_REQUEST_GET_MAX:
                case AUDIO_REQUEST_GET_RES:
                  if (ADC_EP_GetRequest()) {
                    EP0Data.pData = EP0Buf.b;                              /* point to data to be sent */
                    USB_DataInStage();                                   /* send requested data */
                    goto setup_class_ok;
                  }
                  break;
                case AUDIO_REQUEST_SET_CUR:
//              case AUDIO_REQUEST_SET_MIN:
//              case AUDIO_REQUEST_SET_MAX:
//              case AUDIO_REQUEST_SET_RES:
                  EP0Data.pData = EP0Buf.b;                                /* data to be received */
                  goto setup_class_ok;
              }
#endif  /* USB_AUDIO */
              goto stall_i;
              /* end case REQUEST_TO_ENDPOINT */

            default:
              goto stall_i;
          }
        setup_class_ok:                                                          /* request finished successfully */
          break;  /* end case REQUEST_CLASS */
#endif  /* USB_CLASS */

#if USB_VENDOR
        case REQUEST_VENDOR:
          switch (SetupPacket.bmRequestType.BM.Recipient) {

            case REQUEST_TO_DEVICE:
              if (!USB_ReqVendorDev (TRUE)) {
                goto stall_i;                                            /* not supported */
              }
              break;

            case REQUEST_TO_INTERFACE:
              if (!USB_ReqVendorIF (TRUE)) {
                goto stall_i;                                            /* not supported */
              }
              break;

            case REQUEST_TO_ENDPOINT:
              if (!USB_ReqVendorEP (TRUE)) {
                goto stall_i;                                            /* not supported */
              }
              break;

            default:
              goto stall_i;
          }

          if (SetupPacket.wLength) {
            if (SetupPacket.bmRequestType.BM.Dir == REQUEST_DEVICE_TO_HOST) {
              USB_DataInStage();
            }
          } else {
            USB_StatusInStage();
          }

          break;  /* end case REQUEST_VENDOR */
#endif  /* USB_VENDOR */

        default:
        stall_i:
          USB_SetStallEP (0x80);
          EP0Data.Count = 0;
          break;
      }
      break;  /* end case USB_EVT_SETUP */

    case USB_EVT_OUT:
      if (SetupPacket.bmRequestType.BM.Dir == REQUEST_HOST_TO_DEVICE) {
        if (EP0Data.Count) {                                             /* still data to receive ? */
          USB_DataOutStage();                                            /* receive data */
          if (EP0Data.Count == 0) {                                      /* data complete ? */
            switch (SetupPacket.bmRequestType.BM.Type) {

              case REQUEST_STANDARD:
                goto stall_i;                                            /* not supported */

#if (USB_CLASS)
              case REQUEST_CLASS:
                switch (SetupPacket.bmRequestType.BM.Recipient) {
                  case REQUEST_TO_DEVICE:
                    goto stall_i;                                        /* not supported */

                  case REQUEST_TO_INTERFACE:
#if USB_HID
                    if (SetupPacket.wIndex.WB.L == USB_HID_IF_NUM) {     /* IF number correct? */
                      switch (SetupPacket.bRequest) {
                        case HID_REQUEST_SET_REPORT:
                          if (HID_SetReport()) {
                            USB_StatusInStage();                         /* send Acknowledge */
                            goto out_class_ok;
                          }
                          break;
                      }
                    }
#endif  /* USB_HID */
#if USB_DFU
                    if (SetupPacket.wIndex.WB.L == USB_DFU_IF_NUM) {     /* IF number correct? */
                      switch (SetupPacket.bRequest) {
                        case USB_REQ_DFU_DNLOAD:
                          if (DFU_Download (SetupPacket.wValue.W, SetupPacket.wLength)) {
                            USB_StatusInStage();
                            goto out_class_ok;

                          }
                          break;
                      }
                    }
#endif  /* USB_HID */
#if USB_AUDIO
                    if ( (SetupPacket.wIndex.WB.L == USB_ADC_CIF_NUM)  || /* IF number correct? */
                         (SetupPacket.wIndex.WB.L == USB_ADC_SIF1_NUM) ||
                         (SetupPacket.wIndex.WB.L == USB_ADC_SIF2_NUM)) {
                      switch (SetupPacket.bRequest) {
                        case AUDIO_REQUEST_SET_CUR:
//                      case AUDIO_REQUEST_SET_MIN:
//                      case AUDIO_REQUEST_SET_MAX:
//                      case AUDIO_REQUEST_SET_RES:
                          if (ADC_IF_SetRequest()) {
                            USB_StatusInStage();                         /* send Acknowledge */
                            goto out_class_ok;
                          }
                          break;
                      }
                    }
#endif  /* USB_AUDIO */
#if USB_CDC
                    /*
                    if ( (SetupPacket.wIndex.WB.L == USB_CDC_CIF_NUM)  || // IF number correct?
                         (SetupPacket.wIndex.WB.L == USB_CDC_DIF_NUM)) {
                    */
                    ifaceno = SetupPacket.wIndex.WB.L >> 1;
                    if (ifaceno < ciAssoc.number) {
                      switch (SetupPacket.bRequest) {
                        case CDC_SEND_ENCAPSULATED_COMMAND:
                          //if (CDC_SendEncapsulatedCommand()) {
                            USB_StatusInStage();                         /* send Acknowledge */
                            goto out_class_ok;
                          //}
                          break;
                        case CDC_SET_COMM_FEATURE:
                          //if (CDC_SetCommFeature (SetupPacket.wValue.W)) {
                            USB_StatusInStage();                         /* send Acknowledge */
                            goto out_class_ok;
                          //}
                          break;
                        case CDC_SET_LINE_CODING:
                          FillLineCode (&tlc);
                          //if (CDC_SetLineCoding (&tlc)) {
                          if (ciAssoc.iface[ifaceno].SetLineCode (&tlc)) {
                            USB_StatusInStage();                         /* send Acknowledge */
                            goto out_class_ok;
                          }
                          break;
                      }
                    }
#endif  /* USB_CDC */
#if USB_UVC
                    if ( (SetupPacket.wIndex.WB.L == USB_UVC_VCIF_NUM)  ||
                         (SetupPacket.wIndex.WB.L == USB_UVC_VSIF_NUM)) {
                      switch (SetupPacket.bRequest) {
                        case SET_CUR:
                          if (!UVC_SetCur()) {
                            goto stall_i;
                          }
                          break;
                      } /* end switch (SetupPacket.bRequest) */
                      break;
                    }
#endif /* USB_UVC */
                    goto stall_i;
                    /* end case REQUEST_TO_INTERFACE */

                  case REQUEST_TO_ENDPOINT:
#if USB_AUDIO
                    switch (SetupPacket.bRequest) {
                      case AUDIO_REQUEST_SET_CUR:
//                    case AUDIO_REQUEST_SET_MIN:
//                    case AUDIO_REQUEST_SET_MAX:
//                    case AUDIO_REQUEST_SET_RES:
                        if (ADC_EP_SetRequest()) {
                          USB_StatusInStage();                           /* send Acknowledge */
                          goto out_class_ok;
                        }
                        break;
                    }
#endif  /* USB_AUDIO */
                    goto stall_i;
                    /* end case REQUEST_TO_ENDPOINT */

                  default:
                    goto stall_i;
                }
              out_class_ok:                                                            /* request finished successfully */
                break; /* end case REQUEST_CLASS */
#endif  /* USB_CLASS */

#if USB_VENDOR
              case REQUEST_VENDOR:
                switch (SetupPacket.bmRequestType.BM.Recipient) {

                  case REQUEST_TO_DEVICE:
                    if (!USB_ReqVendorDev (FALSE)) {
                      goto stall_i;                                      /* not supported */
                    }
                    break;

                  case REQUEST_TO_INTERFACE:
                    if (!USB_ReqVendorIF (FALSE)) {
                      goto stall_i;                                      /* not supported */
                    }
                    break;

                  case REQUEST_TO_ENDPOINT:
                    if (!USB_ReqVendorEP (FALSE)) {
                      goto stall_i;                                      /* not supported */
                    }
                    break;

                  default:
                    goto stall_i;
                }

                USB_StatusInStage();

                break;  /* end case REQUEST_VENDOR */
#endif  /* USB_VENDOR */

              default:
                goto stall_i;
            }
          }
        }
      } else {
        USB_StatusOutStage();                                            /* receive Acknowledge */
      }
      break;  /* end case USB_EVT_OUT */

    case USB_EVT_IN :
      if (SetupPacket.bmRequestType.BM.Dir == REQUEST_DEVICE_TO_HOST) {
        USB_DataInStage();                                               /* send data */
      } else {
        if (USB_DeviceAddress & 0x80) {
          USB_DeviceAddress &= 0x7F;
          USB_SetAddress (USB_DeviceAddress);
        }
      }
      break;  /* end case USB_EVT_IN */

    case USB_EVT_OUT_STALL:
      USB_ClrStallEP (0x00);
      break;

    case USB_EVT_IN_STALL:
      USB_ClrStallEP (0x80);
      break;

  }
}
