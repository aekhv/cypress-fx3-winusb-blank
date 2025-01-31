/****************************************************************************
**
** This file is part of the CYPRESS-FX3-WINUSB-BLANK project.
** Copyright (C) 2025 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#include <cyu3os.h>
#include <cyu3system.h>
#include <cyu3error.h>
#include <cyu3uart.h>
#include <cyu3gpio.h>
#include <cyu3usb.h>
#include "cyfxdebug.h"

#define CY_FX_APP_THREAD_STACK      (0x1000)
#define CY_FX_APP_THREAD_PRIORITY   (8)
#define CY_FX_GPIO_LED              (54)

extern CyU3PReturnStatus_t CyFxUsbInit(void);

CyU3PThread appThread;

void CyFxFatalErrorHandler(const char* msg, CyU3PReturnStatus_t status, CyBool_t noReturn)
{
    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "FATAL ERROR: %s (%d)\r\n", msg, status);
    if (noReturn)
        while (CyTrue)
            CyU3PThreadSleep(100);
}

void CyFxGetSysInfo(void)
{
    uint16_t majorVersion = 0;
    uint16_t minorVersion = 0;
    uint16_t patchNumer = 0;
    uint16_t buildNumer = 0;
    CyU3PSysGetApiVersion(&majorVersion, &minorVersion, &patchNumer, &buildNumer);
    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "FX3 API version: %d.%d.%d.%d\r\n",
            majorVersion, minorVersion, patchNumer, buildNumer);

    uint32_t clkFreq = 0;
    CyU3PDeviceGetSysClkFreq(&clkFreq);
    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "Current SYS_CLK frequency: %d Hz\r\n", clkFreq);
}

CyU3PReturnStatus_t CyFxDebugInit(void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUartConfig_t uartConfig;

    /* Initialize and configure the UART for logging. */
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
        return apiRetStatus;

    CyU3PMemSet((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma    = CyTrue;

    apiRetStatus = CyU3PUartSetConfig(&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
        return apiRetStatus;

    /* Set the DMA for an infinity transfer */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
        return apiRetStatus;

    /* Start the debug module for printing log messages */
    apiRetStatus = CyU3PDebugInit(CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
        return apiRetStatus;

    CyU3PDebugPreamble(CyFalse);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t CyFxGpioInit(void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PGpioClock_t gpioClock;
    CyU3PGpioSimpleConfig_t gpioConfig;

    CyU3PMemSet((uint8_t *)&gpioClock, 0, sizeof(gpioClock));
    gpioClock.fastClkDiv = 2;
    gpioClock.slowClkDiv = 64;
    gpioClock.simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpioClock.clkSrc     = CY_U3P_SYS_CLK;
    gpioClock.halfDiv    = 0;
    apiRetStatus = CyU3PGpioInit (&gpioClock, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
        return apiRetStatus;

    apiRetStatus = CyU3PDeviceGpioOverride(CY_FX_GPIO_LED, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
        return apiRetStatus;

    CyU3PMemSet((uint8_t *)&gpioConfig, 0, sizeof(gpioConfig));
    gpioConfig.outValue = CyTrue;
    gpioConfig.driveLowEn = CyTrue;
    gpioConfig.driveHighEn = CyTrue;
    gpioConfig.inputEn = CyFalse;
    gpioConfig.intrMode = CY_U3P_GPIO_NO_INTR;
    apiRetStatus = CyU3PGpioSetSimpleConfig(CY_FX_GPIO_LED, &gpioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
        return apiRetStatus;

	return CY_U3P_SUCCESS;
}

/* Thread entry point */
void CyFxAppThreadEntry(uint32_t input)
{
	uint8_t i = 0;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the debug module */
    apiRetStatus = CyFxDebugInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
        CyFxFatalErrorHandler("CyFxDebugInit", apiRetStatus, CyTrue);

    apiRetStatus = CyFxGpioInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
        CyFxFatalErrorHandler("CyFxGpioInit", apiRetStatus, CyTrue);

    CyU3PDebugPrint(CY_FX_DEBUG_PRIORITY, "\r\n");

    CyFxGetSysInfo();

    CyFxUsbInit();

    /* Main loop */
    while (CyTrue)
    {
    	CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();
    	switch (usbSpeed)
    	{
    	case CY_U3P_SUPER_SPEED:
    		for (i = 0; i < 3; i++) {
    	    	CyU3PGpioSetValue(CY_FX_GPIO_LED, CyFalse);
    	    	CyU3PThreadSleep(150);
    	    	CyU3PGpioSetValue(CY_FX_GPIO_LED, CyTrue);
    	    	CyU3PThreadSleep(150);
    		}
        	CyU3PThreadSleep(1100);
        	break;
    	case CY_U3P_HIGH_SPEED:
    		for (i = 0; i < 2; i++) {
    	    	CyU3PGpioSetValue(CY_FX_GPIO_LED, CyFalse);
    	    	CyU3PThreadSleep(150);
    	    	CyU3PGpioSetValue(CY_FX_GPIO_LED, CyTrue);
    	    	CyU3PThreadSleep(150);
    		}
        	CyU3PThreadSleep(1400);
        	break;
    	default:
        	CyU3PGpioSetValue(CY_FX_GPIO_LED, CyTrue);
    		break;
    	}
    }
}

/* Application define function which creates the threads */
void CyFxApplicationDefine(void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc(CY_FX_APP_THREAD_STACK);

    if (ptr != NULL) {
        /* Create the thread for the application */
        retThrdCreate = CyU3PThreadCreate (&appThread,  /* Application thread structure */
                "21:Application thread",                /* Thread ID and Thread name */
                CyFxAppThreadEntry,                     /* Application thread entry function */
                0,                                      /* No input parameter to thread */
                ptr,                                    /* Pointer to the allocated thread stack */
                CY_FX_APP_THREAD_STACK,                 /* Thread stack size */
                CY_FX_APP_THREAD_PRIORITY,              /* Thread priority */
                CY_FX_APP_THREAD_PRIORITY,              /* Pre-emption threshold for the thread. */
                CYU3P_NO_TIME_SLICE,                    /* No time slice for the application thread */
                CYU3P_AUTO_START                        /* Start the thread immediately */
        );
    }

    /* Check the return code */
    if (retThrdCreate != CY_U3P_SUCCESS)
    {
        /* Application cannot continue */
        /* Loop indefinitely */
        while (CyTrue);
    }
}

/* Main function */
int main (void)
{
    CyU3PIoMatrixConfig_t ioConfig;
    CyU3PSysClockConfig_t clkConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* SYS_CLK may be 384 MHz or 403.2 MHz depending upon whether the 'setSysClk400' field */
    clkConfig.setSysClk400  = CyTrue;
    clkConfig.cpuClkDiv     = 2;        /* CPU clock divider */
    clkConfig.dmaClkDiv     = 2;        /* DMA clock divider */
    clkConfig.mmioClkDiv    = 2;        /* MMIO clock divider */
    clkConfig.useStandbyClk = CyFalse;  /* device has no 32KHz clock supplied */
    clkConfig.clkSrc = CY_U3P_SYS_CLK;  /* Clock source for a peripheral block  */

    /* Initialize the device */
    apiRetStatus = CyU3PDeviceInit(&clkConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
        goto fatalErrorHandler;

    /* Initialize the caches. Enable both Instruction and Data Caches. */
    apiRetStatus = CyU3PDeviceCacheControl(CyTrue, CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
        goto fatalErrorHandler;

    /* Configure the IO matrix for the device. On the FX3 DVK board, the COM port
     * is connected to the IO(53:56). This means that either DQ32 mode should be
     * selected or lppMode should be set to UART_ONLY. */
    CyU3PMemSet((uint8_t *)&ioConfig, 0, sizeof(ioConfig));
    ioConfig.isDQ32Bit  = CyTrue;
    ioConfig.useUart    = CyTrue;
    ioConfig.lppMode    = CY_U3P_IO_MATRIX_LPP_DEFAULT;

    apiRetStatus = CyU3PDeviceConfigureIOMatrix(&ioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
        goto fatalErrorHandler;

    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry();

    /* Dummy return to make the compiler happy */
    return 0;

fatalErrorHandler:
    /* Cannot recover from this error. */
    while (CyTrue);
}
