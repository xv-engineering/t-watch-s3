#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_axp2101, CONFIG_AXP2101_LOG_LEVEL);

#define DT_DRV_COMPAT x_powers_axp2101_gpio

struct gpio_axp2101_config
{
    // intentionally empty
};

struct gpio_axp2101_data
{
    // intentionally empty
};

static int gpio_axp2101_init(const struct device *dev)
{
    return 0;
}

#define GPIO_AXP2101_DEFINE(inst)                                                    \
    static const struct gpio_axp2101_config config##inst = {};                       \
    static struct gpio_axp2101_data data##inst = {};                                 \
    DEVICE_DT_INST_DEFINE(inst, gpio_axp2101_init, NULL, &data##inst, &config##inst, \
                          POST_KERNEL, CONFIG_GPIO_AXP2101_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AXP2101_DEFINE)
