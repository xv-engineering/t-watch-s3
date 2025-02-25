//
// Copyright (c) 2025 Noah Luskey <noah@vvvvvvvvvv.io>
// SPDX-License-Identifier: Apache-2.0
//

#define DT_DRV_COMPAT x_powers_axp2101_gpio

#include "reg_axp2101.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_axp2101, CONFIG_AXP2101_LOG_LEVEL);

#define CHECK_OK(ret)                  \
    do                                 \
    {                                  \
        if (ret < 0)                   \
        {                              \
            LOG_ERR("Error: %d", ret); \
            return ret;                \
        }                              \
    } while (0)

struct gpio_axp2101_config
{
    struct gpio_dt_spec int_gpio;
    struct i2c_dt_spec i2c;
};

struct gpio_axp2101_data
{
    struct gpio_callback gpio_cb;
    struct k_sem sem;
    const struct device *self;
};

static void gpio_axp2101_thread(void *d, void *, void *)
{
    struct gpio_axp2101_data *data = d;
    const struct device *dev = data->self;
    const struct gpio_axp2101_config *config = dev->config;

    while (1)
    {
        k_sem_take(&data->sem, K_FOREVER);
        LOG_PRINTK("GPIO interrupt\n");

        // we are only interested in the EDGE interrupt flags. Any other flags
        // are not the responsibility of the GPIO node.
        uint8_t irq_status;
        int ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_IRQ_STATUS_1_REG, &irq_status);
        if (ret < 0)
        {
            LOG_ERR("Could not read IRQ status: %d", ret);
            continue;
        }
        if (irq_status & (AXP2101_IRQ_STATUS_1_MASK_PWRON_NEGATIVE_EDGE |
                          AXP2101_IRQ_STATUS_1_MASK_PWRON_POSITIVE_EDGE))
        {
            LOG_PRINTK("GPIO interrupt\n");
            LOG_PRINTK("IRQ status: %d\n", irq_status);
        }

        // write back to clear those flags
        ret = i2c_reg_write_byte_dt(&config->i2c, AXP2101_IRQ_STATUS_1_REG, irq_status);
        if (ret < 0)
        {
            LOG_ERR("Could not clear IRQ status: %d", ret);
            continue;
        }
    }
}

static void gpio_axp2101_interrupt_callback(const struct device *port,
                                            struct gpio_callback *cb,
                                            gpio_port_pins_t pins)
{
    struct gpio_axp2101_data *data = CONTAINER_OF(cb, struct gpio_axp2101_data, gpio_cb);
    k_sem_give(&data->sem);
}

static int gpio_axp2101_init(const struct device *dev)
{
    struct gpio_axp2101_data *data = dev->data;
    const struct gpio_axp2101_config *config = dev->config;
    data->self = dev;

    CHECK_OK(gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | GPIO_ACTIVE_LOW));
    CHECK_OK(gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE));
    gpio_init_callback(&data->gpio_cb, gpio_axp2101_interrupt_callback, BIT(config->int_gpio.pin));
    CHECK_OK(gpio_add_callback(config->int_gpio.port, &data->gpio_cb));

    // and configure the interrupt register for edge detection
    const uint8_t mask =
        AXP2101_IRQ_ENABLE_1_MASK_PWRON_NEGATIVE_EDGE |
        AXP2101_IRQ_ENABLE_1_MASK_PWRON_POSITIVE_EDGE;
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_1_REG, mask, mask));

    return 0;
}

#define GPIO_AXP2101_DEFINE(inst)                                                        \
    static const struct gpio_axp2101_config config##inst = {                             \
        .int_gpio = GPIO_DT_SPEC_GET(DT_INST_PARENT(inst), int_gpios),                   \
        .i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                    \
    };                                                                                   \
    static struct gpio_axp2101_data data##inst = {                                       \
        .sem = Z_SEM_INITIALIZER(data##inst.sem, 0, 1),                                  \
    };                                                                                   \
    K_THREAD_DEFINE(gpio_axp2101_thread_##inst, 1024, gpio_axp2101_thread, &data##inst,  \
                    NULL, NULL, K_PRIO_COOP(CONFIG_GPIO_AXP2101_THREAD_PRIORITY), 0, 0); \
    DEVICE_DT_INST_DEFINE(inst, gpio_axp2101_init, NULL, &data##inst, &config##inst,     \
                          POST_KERNEL, CONFIG_GPIO_AXP2101_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AXP2101_DEFINE)
