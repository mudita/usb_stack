/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "usb_phydcd.h"
#include "usb_phydcd_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define USB_DCD_DATA_PIN_MAX_DETECTION_COUNT (100U)
/*time settting in ms*/
#define USB_DCD_DATA_PIN_DETECTION_TIME              (10U)
#define USB_DCD_BASE_DETECTION_TIME                  (100U)
#define USB_DCD_CONTACT_DETECTION_TIME               (1000U)
#define USB_DCD_DELAY_TIME                           (1500U)
#define USB_DCD_SECONDARY_DETECTION_PULL_DOWN_CONFIG (0x3CU)
typedef enum _usb_phydcd_dev_status
{
    kUSB_DCDDetectInit = 0x0U,
    kUSB_DCDDetectIdle,
    kUSB_DCDDetectStart,
    kUSB_DCDDataContactDetection,
    kUSB_DCDPrimaryDetection,
    kUSB_DCDPostPrimaryDetection,
    kUSB_DCDSecondaryDetection,
    kUSB_DCDDectionFinished,
    kUSB_DCDAppleCheck,
    kUSB_DCDDetectWait,
} usb_phydcd_dev_status_t;

typedef enum
{
    CLEAR,
    SET,
    SENTINEL
} usb_phydcd_bit_state_t;

typedef struct _usb_phydcd_state_struct
{
    volatile uint64_t hwTick;           /*!< Current hw tick(ms)*/
    volatile uint64_t startTime;        /*!< start time for delay*/
    USB_ANALOG_Type *usbAnalogBase;     /*!< The base address of the dcd module */
    usb_phydcd_callback_t dcdCallback;  /*!< DCD callback function*/
    void *dcdCallbackParam;             /*!< DCD callback parameter*/
    void *phyBase;                      /*!< dcd phy base address, if no phy control needed, set to NULL*/
    uint8_t dcdDisable;                 /*!< whether enable dcd function or not*/
    uint8_t detectResult;               /*!< dcd detect result*/
    uint8_t index;                      /*!< analog instance index*/
    volatile uint8_t dataPinCheckTimes; /*!< the check times to make sure data pin is contacted*/
    volatile uint8_t dcdDetectState;    /*!< DCD callback parameter*/
} usb_phydcd_state_struct_t;
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Apply for device dcd state structure */
static usb_phydcd_state_struct_t s_UsbDeviceDcdHSState[FSL_FEATURE_SOC_USBPHY_COUNT];
/*******************************************************************************
 * Code
 ******************************************************************************/

usb_phydcd_status_t USB_PHYDCD_Init(uint8_t index, usb_phydcd_config_struct_t *config, usb_phydcd_handle *dcdHandle)
{
    usb_phydcd_state_struct_t *dcdState;
    usb_phydcd_status_t dcdError = kStatus_phydcd_Success;
    uint32_t analog_base[]       = USB_ANALOG_BASE_ADDRS;
    USB_ANALOG_Type *base;
    uint32_t *temp;
    base               = (USB_ANALOG_Type *)analog_base[0];
    uint32_t phyBase[] = USBPHY_BASE_ADDRS;
    if ((NULL == config) || (NULL == base) || (NULL == config->dcdCallback)) {
        return kStatus_phydcd_Error;
    }

    dcdState                   = &s_UsbDeviceDcdHSState[index];
    dcdState->index            = index;
    temp                       = (uint32_t *)phyBase[index + 1U];
    dcdState->phyBase          = (void *)temp;
    dcdState->usbAnalogBase    = base;
    dcdState->dcdCallbackParam = config->dcdCallbackParam;
    dcdState->dcdCallback      = config->dcdCallback;
    dcdState->dcdDisable       = 0U;
    dcdState->dcdDetectState   = (uint8_t)kUSB_DCDDetectInit;
    *dcdHandle                 = dcdState;
    return dcdError;
}
usb_phydcd_status_t USB_PHYDCD_Deinit(usb_phydcd_handle dcdHandle)
{
    usb_phydcd_state_struct_t *dcdState;
    dcdState                     = (usb_phydcd_state_struct_t *)dcdHandle;
    usb_phydcd_status_t dcdError = kStatus_phydcd_Success;

    dcdState->index            = 0U;
    dcdState->phyBase          = NULL;
    dcdState->usbAnalogBase    = NULL;
    dcdState->dcdCallbackParam = NULL;
    dcdState->dcdCallback      = NULL;
    dcdState->dcdDisable       = 0U;
    dcdState->dcdDetectState   = (uint8_t)kUSB_DCDDetectInit;

    return dcdError;
}
usb_phydcd_status_t USB_PHYDCD_Control(usb_phydcd_handle handle, usb_phydcd_control_t type, void *param)
{
    usb_phydcd_state_struct_t *dcdState;
    dcdState                     = (usb_phydcd_state_struct_t *)handle;
    usb_phydcd_status_t dcdError = kStatus_phydcd_Success;
    if (NULL == handle) {
        return kStatus_phydcd_Error;
    }
    switch (type) {
    case kUSB_DevicePHYDcdRun:
    case kUSB_DevicePHYDcdStop:
        if (0U == dcdState->dcdDisable) {
            dcdState->dcdDetectState = (uint8_t)kUSB_DCDDetectInit;
        }
        break;
    case kUSB_DevicePHYDcdEnable:
        dcdState->dcdDisable = 0U;
        break;
    case kUSB_DevicePHYDcdDisable:
        dcdState->dcdDisable = 1U;
        break;
    default:
        /*no action*/
        break;
    }
    return dcdError;
}

