add_library(usb_stack STATIC)
add_library(usb_stack::usb_stack ALIAS usb_stack)

if (MUDITA_USB_ID)
    set (USB_DEVICE_VENDOR_ID  0x3310 CACHE INTERNAL "Sets USB_DEVICE_VENDOR_ID to Mudita Vendor ID")

    if (${PRODUCT} STREQUAL "PurePhone")
        if (ENABLE_USB_MTP)
            set (USB_DEVICE_PRODUCT_ID 0x0100 CACHE INTERNAL "Sets USB_DEVICE_PRODUCT_ID to Mudita Pure Product ID")
        else()
            set (USB_DEVICE_PRODUCT_ID 0x0102 CACHE INTERNAL "Sets USB_DEVICE_PRODUCT_ID to Mudita Pure (non-MTP) Product ID")
        endif()
    elseif (${PRODUCT} STREQUAL "BellHybrid")
        set (USB_DEVICE_PRODUCT_ID 0x0300 CACHE INTERNAL "Sets USB_DEVICE_PRODUCT_ID to Mudita Harmony Product ID")
    else ()
        message (FATAL_ERROR "Unknown product: ${PRODUCT}")
    endif ()
else()
    set (USB_DEVICE_VENDOR_ID  0x045E CACHE INTERNAL "Sets USB_DEVICE_VENDOR_ID to Microsoft Vendor ID")
    set (USB_DEVICE_PRODUCT_ID 0x0622 CACHE INTERNAL "Sets USB_DEVICE_PRODUCT_ID to Windows MTP Simulator Product ID")
endif()

if (ENABLE_USB_TASK)
    set (ENABLE_USB_DEVICE_TASK 1 CACHE INTERNAL "Turn device task in USB stack ON")
else()
    set (ENABLE_USB_DEVICE_TASK 0 CACHE INTERNAL "Turn device task in USB stack OFF")
endif()

option(USB_ENABLE_LOGS "Enable logs" OFF)

target_compile_definitions(usb_stack
    PRIVATE
        USB_DEVICE_VENDOR_ID=${USB_DEVICE_VENDOR_ID}
        USB_DEVICE_PRODUCT_ID=${USB_DEVICE_PRODUCT_ID}
        USB_DEVICE_CONFIG_MTP=$<BOOL:${ENABLE_USB_MTP}>
        USB_DEVICE_CONFIG_USE_TASK=$<BOOL:${ENABLE_USB_DEVICE_TASK}>
        $<$<BOOL:${USB_ENABLE_LOGS}>:USB_ENABLE_LOGS>
)

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
        phy/usb_phy.h
        usb_device_config.h
        usb_device_descriptor.h
        pure/usb_strings.h
        bell/usb_strings.h
        usb.hpp
)

if (ENABLE_USB_MTP)
    target_sources(usb_stack
        PRIVATE
            mtp/libmtp/mtp_container.c
            mtp/libmtp/mtp_dataset.c
            mtp/libmtp/mtp_responder.c
            mtp/libmtp/mtp_storage.c
            mtp/libmtp/mtp_util.c
            mtp/mtp_db.cpp
            mtp/mtp_fs.cpp
            mtp/mtp.c
            mtp/usb_device_mtp.c
    )
endif()

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
            phy
        >
    PRIVATE
        $<BUILD_INTERFACE:
            $<$<STREQUAL:${PRODUCT},PurePhone>:${CMAKE_CURRENT_SOURCE_DIR}/pure>
            $<$<STREQUAL:${PRODUCT},BellHybrid>:${CMAKE_CURRENT_SOURCE_DIR}/bell>
        >
)

if (ENABLE_USB_MTP)
    target_include_directories(usb_stack
        PUBLIC
            $<BUILD_INTERFACE:
                mtp
                mtp/libmtp
            >
    )
endif()

target_link_libraries(usb_stack
    PRIVATE
        $<$<BOOL:${USB_ENABLE_LOGS}>:log-api>
        module-vfs
    PUBLIC
        module-bsp
        module-os
)
