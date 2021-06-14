add_library(usb_stack STATIC)
add_library(usb_stack::usb_stack ALIAS usb_stack)

target_sources(usb_stack
    PRIVATE
        cdc/usb_device_cdc_acm.c
        cdc/virtual_com.c
        component/lists/generic_list.c
        component/osa/fsl_os_abstraction_free_rtos.c
        composite.c
        dcd/usb_phydcd.c
        device/ehci/usb_device_ehci.c
        device/usb_device_ch9.c
        device/usb_device_class.c
        device/usb_device_dci.c
        device/usb_string_descriptor.c
        mtp/libmtp/mtp_container.c
        mtp/libmtp/mtp_dataset.c
        mtp/libmtp/mtp_responder.c
        mtp/libmtp/mtp_storage.c
        mtp/libmtp/mtp_util.c
        mtp/mtp_db.c
        mtp/mtp_fs.cpp
        mtp/mtp.c
        mtp/usb_device_mtp.c
        phy/usb_phy.c
        usb_device_descriptor.c
        usb.cpp
    PUBLIC
        cdc/usb_device_cdc_acm.h
        cdc/virtual_com.h
        component/lists/generic_list.h
        component/osa/fsl_os_abstraction_config.h
        component/osa/fsl_os_abstraction_free_rtos.h
        component/osa/fsl_os_abstraction.h
        composite.h
        dcd/usb_charger_detect.h
        dcd/usb_phydcd_config.h
        dcd/usb_phydcd.h
        device/ehci/usb_device_ehci.h
        device/usb_device_ch9.h
        device/usb_device_class.h
        device/usb_device_dci.h
        device/usb_device.h
        device/usb_misc.h
        device/usb_spec.h
        device/usb_string_descriptor.h
        device/usb.h
        mtp/libmtp/mtp_container.h
        mtp/libmtp/mtp_dataset.h
        mtp/libmtp/mtp_responder.h
        mtp/libmtp/mtp_storage.h
        mtp/libmtp/mtp_util.h
        mtp/mtp_db.h
        mtp/mtp_fs.h
        mtp/mtp.h
        mtp/usb_device_mtp.h
        phy/usb_phy.h
        usb_device_config.h
        usb_device_descriptor.h
        usb_strings.h
        usb.hpp
)

target_include_directories(usb_stack
    PUBLIC
        $<BUILD_INTERFACE:
            .
            cdc
            component/lists
            component/osa
            dcd
            device
            device/ehci
            mtp
            mtp/libmtp
            phy
        >
)

target_link_libraries(usb_stack
    PRIVATE
        log
        module-vfs
    PUBLIC
        module-bsp
        module-os
)