static void USB_PHYDCD_AnalogChargerDetectRegSet(usb_phydcd_state_struct_t *dcd,
                                                 usb_phydcd_bit_state_t state,
                                                 uint32_t mask)
{
    if (state == CLEAR) {
        dcd->usbAnalogBase->INSTANCE[dcd->index].CHRG_DETECT_CLR = mask;
    }
    else {
        dcd->usbAnalogBase->INSTANCE[dcd->index].CHRG_DETECT_SET = mask;
    }
}

static void USB_PHYDCD_LoopbackRegSet(usb_phydcd_state_struct_t *dcd, usb_phydcd_bit_state_t state, uint32_t mask)
{
    if (state == CLEAR) {
        dcd->usbAnalogBase->INSTANCE[dcd->index].LOOPBACK_CLR = mask;
    }
    else {
        dcd->usbAnalogBase->INSTANCE[dcd->index].LOOPBACK_SET = mask;
    }
}

static void USB_PHYDCD_DebugRegSet(usb_phydcd_state_struct_t *dcd, usb_phydcd_bit_state_t state, uint32_t mask)
{
    if (state == CLEAR) {
        ((USBPHY_Type *)dcd->phyBase)->DEBUG_CLR = mask;
    }
    else {
        ((USBPHY_Type *)dcd->phyBase)->DEBUG_SET = mask;
    }
}

static uint32_t USB_PHYDCD_AnalogChargerDetectStatGet(usb_phydcd_state_struct_t *dcd, uint32_t mask)
{
    return (dcd->usbAnalogBase->INSTANCE[dcd->index].CHRG_DETECT_STAT & mask);
}

static usb_phydcd_dev_status_t USB_PHYDCD_AppleCheck(usb_phydcd_state_struct_t *dcd)
{
    if ((dcd->hwTick - dcd->startTime) >= USB_DCD_BASE_DETECTION_TIME) {
        if (USB_PHYDCD_AnalogChargerDetectStatGet(dcd, USB_ANALOG_CHRG_DETECT_STAT_DP_STATE_MASK) != 0U) {
            dcd->detectResult = (uint8_t)kUSB_DcdDCP;
        }
        /* Set new state start time */
        dcd->startTime = dcd->hwTick;
        /* Change state machine to detection finished */
        return kUSB_DCDDectionFinished;
    }
    return kUSB_DCDAppleCheck;
}

