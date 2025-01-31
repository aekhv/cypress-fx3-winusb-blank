/****************************************************************************
**
** This file is part of the CYPRESS-FX3-WINUSB-BLANK project.
** Copyright (C) 2025 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#include <cyu3system.h>
#include <cyu3error.h>
#include "cyfxdebug.h"

CyU3PReturnStatus_t CyFxUsbAppStart(void)
{
    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Application started...\r\n");
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t CyFxUsbAppStop(void)
{
    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Application stopped...\r\n");
    return CY_U3P_SUCCESS;
}
