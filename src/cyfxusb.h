/****************************************************************************
**
** This file is part of the CYPRESS-FX3-WINUSB-BLANK project.
** Copyright (C) 2025 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#ifndef CYFXUSB_H_
#define CYFXUSB_H_

#define CY_FX_USB_VID                   (0x04B4)  /* Default USB Vendor ID */
#define CY_FX_USB_PID                   (0x0101)  /* Default USB Product ID */
#define CY_FX_MS_VENDOR_CODE            (0xAE)    /* Used defined vendor code used by Microsoft WinUSB driver (AE - it's me) */
#define CY_FX_EP_PRODUCER               (0x01)    /* EP 1 OUT */
#define CY_FX_EP_CONSUMER               (0x81)    /* EP 1 IN */
#define CY_FX_EP_BURST_LENGTH           (16)
#define CY_FX_HIGH_SPEED_EP_SIZE        (512)
#define CY_FX_SUPER_SPEED_EP_SIZE       (1024)
#define CY_FX_BULK_BUFFER_SIZE          (8192)
#define CY_FX_VENDOR_REQUEST            (0xFF)    /* Vendor request type code */

// A mask to define EP0 request direction
#define USB_REQUEST_DEVICE_TO_HOST      (0x80)

/*
 * MS OS String Descriptor.
 * See page 6 of the "OS_Desc_Intro.doc" document.
 */
#define MS_OS_STRING_DESCRIPTOR         (0xEE)

/*
 * MS Extended Compat ID OS Descriptor.
 * See page 4 of the "OS_Desc_CompatID.doc" document.
 */
#define MS_EXTENDED_COMPAT_ID_OS_DESCRIPTOR (4)

/*
 * MS Extended Properties OS Descriptor.
 * See description on page 3 of the "OS_Desc_Ext_Prop.doc" document.
 */
#define MS_EXTENDED_PROPERTIES_OS_DESCRIPTOR (5)

extern const uint8_t CyFxUsb30DeviceDscr[];
extern const uint8_t CyFxUsb20DeviceDscr[];
extern const uint8_t CyFxUsbBOSDscr[];
extern const uint8_t CyFxUsbSSConfigDscr[];
extern const uint8_t CyFxUsbHSConfigDscr[];
extern const uint8_t CyFxUsbLangIdDscr[];
extern const uint8_t CyFxUsbManufactureDscr[];
extern const uint8_t CyFxUsbProductDscr[];
extern const uint8_t CyFxUsbSerialNumberDscr[];
extern const uint8_t CyFxUsbMsOsStringDscr[];
extern const uint8_t CyFxUsbMsCompIdOsDscr[];
extern const uint8_t CyFxUsbMsExtPropOsDscr[];

#include <cyu3externcend.h>

#endif /* CYFXUSB_H_ */