static usb_phydcd_dev_status_t USB_PHYDCD_DetectionInit(usb_phydcd_state_struct_t *dcd)
{
    if ((dcd->hwTick - dcd->startTime) >= USB_DCD_BASE_DETECTION_TIME) {
        /* Control the charger detector (EN_B)
         * 		0  ENABLE — Enable the charger detector.
         * 		1  DISABLE — Disable the charger detector.
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, SET, USB_ANALOG_CHRG_DETECT_SET_EN_B_MASK);

        /* Check the charger connection (CHK_CHRG_B)
         * 		0 CHECK — Check whether a charger (either a dedicated charger or a host charger) is connected to USB
                            port.
         * 		1 NO_CHECK — Do not check whether a charger is connected to the USB port.
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, SET, USB_ANALOG_CHRG_DETECT_SET_CHK_CHRG_B_MASK);

        /* Check the contact of USB plug(CHK_CONTACT)
         * 		0 NO_CHECK — Do not check the contact of USB plug.
         * 		1 CHECK — Check whether the USB plug has been in contact with each other
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, CLEAR, USB_ANALOG_CHRG_DETECT_SET_CHK_CONTACT_MASK);

        /* Setting this bit can enable 1.5 kOhm pull-up resister on DP.(UTMI_TESTSTART)
         * 		Setting this bit can enable 1.5 kOhm pull-up resister on DP.
         *
         * 		This bit can only be used as DCD detection, while it must be cleared in normal function.
         */
        USB_PHYDCD_LoopbackRegSet(dcd, CLEAR, USB_ANALOG_LOOPBACK_UTMI_TESTSTART_MASK);

        /* HSTPULLDOWN
         * Set bit 3 to 1 to pull down 15-KOhm on USB_DP line.
         * Set bit 2 to 1 to pull down 15-KOhm on USB_DM line.
         * Clear to 0 to disable.
         */
        USB_PHYDCD_DebugRegSet(dcd, CLEAR, USB_DCD_SECONDARY_DETECTION_PULL_DOWN_CONFIG);

        /* Gate Test Clocks. (CLKGATE)
         * 		Clear to 0 for running clocks.
         * 		Set to 1 to gate clocks. Set this to save power while the USB is not actively being used
         */
        USB_PHYDCD_DebugRegSet(dcd, SET, USBPHY_DEBUG_CLR_CLKGATE_MASK);

        /* Set new state start time */
        dcd->startTime = dcd->hwTick;

        /* Change state machine */
        return kUSB_DCDDetectStart;
    }
    return kUSB_DCDDetectInit;
}

static usb_phydcd_dev_status_t USB_PHYDCD_DetectionStart(usb_phydcd_state_struct_t *dcd)
{
    /* VBUS detected, lets start charging detection */
    if ((dcd->hwTick - dcd->startTime) >= USB_DCD_BASE_DETECTION_TIME) {
        /* Unknown detect result due to start detection */
        dcd->detectResult = (uint8_t)kUSB_DcdUnknownType;

        /* Clear PinCheckTimes */
        dcd->dataPinCheckTimes = 0U;

        /* Set new state start time */
        dcd->startTime = dcd->hwTick;

        /*
         * i.MX RT1050 Processor Reference Manual, Rev. 5, 09/2021
         * 		43.5.2 USB Charger Detect Register (USB_ANALOG_USB1_CHRG_DETECTn) p.2559
         */

        /* Control the charger detector (EN_B)
         * 		0  ENABLE — Enable the charger detector.
         * 		1  DISABLE — Disable the charger detector.
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, CLEAR, USB_ANALOG_CHRG_DETECT_CLR_EN_B_MASK);

        /*
         * Check the charger connection (CHK_CHRG_B)
         * 		0 CHECK — Check whether a charger (either a dedicated charger or a host charger) is connected to USB
         * port. 1 NO_CHECK — Do not check whether a charger is connected to the USB port.
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, SET, USB_ANALOG_CHRG_DETECT_SET_CHK_CHRG_B_MASK);

        /* Check the contact of USB plug (CHK_CONTACT)
         * 		0 NO_CHECK — Do not check the contact of USB plug.
         * 		1 CHECK — Check whether the USB plug has been in contact with each other
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, SET, USB_ANALOG_CHRG_DETECT_SET_CHK_CONTACT_MASK);

        /* Set new state start time */
        dcd->startTime = dcd->hwTick;

        /* Change state machine */
        return kUSB_DCDDataContactDetection;
    }
    return kUSB_DCDDetectStart;
}

