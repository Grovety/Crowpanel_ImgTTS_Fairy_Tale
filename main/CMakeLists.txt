idf_component_register(
    SRCS
    "lvgl_port_v8.cpp"
    "main.cpp"

    "uart.c"
    "uart_manager.c"
    "uart_json.c"
    "uart_rx_task.c"

    "entities/AudioPlayer.cpp"
    "entities/HimaxModule.cpp"
    "entities/i2c_comm/i2c_master.c"
    "entities/i2s/i2s_tx.c"
    "entities/i2c_comm/crc.c"

    INCLUDE_DIRS
    .
    "entities"
    "entities/i2c_comm"
    "entities/i2s"
    
    PRIV_REQUIRES
    json
    espressif__esp32_io_expander
    
    REQUIRES
    driver
    esp_timer
    lvgl__lvgl
    ui
    spiffs
    ringbuf
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)

idf_build_get_property(build_components BUILD_COMPONENTS)
foreach(COMPONENT ${build_components})
    if(COMPONENT MATCHES "lvgl" OR COMPONENT MATCHES "lvgl__lvgl")
        set(TARGET_COMPONENT ${COMPONENT})
        break()
    endif()
endforeach()

if(TARGET_COMPONENT STREQUAL "")
    message(FATAL_ERROR "Component 'lvgl' not found.")
else()
    idf_component_get_property(LVGL_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
endif()
target_compile_options(${LVGL_LIB} PRIVATE "-Wno-format")
set(TARGET_COMPONENT "")