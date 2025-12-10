idf_build_get_property(__hack_component_targets __COMPONENT_TARGETS)

list(APPEND MODBUS_PRIV_REQUIRES
    bblanchon__arduinojson
    app_update
)

if("arduino" IN_LIST BUILD_COMPONENTS OR __hack_component_targets MATCHES "__idf_arduino")
    list(APPEND MODBUS_PRIV_REQUIRES
        arduino
    )
elseif("framework-arduinoespressif32" IN_LIST BUILD_COMPONENTS OR __hack_component_targets MATCHES "___idf_framework-arduinoespressif32")
    list(APPEND MODBUS_PRIV_REQUIRES
        framework-arduinoespressif32
    )
elseif("espressif__arduino-esp32" IN_LIST BUILD_COMPONENTS OR __hack_component_targets MATCHES "___idf_espressif__arduino-esp32")
    list(APPEND MODBUS_PRIV_REQUIRES
        espressif__arduino-esp32
    )
endif()

idf_component_register(
    SRCS "src/esp32ModbusRTU.cpp" "src/ModbusMessage.cpp"
    INCLUDE_DIRS "src"
    PRIV_REQUIRES ${MODBUS_PRIV_REQUIRES}
)
