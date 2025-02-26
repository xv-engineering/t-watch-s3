#include <zephyr/kernel.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(axp2101, CONFIG_AXP2101_LOG_LEVEL);

#define DT_DRV_COMPAT x_powers_axp2101_charger