static usb_phydcd_dev_status_t USB_PHYDCD_ContactDetection(usb_phydcd_state_struct_t *dcd)
{
    usb_phydcd_dev_status_t ret = dcd->dcdDetectState;

    /* If detection takes more than 1000 [ms] */
    if ((dcd->hwTick - dcd->startTime) >= USB_DCD_CONTACT_DETECTION_TIME) {
        /* Set dcd detection result as error */
        dcd->detectResult = (uint8_t)kUSB_DcdError;

        /* detection error may mean connecting apple charger */
        ret = kUSB_DCDAppleCheck;

        /* Set new state start time */
        dcd->startTime = dcd->hwTick;

        /* Control the charger detector (EN_B)
         * 		0  ENABLE — Enable the charger detector.
         * 		1  DISABLE — Disable the charger detector.
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, SET, USB_ANALOG_CHRG_DETECT_SET_EN_B_MASK);

        /* Check the charger connection (CHK_CHRG_B)
         * 		0 CHECK — Check whether a charger (either a dedicated charger or a host charger) is connected to USB
         * port. 1 NO_CHECK — Do not check whether a charger is connected to the USB port.
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, SET, USB_ANALOG_CHRG_DETECT_SET_CHK_CHRG_B_MASK);

        /* Check the contact of USB plug (CHK_CONTACT)
         * 		0 NO_CHECK — Do not check the contact of USB plug.
         * 		1 CHECK — Check whether the USB plug has been in contact with each other
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, CLEAR, USB_ANALOG_CHRG_DETECT_SET_CHK_CONTACT_MASK);
    }
    else if ((dcd->hwTick - dcd->startTime) / USB_DCD_DATA_PIN_DETECTION_TIME) {
        /* State of the USB plug contact detector (PLUG_CONTACT)
         * 		0 NO_CONTACT — The USB plug has not made contact.
         * 		1 GOOD_CONTACT — The USB plug has made good contact.
         */
        if (USB_PHYDCD_AnalogChargerDetectStatGet(dcd, USB_ANALOG_CHRG_DETECT_STAT_PLUG_CONTACT_MASK) != 0U) {
            dcd->dataPinCheckTimes++;

            if ((dcd->dataPinCheckTimes) > 5u) {
                /* Change state machine to primary detection */
                ret = (uint8_t)kUSB_DCDPrimaryDetection;

                /* Set new state machine start time */
                dcd->startTime = dcd->hwTick;

                /* Check the contact of USB plug(CHK_CONTACT)
                 * 		0 NO_CHECK — Do not check the contact of USB plug.
                 * 		1 CHECK — Check whether the USB plug has been in contact with each other
                 */
                USB_PHYDCD_AnalogChargerDetectRegSet(dcd, CLEAR, USB_ANALOG_CHRG_DETECT_CLR_CHK_CONTACT_MASK);

                /* Check the charger connection (CHK_CHRG_B)
                 * 		0 CHECK — Check whether a charger (either a dedicated charger or a host charger) is connected to
                 * USB port. 1 NO_CHECK — Do not check whether a charger is connected to the USB port.
                 */
                USB_PHYDCD_AnalogChargerDetectRegSet(dcd, CLEAR, USB_ANALOG_CHRG_DETECT_CLR_CHK_CHRG_B_MASK);
            }
        }
        else {
            dcd->dataPinCheckTimes = 0U;
        }
    }
    else {
        /* Satisfy Misra */
    }

    return ret;
}

