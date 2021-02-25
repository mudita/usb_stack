#include "board.h"
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "usb_phy.h"
#include "mtp.h"

void USB_OTG1_IRQHandler(void)
{
    USB_DeviceEhciIsrFunction(g_composite.deviceHandle);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}

void USB_OTG2_IRQHandler(void)
{
    USB_DeviceEhciIsrFunction(g_composite.deviceHandle);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}

static uint32_t USB_DeviceClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };

    if (CONTROLLER_ID == kUSB_ControllerEhci0)
    {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 480000000U);
    }
    else
    {
        CLOCK_EnableUsbhs1PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
        CLOCK_EnableUsbhs1Clock(kCLOCK_Usb480M, 480000000U);
    }
    return USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
}

static void USB_DeviceClockDeinit(void)
{
    USB_EhciPhyDeinit(CONTROLLER_ID);

    if (CONTROLLER_ID == kUSB_ControllerEhci0)
    {
        CLOCK_DisableUsbhs0PhyPllClock();
    }
    else
    {
        CLOCK_DisableUsbhs1PhyPllClock();
    }
}

static void USB_DeviceSetIsr(bool enable)
{
    uint8_t irqNumber;

    uint8_t usbDeviceEhciIrq[] = USBHS_IRQS;
    irqNumber                  = usbDeviceEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];

    if (enable) {
        /* Install isr, set priority, and enable IRQ. */
        NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
        EnableIRQ((IRQn_Type)irqNumber);
    } else {
        DisableIRQ((IRQn_Type)irqNumber);
    }
}

#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle)
{
    USB_DeviceEhciTaskFunction(deviceHandle);
}

void USB_DeviceTask(void *handle)
{
    while (1U)
    {
        USB_DeviceTaskFn(handle);
    }
}
#endif


/* USB device class information */
static usb_device_class_config_struct_t g_CompositeClassConfig[] = {
#if ((defined(USB_DEVICE_CONFIG_CDC_ACM)) && (USB_DEVICE_CONFIG_CDC_ACM > 0U))
    {
        USB_DeviceMtpCallback, (class_handle_t)NULL, &g_UsbDeviceMtpConfig,
    },
#endif
#if ((defined(USB_DEVICE_CONFIG_MTP)) && (USB_DEVICE_CONFIG_MTP > 0U))
    {
        USB_DeviceCdcVcomCallback, (class_handle_t)NULL, &g_UsbDeviceCdcVcomConfig,
    }
#endif
};

/* USB device class configuration information */
static usb_device_class_config_list_struct_t g_UsbDeviceCompositeConfigList = {
    g_CompositeClassConfig,
    USB_DeviceCallback,
    sizeof(g_CompositeClassConfig)/sizeof(usb_device_class_config_struct_t),
};


usb_device_composite_struct_t* USB_DeviceApplicationInit(userCbFunc callback, void* userArg)
{
    if (USB_DeviceClockInit() != kStatus_USB_Success) {
        LOG_ERROR("[Composite] USB Device Clock init failed");
    }

    g_composite.speed = USB_SPEED_FULL;
    g_composite.attach = 0;
    g_composite.deviceHandle = NULL;

    g_cdcVcom.cdcAcmHandle = (class_handle_t)NULL;


    PMU->REG_3P0 |= PMU_REG_3P0_ENABLE_ILIMIT(1);
    PMU->REG_3P0 |= PMU_REG_3P0_ENABLE_LINREG(1);

    LOG_DEBUG("VBUS_DETECT: 0x%08x\r\n", (unsigned int)USB_ANALOG->INSTANCE[0].VBUS_DETECT);
    LOG_DEBUG("VBUS_DETECT_STAT: 0x%08x\r\n", (unsigned int)USB_ANALOG->INSTANCE[0].VBUS_DETECT_STAT);

    if (kStatus_USB_Success !=
        USB_DeviceClassInit(CONTROLLER_ID, &g_UsbDeviceCompositeConfigList, &g_composite.deviceHandle))
    {
        LOG_ERROR("[Composite] USB Device init failed");
        return NULL;
    }
    else
    {
        /* TODO: pass event handling function here */
        if (VirtualComInit(&g_cdcVcom, g_CompositeClassConfig[1].classHandle, callback, userArg) != kStatus_USB_Success)
            LOG_ERROR("[Composite] VirtualCom initialization failed");

        if (USB_DeviceMtpApplicationInit(g_CompositeClassConfig[0].classHandle) != kStatus_USB_Success)
            LOG_ERROR("[Composite] MTP initialization failed");
    }

    USB_DeviceSetIsr(true);

    if (USB_DeviceRun(g_composite.deviceHandle) != kStatus_USB_Success) {
        LOG_ERROR("[Composite] USB Device run failed");
    }



    LOG_DEBUG("[Composite] USB initialized");
    return &g_composite;
}

void USB_DeviceApplicationDeinit(usb_device_composite_struct_t *composite)
{
    usb_status_t err;
    if ((err = USB_DeviceStop(composite->deviceHandle)) != kStatus_USB_Success) {
        LOG_ERROR("[Composite] Device stop failed: 0x%x", err);
    }
    USB_DeviceSetIsr(false);
    MtpDeinit(&g_mtp);
    VirtualComDeinit(&g_cdcVcom);
    if ((err = USB_DeviceClassDeinit(CONTROLLER_ID)) != kStatus_USB_Success) {
        LOG_ERROR("[Composite] Device class deinit failed: 0x%x", err);
    }
    USB_DeviceClockDeinit();
}
