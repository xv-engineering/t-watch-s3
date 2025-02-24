//
// Copyright (c) 2025 Noah Luskey <noah@vvvvvvvvvv.io>
// SPDX-License-Identifier: Apache-2.0
//

#define DT_DRV_COMPAT x_powers_axp2101

#include <errno.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>

LOG_MODULE_REGISTER(axp2101, CONFIG_AXP2101_LOG_LEVEL);

// Chip ID value and register
#define AXP2101_CHIP_ID 0x4A
#define AXP2101_REG_CHIP_ID 0x03U

// Register and bit to enable button battery charging (for RTC)
#define AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_REG 0x18U
#define AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_BUTTON_CHARGER_MASK BIT(2)

// IRQ register for controlling PWRON button interrupts
#define AXP2101_IRQ_ENABLE_1_REG 0x40U
#define AXP2101_IRQ_ENABLE_1_PWRON_POSITIVE_EDGE_MASK BIT(0)
#define AXP2101_IRQ_ENABLE_1_PWRON_NEGATIVE_EDGE_MASK BIT(1)
#define AXP2101_IRQ_ENABLE_1_PWRON_LONG_PRESS_ENABLE_MASK BIT(2)
#define AXP2101_IRQ_ENABLE_1_PWRON_SHORT_PRESS_ENABLE_MASK BIT(3)

// IRQ status registers
#define AXP2101_IRQ_STATUS_0_REG 0x48U
#define AXP2101_IRQ_STATUS_1_REG 0x49U
#define AXP2101_IRQ_STATUS_2_REG 0x4AU

struct axp2101_config
{
        struct i2c_dt_spec i2c;
        bool button_battery_charge_enable;
        struct gpio_dt_spec int_gpio;
        uint16_t short_press_key;
        uint16_t long_press_key;
};

struct axp2101_data
{
        struct gpio_callback gpio_cb;
        struct k_sem sem;
        const struct axp2101_config *config;
        const struct device *self;
};

static int axp2101_clear_interrupts(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t *value)
{
        int ret = i2c_reg_read_byte_dt(i2c, reg, value);
        if (ret < 0)
        {
                LOG_ERR("Could not read interrupt register (%d)", ret);
                return ret;
        }

        ret = i2c_reg_write_byte_dt(i2c, reg, *value);
        if (ret < 0)
        {
                LOG_ERR("Could not write interrupt register (%d)", ret);
                return ret;
        }

        return 0;
}

static void axp2101_thread(void *d, void *, void *)
{
        struct axp2101_data *data = d;
        const struct axp2101_config *config = data->config;
        while (1)
        {
                k_sem_take(&data->sem, K_FOREVER);

                // clear all interrupts, but we only care about the status 1 reg right now
                uint8_t value = 0;
                axp2101_clear_interrupts(&config->i2c, AXP2101_IRQ_STATUS_0_REG, &value);

                value = 0;
                axp2101_clear_interrupts(&config->i2c, AXP2101_IRQ_STATUS_1_REG, &value);

                if ((value & AXP2101_IRQ_ENABLE_1_PWRON_SHORT_PRESS_ENABLE_MASK) && (config->short_press_key != INPUT_KEY_RESERVED))
                {
                        input_report_key(data->self, config->short_press_key, 1, false, K_NO_WAIT);
                }

                if ((value & AXP2101_IRQ_ENABLE_1_PWRON_LONG_PRESS_ENABLE_MASK) && (config->long_press_key != INPUT_KEY_RESERVED))
                {
                        input_report_key(data->self, config->long_press_key, 1, false, K_NO_WAIT);
                }

                axp2101_clear_interrupts(&config->i2c, AXP2101_IRQ_STATUS_2_REG, &value);
        }
}

static void axp2101_interrupt_callback(const struct device *port,
                                       struct gpio_callback *cb,
                                       gpio_port_pins_t pins)
{
        struct axp2101_data *data = CONTAINER_OF(cb, struct axp2101_data, gpio_cb);
        k_sem_give(&data->sem);
}

