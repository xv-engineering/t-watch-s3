# Copyright (c) 2025 Noah Luskey <noah@vvvvvvvvvv.io>

# The project we're going to amend is actually `drivers__sensor__bosch__bma4xx`
# (which could change if the directory structure changes in the zephyr repo).
# Referring to it in this way lets us amend the library with additional code
# without needing to go in-tree

if(CONFIG_BMA4XX_BOOT_RESET_HACK)
target_sources(drivers__sensor__bosch__bma4xx PRIVATE
    bma4xx_boot_reset.c
)
endif()

if(CONFIG_BMA4XX_DECODER_PATCH_HACK)
target_sources(drivers__sensor__bosch__bma4xx PRIVATE
    bma4xx_decoder_patch.c
)
endif()

target_include_directories(drivers__sensor__bosch__bma4xx PRIVATE
    ${ZEPHYR_BASE}/drivers/sensor/bosch/bma4xx
)
