set(USB_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/component/lists/generic_list.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/component/osa/fsl_os_abstraction_free_rtos.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/ehci/usb_device_ehci.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/usb_device_ch9.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/usb_device_class.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/usb_device_dci.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/usb_string_descriptor.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/cdc/usb_device_cdc_acm.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/cdc/virtual_com.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/dcd/usb_phydcd.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/mtp/libmtp/mtp_file_system_adapter.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/mtp/libmtp/mtp_object_handle.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/mtp/libmtp/mtp_operation.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/mtp/mtp.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/mtp/usb_device_mtp.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/mtp/usb_device_mtp_operation.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/phy/usb_phy.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/usb_device_descriptor.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/usb_device_init.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/composite.c"
	    "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/usb.cpp"
        CACHE INTERNAL ""
        )

set(USB_DIR_INCLUDES
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/cdc"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/component/lists"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/component/osa"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/dcd"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/device/ehci"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/mtp/libmtp"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/mtp"
        "${CMAKE_CURRENT_SOURCE_DIR}/board/rt1051/bsp/usb/phy"
        CACHE INTERNAL ""
        )