static usb_phydcd_dev_status_t USB_PHYDCD_ChargerPrimaryDetection(usb_phydcd_state_struct_t *dcd)
{
    usb_phydcd_dev_status_t ret = dcd->dcdDetectState;

    if ((dcd->hwTick - dcd->startTime) >= USB_DCD_BASE_DETECTION_TIME) {
        /* State of charger detection. This bit is a read only version of the state of the analog signal (CHRG_DETECTED)
         * 		0 CHARGER_NOT_PRESENT — The USB port is not connected to a charger
         * 		1 CHARGER_PRESENT — A charger (either a dedicated charger or a host charger) is connected to the USB
         * port.
         */
        if (USB_PHYDCD_AnalogChargerDetectStatGet(dcd, USB_ANALOG_CHRG_DETECT_STAT_CHRG_DETECTED_MASK) == 0U) {
            /* Primary detection finished as SDP */
            dcd->detectResult = (uint8_t)kUSB_DcdSDP;

            /* Change state machine to detection finished */
            ret = kUSB_DCDDectionFinished;
        }
        else {
            /* Change state machine to secondary detection */
            ret = (uint8_t)kUSB_DCDPostPrimaryDetection;
        }

        /* Control the charger detector (EN_B)
         * 		0  ENABLE — Enable the charger detector.
         * 		1  DISABLE — Disable the charger detector.
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, SET, USB_ANALOG_CHRG_DETECT_SET_EN_B_MASK);

        /* Check the charger connection (CHK_CHRG_B)
         * 		0 CHECK — Check whether a charger (either a dedicated charger or a host charger) is connected to USB
         * port. 1 NO_CHECK — Do not check whether a charger is connected to the USB port.
         */
        USB_PHYDCD_AnalogChargerDetectRegSet(dcd, SET, USB_ANALOG_CHRG_DETECT_SET_CHK_CHRG_B_MASK);

        /* Set new state machine start time */
        dcd->startTime = dcd->hwTick;
    }

    return ret;
}

static usb_phydcd_dev_status_t USB_PHYDCD_PostPrimaryDetection(usb_phydcd_state_struct_t *dcd)
{
    usb_phydcd_dev_status_t ret = dcd->dcdDetectState;

    /* Post Primary detection after the same time as Primary Detection */
    if ((dcd->hwTick - dcd->startTime) >= USB_DCD_BASE_DETECTION_TIME) {
        /* Gate Test Clocks. (CLKGATE)
         * 		Clear to 0 for running clocks.
         * 		Set to 1 to gate clocks. Set this to save power while the USB is not actively being used
         *
         * 		Configuration state is kept while the clock is gated
         */
        USB_PHYDCD_DebugRegSet(dcd, CLEAR, USBPHY_DEBUG_CLR_CLKGATE_MASK);

        /* HSTPULLDOWN
         * Set bit 3 to 1 to pull down 15-KOhm on USB_DP line.
         * Set bit 2 to 1 to pull down 15-KOhm on USB_DM line.
         * Clear to 0 to disable.
         */
        USB_PHYDCD_DebugRegSet(dcd, SET, USB_DCD_SECONDARY_DETECTION_PULL_DOWN_CONFIG);

        /* Setting this bit can enable 1.5 kOhm pull-up resister on DP.(UTMI_TESTSTART)
         * 		Setting this bit can enable 1.5 kOhm pull-up resister on DP.
         *
         * 		This bit can only be used as DCD detection, while it must be cleared in normal function.
         */
        USB_PHYDCD_LoopbackRegSet(dcd, SET, USB_ANALOG_LOOPBACK_UTMI_TESTSTART_MASK);

        /* Set new state start time */
        dcd->startTime = dcd->hwTick;

        /* Change state machine to secondary detection */
        ret = kUSB_DCDSecondaryDetection;
    }
    return ret;
}

