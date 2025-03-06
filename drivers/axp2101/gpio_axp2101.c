//
// Copyright (c) 2025 Noah Luskey <noah@vvvvvvvvvv.io>
// SPDX-License-Identifier: Apache-2.0
//

#include "axp2101.h"
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT x_powers_axp2101_gpio

LOG_MODULE_REGISTER(gpio_axp2101, CONFIG_AXP2101_LOG_LEVEL);

struct gpio_axp2101_config
{
    // gpio_driver_config needs to be first
    struct gpio_driver_config drv_cfg;
    struct gpio_dt_spec int_gpio;
    struct i2c_dt_spec i2c;

    LOG_INSTANCE_PTR_DECLARE(log);
};

struct gpio_axp2101_data
{
    // gpio_driver_data needs to be first
    struct gpio_driver_data common;

    // for receiving the interrupt from the axp2101
    const struct device *dev;
    struct gpio_callback gpio_cb;
    struct k_work work;

    // for generating new callbacks when the appropriate
    // interrupt from the axp2101 occurs
    enum gpio_int_mode int_mode;
    enum gpio_int_trig int_trig;
    sys_slist_t cb;

    // state of the single GPIO
    volatile bool raw;
};

static int gpio_axp2101_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
    struct gpio_axp2101_data *data = dev->data;
    if ((pin != 0) || (flags & GPIO_OUTPUT))
    {
        LOG_ERR("Bad arguments (pin: %d, flags: %d)", pin, flags);
        return -ENOTSUP;
    }

    if (flags & GPIO_ACTIVE_LOW)
    {
        data->common.invert |= BIT(pin);
    }
    else
    {
        data->common.invert &= ~BIT(pin);
    }

    return 0;
}

static int gpio_axp2101_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
    struct gpio_axp2101_data *data = dev->data;
    *value = data->raw ? BIT(0) : 0;
    return 0;
}

static int gpio_axp2101_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask, gpio_port_value_t value)
{
    LOG_ERR("port_set_masked_raw not supported");
    return -ENOTSUP;
}

static int gpio_axp2101_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
    LOG_ERR("port_set_bits_raw not supported");
    return -ENOTSUP;
}

static int gpio_axp2101_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
    LOG_ERR("port_clear_bits_raw not supported");
    return -ENOTSUP;
}

static int gpio_axp2101_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
    LOG_ERR("port_toggle_bits not supported");
    return -ENOTSUP;
}

static int gpio_axp2101_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
                                                enum gpio_int_mode mode, enum gpio_int_trig trig)
{
    if (pin != 0)
    {
        LOG_ERR("Bad arguments (pin: %d)", pin);
        return -ENOTSUP;
    }

    if ((mode != GPIO_INT_MODE_EDGE) && (mode != GPIO_INT_MODE_DISABLED))
    {
        // GPIO_INT_MODE_LEVEL not supported
        LOG_ERR("Bad arguments (mode: %d)", mode);
        return -ENOTSUP;
    }

    if ((trig != GPIO_INT_TRIG_BOTH) && (trig != GPIO_INT_TRIG_LOW) && (trig != GPIO_INT_TRIG_HIGH))
    {
        LOG_ERR("Bad arguments (trig: %d)", trig);
        return -ENOTSUP;
    }

    struct gpio_axp2101_data *data = dev->data;
    data->int_mode = mode;
    data->int_trig = trig;

    return 0;
}

static int gpio_axp2101_manage_callback(const struct device *dev, struct gpio_callback *callback, bool set)
{
    __ASSERT(callback, "No callback!");
    __ASSERT(callback->handler, "No callback handler!");

    struct gpio_axp2101_data *data = dev->data;
    sys_slist_t *callbacks = &data->cb;

    if (!sys_slist_is_empty(callbacks))
    {
        if (!sys_slist_find_and_remove(callbacks, &callback->node))
        {
            if (!set)
            {
                return -EINVAL;
            }
        }
    }
    else if (!set)
    {
        return -EINVAL;
    }

    if (set)
    {
        sys_slist_prepend(callbacks, &callback->node);
    }

    return 0;
}

static uint32_t gpio_axp2101_get_pending_int(const struct device *dev)
{
    LOG_ERR("get_pending_int not supported");
    return -ENOTSUP;
}

static const struct gpio_driver_api gpio_axp2101_driver_api = {
    .pin_configure = gpio_axp2101_pin_configure,
    .port_get_raw = gpio_axp2101_port_get_raw,
    .port_set_masked_raw = gpio_axp2101_port_set_masked_raw,
    .port_set_bits_raw = gpio_axp2101_port_set_bits_raw,
    .port_clear_bits_raw = gpio_axp2101_port_clear_bits_raw,
    .port_toggle_bits = gpio_axp2101_port_toggle_bits,
    .pin_interrupt_configure = gpio_axp2101_pin_interrupt_configure,
    .manage_callback = gpio_axp2101_manage_callback,
    .get_pending_int = gpio_axp2101_get_pending_int,
};

