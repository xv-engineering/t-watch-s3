#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_axp2101, CONFIG_AXP2101_LOG_LEVEL);

static int gpio_axp2101_init(const struct device *dev)
{
    return 0;
}
