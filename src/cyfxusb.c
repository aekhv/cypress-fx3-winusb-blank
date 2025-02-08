/****************************************************************************
**
** This file is part of the CYPRESS-FX3-WINUSB-BLANK project.
** Copyright (C) 2025 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#include <cyu3system.h>
#include <cyu3error.h>
#include <cyu3usb.h>
#include "cyfxdebug.h"
#include "cyfxusb.h"

/* Page numbers below reference to "USB 3.2 Revision 1.0.pdf" document */

extern void CyFxFatalErrorHandler(const char* msg, CyU3PReturnStatus_t status, CyBool_t noReturn);
extern CyU3PReturnStatus_t CyFxUsbAppStart(void);
extern CyU3PReturnStatus_t CyFxUsbAppStop(void);

uint8_t glUsbConfiguration = 0; /* Active USB device configuration */
uint8_t glEp0Buffer[64] __attribute__ ((aligned (32))); /* EP0 buffer */

void CyFxUsbDebugPrintRequest(uint8_t bDir, uint8_t bType, uint8_t bTarget, uint8_t bRequest,
        uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "USB request: bDir (%s), bType (",
            (bDir == USB_REQUEST_DEVICE_TO_HOST) ? "to Host  " : "to Device");

    switch (bType)
    {
    case CY_U3P_USB_STANDARD_RQT:
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Standard");
        break;
    case CY_U3P_USB_CLASS_RQT:
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Class   ");
        break;
    case CY_U3P_USB_VENDOR_RQT:
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Vendor  ");
        break;
    default:
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Unknown ");
    }

    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "), bTarget (");
    switch (bTarget)
    {
    case CY_U3P_USB_TARGET_DEVICE:
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Device   ");
        break;
    case CY_U3P_USB_TARGET_INTF:
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Interface");
        break;
    case CY_U3P_USB_TARGET_ENDPT:
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Endpoint ");
        break;
    default:
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Unknown  ");
    }
    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY,
                    "), bRequest (0x%x), wValue (0x%x), wIndex (0x%x), wLength (0x%x)\r\n",
                    bRequest, wValue, wIndex, wLength);
}

CyU3PReturnStatus_t CyFxUsbSendDescriptor(uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
    uint16_t length = 0;
    uint8_t *buffer = NULL;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    switch (wValue >> 8)
    {
    case CY_U3P_USB_DEVICE_DESCR:
        switch (usbSpeed)
        {
        case CY_U3P_SUPER_SPEED:
            buffer = (uint8_t *)CyFxUsb30DeviceDscr;
            break;
        case CY_U3P_HIGH_SPEED:
            buffer = (uint8_t *)CyFxUsb20DeviceDscr;
            break;
        default:
            break;
        }
        if (buffer != NULL)
            length = buffer[0];
        break;

    case CY_U3P_USB_CONFIG_DESCR:
        switch (usbSpeed)
        {
        case CY_U3P_SUPER_SPEED:
            buffer = (uint8_t *)CyFxUsbSSConfigDscr;
            break;
        case CY_U3P_HIGH_SPEED:
            buffer = (uint8_t *)CyFxUsbHSConfigDscr;
            break;
        default:
            break;
        }
        if (buffer != NULL)
            length = (buffer[2] | ((uint16_t)buffer[3] << 8));
        break;

    case CY_U3P_BOS_DESCR:
        buffer = (uint8_t *)CyFxUsbBOSDscr;
        length = (buffer[2] | ((uint16_t)buffer[3] << 8));
        break;

    case CY_U3P_USB_STRING_DESCR:
        switch (wValue & 0xFF)
        {
        case 0:
            buffer = (uint8_t *)CyFxUsbLangIdDscr;
            break;
        case 1:
            buffer = (uint8_t *)CyFxUsbManufactureDscr;
            break;
        case 2:
            buffer = (uint8_t *)CyFxUsbProductDscr;
            break;
        case 3:
            buffer = (uint8_t *)CyFxUsbSerialNumberDscr;
            break;
        case MS_OS_STRING_DESCRIPTOR:
            buffer = (uint8_t *)CyFxUsbMsOsStringDscr;
            break;
        default:
            break;
        }
        if (buffer != NULL)
            length = buffer[0];
        break;

    default:
        if (CY_FX_DEBUG_TRACE_ALL_REQUESTS)
            CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Unknown descriptor requested: wValue (0x%x), wIndex (0x%x), wLength (0x%x)\r\n",
                    wValue, wIndex, wLength);
    }

    if (buffer != NULL)
        return CyU3PUsbSendEP0Data((wLength < length) ? wLength : length, buffer);
    else
        return CY_U3P_ERROR_FAILURE;
}

