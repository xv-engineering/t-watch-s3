zephyr_library()

zephyr_library_sources(
    t_watch_s3_boot.c
    t_watch_s3_zephyr_ver.c
)

# if LoRa (specifically the soft secure element) and WiFi are both enabled, the symbol 
# aes_encrypt conflicts and results in failed linking. Therefore we have to add a small hack
# to rename the aes_encrypt symbol within the loramac-node library
if (CONFIG_HAS_SEMTECH_LORAMAC AND CONFIG_HAS_SEMTECH_SOFT_SE AND CONFIG_WIFI_ESP32)

add_custom_target(rename_loramac_node_aes_encrypt
    COMMAND ${CMAKE_OBJCOPY} --redefine-sym aes_encrypt=loramac_node_aes_encrypt $<TARGET_FILE:loramac-node>
    DEPENDS $<TARGET_FILE:loramac-node>
    COMMENT "Renaming aes_encrypt to loramac_node_aes_encrypt in libloramac-node"
    VERBATIM
)

zephyr_library_add_dependencies(rename_loramac_node_aes_encrypt)

endif()

