set(USB_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/component/lists/generic_list.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/component/osa/fsl_os_abstraction_free_rtos.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/class/cdc/usb_device_cdc_acm.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/class/mtp/usb_device_mtp.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/class/usb_device_class.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/dcd/usb_phydcd.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/ehci/usb_device_ehci.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/usb_device_ch9.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/usb_device_dci.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/usb_string_descriptor.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/phy/usb_phy.c"
        CACHE INTERNAL ""
        )

set(USB_DIR_INCLUDES
#       "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/component/lists"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/component/osa"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/class"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/class/cdc"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/class/mtp"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/dcd"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/ehci"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/phy"
        CACHE INTERNAL ""
        )