static int axp2101_init(const struct device *dev)
{
        const struct axp2101_config *config = dev->config;
        struct axp2101_data *data = dev->data;
        data->self = dev;
        uint8_t chip_id;
        int ret;

        LOG_DBG("Initializing instance");

        if (!i2c_is_ready_dt(&config->i2c))
        {
                LOG_ERR("I2C bus not ready");
                return -ENODEV;
        }

        // Check if axp2101 chip is available
        ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_CHIP_ID, &chip_id);
        if (ret < 0)
        {
                return ret;
        }

        if (chip_id != AXP2101_CHIP_ID)
        {
                LOG_ERR("Invalid Chip detected (%d)", chip_id);
                return -EINVAL;
        }

        // enable coin battery charging through VBACKUP if requested
        if (config->button_battery_charge_enable)
        {
                ret = i2c_reg_update_byte_dt(&config->i2c, AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_REG, BIT(2), BIT(2));
                if (ret < 0)
                {
                        LOG_ERR("Could not enable coin battery charging (%d)", ret);
                        return ret;
                }
        }

        // configure the button
        ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | GPIO_ACTIVE_LOW);
        if (ret < 0)
        {
                LOG_ERR("Could not configure interrupt GPIO (%d)", ret);
                return ret;
        }

        // configure the interrupt
        ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
        if (ret < 0)
        {
                LOG_ERR("Could not configure interrupt GPIO (%d)", ret);
                return ret;
        }

        // read all current interrupts and write back to clear all of them
        uint8_t value;
        ret = axp2101_clear_interrupts(&config->i2c, AXP2101_IRQ_STATUS_0_REG, &value);
        if (ret < 0)
        {
                LOG_ERR("Could not clear interrupts (%d)", ret);
                return ret;
        }

        ret = axp2101_clear_interrupts(&config->i2c, AXP2101_IRQ_STATUS_1_REG, &value);
        if (ret < 0)
        {
                LOG_ERR("Could not clear interrupts (%d)", ret);
                return ret;
        }

        ret = axp2101_clear_interrupts(&config->i2c, AXP2101_IRQ_STATUS_2_REG, &value);
        if (ret < 0)
        {
                LOG_ERR("Could not clear interrupts (%d)", ret);
                return ret;
        }

        // register the interrupt callback
        gpio_init_callback(&data->gpio_cb, axp2101_interrupt_callback, BIT(config->int_gpio.pin));
        ret = gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb);
        if (ret < 0)
        {
                LOG_ERR("Could not add interrupt callback (%d)", ret);
                return ret;
        }

        // and configure the interrupt register for all pwron events
        const uint8_t mask =
            AXP2101_IRQ_ENABLE_1_PWRON_POSITIVE_EDGE_MASK |
            AXP2101_IRQ_ENABLE_1_PWRON_NEGATIVE_EDGE_MASK |
            AXP2101_IRQ_ENABLE_1_PWRON_LONG_PRESS_ENABLE_MASK |
            AXP2101_IRQ_ENABLE_1_PWRON_SHORT_PRESS_ENABLE_MASK;
        const uint8_t reg_value = AXP2101_IRQ_ENABLE_1_PWRON_NEGATIVE_EDGE_MASK | AXP2101_IRQ_ENABLE_1_PWRON_POSITIVE_EDGE_MASK;
        ret = i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_1_REG, mask, reg_value);
        if (ret < 0)
        {
                LOG_ERR("Could not update interrupt register (%d)", ret);
                return ret;
        }

        return 0;
}

// I2C must be initialized before the AXP2101 driver
BUILD_ASSERT(CONFIG_AXP2101_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY);

#define AXP2101_DEFINE(inst)                                                                     \
        static const struct axp2101_config config##inst = {                                      \
            .i2c = I2C_DT_SPEC_INST_GET(inst),                                                   \
            .button_battery_charge_enable = DT_INST_PROP(inst, button_battery_charge_enable),    \
            .int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                  \
            .short_press_key = DT_INST_PROP_OR(inst, short_press_key, INPUT_KEY_RESERVED),       \
            .long_press_key = DT_INST_PROP_OR(inst, long_press_key, INPUT_KEY_RESERVED)};        \
        static struct axp2101_data data##inst = {                                                \
            .sem = Z_SEM_INITIALIZER(data##inst.sem, 0, 1),                                      \
            .config = &config##inst,                                                             \
        };                                                                                       \
        K_THREAD_DEFINE(axp2101_thread_##inst, 1024, axp2101_thread, &data##inst,                \
                        NULL, NULL, K_PRIO_COOP(CONFIG_AXP2101_THREAD_PRIORITY), 0, 0);          \
                                                                                                 \
        DEVICE_DT_INST_DEFINE(inst, axp2101_init, NULL, &data##inst, &config##inst, POST_KERNEL, \
                              CONFIG_AXP2101_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(AXP2101_DEFINE)