CyBool_t CyFxUsbSetupCB(uint32_t setupdat0, uint32_t setupdat1)
{
    uint8_t bReqType, bDir, bType, bTarget;
    uint8_t bRequest;
    uint16_t wValue, wIndex, wLength;
    uint16_t br;

    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bDir     = setupdat0 & USB_REQUEST_DEVICE_TO_HOST;  // 0x80 = Device to Host, 0 = Host to Device
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);       // Standard, Class, Vendor or Reserved
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);     // Device, Interface, Endpoint or Other
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);
    wLength  = ((setupdat1 & CY_U3P_USB_LENGTH_MASK)  >> CY_U3P_USB_LENGTH_POS);

    CyBool_t isHandled = CyFalse;

    if (CY_FX_DEBUG_TRACE_ALL_REQUESTS)
        CyFxUsbDebugPrintRequest(bDir, bType, bTarget, bRequest, wValue, wIndex, wLength);

    if ((bType == CY_U3P_USB_STANDARD_RQT)
            && (bTarget == CY_U3P_USB_TARGET_DEVICE)
            && (bDir == USB_REQUEST_DEVICE_TO_HOST))
        switch (bRequest)
        {
        case CY_U3P_USB_SC_GET_DESCRIPTOR:
            if (CyFxUsbSendDescriptor(wValue, wIndex, wLength) == CY_U3P_SUCCESS)
                isHandled = CyTrue;
            break;
        case CY_U3P_USB_SC_GET_CONFIGURATION: /* See p.333 */
            glEp0Buffer[0] = glUsbConfiguration;
            if (CyU3PUsbSendEP0Data(wLength, glEp0Buffer) == CY_U3P_SUCCESS)
                isHandled = CyTrue;
            break;
        default:
            break;
        }

    if ((bType == CY_U3P_USB_STANDARD_RQT)
            && (bRequest == CY_U3P_USB_SC_GET_STATUS)
            && (bDir == USB_REQUEST_DEVICE_TO_HOST)) /* See p.335 */
    {
        CyBool_t isStall = CyFalse;
        CyU3PMemSet (glEp0Buffer, 0, sizeof(glEp0Buffer));
        switch (bTarget)
        {
        case CY_U3P_USB_TARGET_DEVICE:
            if (CY_U3P_SUCCESS == CyU3PUsbSendEP0Data(wLength, glEp0Buffer))
                isHandled = CyTrue;
            break;
        case CY_U3P_USB_TARGET_INTF:
            if (CY_U3P_SUCCESS == CyU3PUsbSendEP0Data(wLength, glEp0Buffer))
                isHandled = CyTrue;
            break;
        case CY_U3P_USB_TARGET_ENDPT:
            if (CyU3PUsbGetEpCfg(wIndex, NULL, &isStall) == CY_U3P_SUCCESS)
            {
                glEp0Buffer[0] = isStall;
                if (CyU3PUsbSendEP0Data(wLength, glEp0Buffer) == CY_U3P_SUCCESS)
                    isHandled = CyTrue;
            }
            break;
        default:
            break;
        }
    }

    if ((bType == CY_U3P_USB_VENDOR_RQT)
            && (bTarget == CY_U3P_USB_TARGET_DEVICE)
            && (bDir == USB_REQUEST_DEVICE_TO_HOST)
            && (bRequest == CY_FX_MS_VENDOR_CODE)
            && (wIndex == MS_EXTENDED_COMPAT_ID_OS_DESCRIPTOR))
    {
        if (CyU3PUsbSendEP0Data(wLength < CyFxUsbMsCompIdOsDscr[0] ? wLength : CyFxUsbMsCompIdOsDscr[0],
                (uint8_t *)CyFxUsbMsCompIdOsDscr) == CY_U3P_SUCCESS)
            isHandled = CyTrue;
    }

    if ((bType == CY_U3P_USB_VENDOR_RQT)
            && (bTarget == CY_U3P_USB_TARGET_INTF)
            && (bDir == USB_REQUEST_DEVICE_TO_HOST)
            && (bRequest == CY_FX_MS_VENDOR_CODE)
            && (wIndex == MS_EXTENDED_PROPERTIES_OS_DESCRIPTOR))
    {
        if (CyU3PUsbSendEP0Data(wLength < CyFxUsbMsExtPropOsDscr[0] ? wLength : CyFxUsbMsExtPropOsDscr[0],
                (uint8_t *)CyFxUsbMsExtPropOsDscr) == CY_U3P_SUCCESS)
            isHandled = CyTrue;
    }

    if ((bType == CY_U3P_USB_STANDARD_RQT)
            && (bTarget == CY_U3P_USB_TARGET_DEVICE)
            && (bDir != USB_REQUEST_DEVICE_TO_HOST))
        switch (bRequest)
        {
        case CY_U3P_USB_SC_SET_SEL: /* See p.342 */
        case CY_U3P_USB_SC_SET_ISOC_DELAY: /* See p.342 */
        case CY_U3P_USB_SC_SET_FEATURE: /* See p.332 */
            CyU3PUsbStall(0, CyTrue, CyFalse);
            isHandled = CyTrue;
            break;
        case CY_U3P_USB_SC_SET_CONFIGURATION: /* See p.339 */
            if (wValue == 1) {
                CyU3PUsbLPMDisable();
                CyFxUsbAppStart();
                glUsbConfiguration = wValue;
                CyU3PUsbAckSetup();
                isHandled = CyTrue;
            }
            break;
        default:
            break;
        }

    // User defined request
    if ((bType == CY_U3P_USB_VENDOR_RQT)
            && (bTarget == CY_U3P_USB_TARGET_INTF)
            && (bRequest == CY_FX_VENDOR_REQUEST)) {
        if (bDir == USB_REQUEST_DEVICE_TO_HOST) {
            // EP0 buffer send to Host
            if (CyU3PUsbSendEP0Data (wLength, glEp0Buffer) == CY_U3P_SUCCESS)
                isHandled = CyTrue;
        } else {
            // EP0 buffer receive from Host
            if (CyU3PUsbGetEP0Data(sizeof(glEp0Buffer), glEp0Buffer, &br) == CY_U3P_SUCCESS)
                isHandled = CyTrue;
        }
    }

	if (!isHandled)
    {
        if (CY_FX_DEBUG_TRACE_ALL_REQUESTS)
            CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Request is not handled!\r\n");
        CyU3PUsbStall(0, CyTrue, CyFalse);
    }

    return isHandled;
}

void CyFxUsbEventCB (CyU3PUsbEventType_t evType, uint16_t evData)
{
    if (CY_FX_DEBUG_TRACE_ALL_REQUESTS)
        CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "USB event  : evType (%d), evData (%d)\r\n", evType, evData);

    switch (evType)
    {
    case CY_U3P_USB_EVENT_RESET:
        CyFxUsbAppStop();
        break;
    default:
        break;
    }
}

CyBool_t CyFxUsbLPMRequestCB(CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

CyU3PReturnStatus_t CyFxUsbInit(void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Start the USB functionality. */
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus != CY_U3P_SUCCESS)
    	CyFxFatalErrorHandler("CyU3PUsbStart", apiRetStatus, CyTrue);

    CyU3PUsbRegisterSetupCallback(CyFxUsbSetupCB, CyFalse);
    CyU3PUsbRegisterEventCallback(CyFxUsbEventCB);
    CyU3PUsbRegisterLPMRequestCallback(CyFxUsbLPMRequestCB);

    apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    	CyFxFatalErrorHandler("CyU3PConnectState", apiRetStatus, CyTrue);

    return CY_U3P_SUCCESS;
}