static void gpio_axp2101_int_work(struct k_work *work)
{
    struct gpio_axp2101_data *data = CONTAINER_OF(work, struct gpio_axp2101_data, work);
    const struct device *dev = data->dev;
    const struct gpio_axp2101_config *config = dev->config;

    LOG_INST_DBG(config->log, "Interrupt received");

    // we are only interested in the EDGE interrupt flags. Any other flags
    // are not the responsibility of the GPIO node.
    const uint8_t flag_mask = AXP2101_IRQ_STATUS_1_MASK_PWRON_NEGATIVE_EDGE |
                              AXP2101_IRQ_STATUS_1_MASK_PWRON_POSITIVE_EDGE;
    uint8_t irq_status;
    int ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_IRQ_STATUS_1_REG, &irq_status);
    if (ret < 0)
    {
        LOG_INST_ERR(config->log, "Could not read IRQ status: %d", ret);
        return;
    }

    if (irq_status & flag_mask)
    {
        if (irq_status & AXP2101_IRQ_STATUS_1_MASK_PWRON_NEGATIVE_EDGE)
        {
            data->raw = false;
        }
        if (irq_status & AXP2101_IRQ_STATUS_1_MASK_PWRON_POSITIVE_EDGE)
        {
            data->raw = true;
        }
    }

    // clear only the edge flags
    ret = i2c_reg_write_byte_dt(&config->i2c, AXP2101_IRQ_STATUS_1_REG, flag_mask);
    if (ret < 0)
    {
        LOG_INST_ERR(config->log, "Could not clear IRQ status: %d", ret);
        return;
    }

    // handle the callbacks as appropriate
    bool should_fire = false;
    if (data->int_mode == GPIO_INT_MODE_EDGE)
    {
        if (data->int_trig == GPIO_INT_TRIG_BOTH)
        {
            should_fire = true;
        }
        else if (data->int_trig == GPIO_INT_TRIG_LOW)
        {
            should_fire = !data->raw;
        }
        else if (data->int_trig == GPIO_INT_TRIG_HIGH)
        {
            should_fire = data->raw;
        }
    }

    if (should_fire)
    {
        struct gpio_callback *cb, *tmp;
        SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->cb, cb, tmp, node)
        {
            if (cb->pin_mask & BIT(0))
            {
                __ASSERT(cb->handler, "No callback handler!");
                cb->handler(dev, cb, cb->pin_mask & BIT(0));
            }
        }
    }
}

static void gpio_axp2101_interrupt_callback(const struct device *port,
                                            struct gpio_callback *cb,
                                            gpio_port_pins_t pins)
{
    struct gpio_axp2101_data *data = CONTAINER_OF(cb, struct gpio_axp2101_data, gpio_cb);
    k_work_submit(&data->work);
}

static int gpio_axp2101_init(const struct device *dev)
{
    struct gpio_axp2101_data *data = dev->data;
    const struct gpio_axp2101_config *config = dev->config;

    CHECK_OK(gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | GPIO_ACTIVE_LOW), config->log);
    CHECK_OK(gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE), config->log);
    gpio_init_callback(&data->gpio_cb, gpio_axp2101_interrupt_callback, BIT(config->int_gpio.pin));
    CHECK_OK(gpio_add_callback(config->int_gpio.port, &data->gpio_cb), config->log);

    // and configure the interrupt register for edge detection
    const uint8_t mask =
        AXP2101_IRQ_ENABLE_1_MASK_PWRON_NEGATIVE_EDGE |
        AXP2101_IRQ_ENABLE_1_MASK_PWRON_POSITIVE_EDGE;
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_1_REG, mask, mask), config->log);

    LOG_INST_DBG(config->log, "Initialized");
    return 0;
}

#define GPIO_AXP2101_DEFINE(inst)                                                    \
    LOG_INSTANCE_REGISTER(gpio_axp2101, inst, CONFIG_AXP2101_LOG_LEVEL);             \
    static const struct gpio_axp2101_config config##inst = {                         \
        .drv_cfg = {                                                                 \
            .port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),                  \
        },                                                                           \
        .int_gpio = GPIO_DT_SPEC_GET(DT_INST_PARENT(inst), int_gpios),               \
        .i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                \
        LOG_INSTANCE_PTR_INIT(log, gpio_axp2101, inst)};                             \
    static struct gpio_axp2101_data data##inst = {                                   \
        .dev = DEVICE_DT_INST_GET(inst),                                             \
        .raw = DT_INST_PROP(inst, initial_state_high),                               \
        .work = Z_WORK_INITIALIZER(gpio_axp2101_int_work),                           \
    };                                                                               \
    DEVICE_DT_INST_DEFINE(inst, gpio_axp2101_init, NULL, &data##inst, &config##inst, \
                          POST_KERNEL, CONFIG_GPIO_AXP2101_INIT_PRIORITY, &gpio_axp2101_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AXP2101_DEFINE)