static usb_phydcd_dev_status_t USB_PHYDCD_SecondaryDetection(usb_phydcd_state_struct_t *dcd)
{
    usb_phydcd_dev_status_t ret = dcd->dcdDetectState;

    /* Secondary detection after set time*/
    if ((dcd->hwTick - dcd->startTime) >= USB_DCD_BASE_DETECTION_TIME) {
        /* DM line state output of the charger detector (DM_STATE)
         * 		DM_STATE
         */
        dcd->detectResult = (USB_PHYDCD_AnalogChargerDetectStatGet(dcd, USB_ANALOG_CHRG_DETECT_STAT_DM_STATE_MASK))
                                ? (uint8_t)kUSB_DcdDCP
                                : (uint8_t)kUSB_DcdCDP;

        /* Setting this bit can enable 1.5 kOhm pull-up resister on DP.(UTMI_TESTSTART)
         * 		Setting this bit can enable 1.5 kOhm pull-up resister on DP.
         *
         * 		This bit can only be used as DCD detection, while it must be cleared in normal function.
         */
        USB_PHYDCD_LoopbackRegSet(dcd, CLEAR, USB_ANALOG_LOOPBACK_UTMI_TESTSTART_MASK);

        /* (HSTPULLDOWN)
         * Set bit 3 to 1 to pull down 15-KOhm on USB_DP line.
         * Set bit 2 to 1 to pull down 15-KOhm on USB_DM line.
         * Clear to 0 to disable.
         */
        USB_PHYDCD_DebugRegSet(dcd, CLEAR, USB_DCD_SECONDARY_DETECTION_PULL_DOWN_CONFIG);

        /* Gate Test Clocks. (CLKGATE)
         * 		Clear to 0 for running clocks.
         * 		Set to 1 to gate clocks. Set this to save power while the USB is not actively being used
         */
        USB_PHYDCD_DebugRegSet(dcd, SET, USBPHY_DEBUG_CLR_CLKGATE_MASK);

        /* Set new state start time */
        dcd->startTime = dcd->hwTick;

        /* Change state machine to detection finished */
        ret = kUSB_DCDDectionFinished;
    }

    return ret;
}

static usb_phydcd_dev_status_t USB_PHYDCD_DetectionFinished(usb_phydcd_state_struct_t *dcd)
{
    static bool first_run = true;
    usb_phydcd_dev_status_t next_step;

    if (first_run == true) {
        dcd->detectResult = (uint8_t)kUSB_DcdUnknownType;
        next_step         = kUSB_DCDDetectWait;
    }
    else {
        next_step = kUSB_DCDDetectIdle;
    }

    (void)dcd->dcdCallback(dcd->dcdCallbackParam, dcd->detectResult, (void *)&dcd->detectResult);

    first_run = false;

    /* Set new state start time */
    dcd->startTime = dcd->hwTick;
    /* Change state machine to Idle */
    return next_step;
}

static usb_phydcd_dev_status_t USB_PHYDCD_DetectionWait(usb_phydcd_state_struct_t *dcd)
{
    if ((dcd->hwTick - dcd->startTime) >= USB_DCD_DELAY_TIME) {
        /* Set new state start time */
        dcd->startTime = dcd->hwTick;
        /* Change state machine to Idle */
        return kUSB_DCDDetectInit;
    }
    return kUSB_DCDDetectWait;
}

usb_phydcd_status_t USB_PHYDCD_TimerIsrFunction(usb_phydcd_handle handle, const uint64_t tick)
{
    if (NULL == handle) {
        return kStatus_phydcd_Error;
    }
    /* Get state of DCD */
    usb_phydcd_state_struct_t *dcdState = (usb_phydcd_state_struct_t *)handle;

    dcdState->hwTick = tick;

    switch (dcdState->dcdDetectState) {
    case (uint8_t)kUSB_DCDDetectInit:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_DetectionInit(dcdState);
        break;
    case (uint8_t)kUSB_DCDDetectIdle:
        break;
    case (uint8_t)kUSB_DCDDetectStart:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_DetectionStart(dcdState);
        break;
    case (uint8_t)kUSB_DCDDataContactDetection:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_ContactDetection(dcdState);
        break;
    case (uint8_t)kUSB_DCDPrimaryDetection:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_ChargerPrimaryDetection(dcdState);
        break;
    case (uint8_t)kUSB_DCDPostPrimaryDetection:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_PostPrimaryDetection(dcdState);
        break;
    case (uint8_t)kUSB_DCDSecondaryDetection:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_SecondaryDetection(dcdState);
        break;
    case (uint8_t)kUSB_DCDDectionFinished:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_DetectionFinished(dcdState);
        break;
    case (uint8_t)kUSB_DCDAppleCheck:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_AppleCheck(dcdState);
        break;
    case (uint8_t)kUSB_DCDDetectWait:
        dcdState->dcdDetectState = (uint8_t)USB_PHYDCD_DetectionWait(dcdState);
        break;
    default:
        break;
    }
    return kStatus_phydcd_Success;
}
