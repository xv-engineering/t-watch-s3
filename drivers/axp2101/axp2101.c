//
// Copyright (c) 2025 Noah Luskey <noah@vvvvvvvvvv.io>
// SPDX-License-Identifier: Apache-2.0
//

// TODO (#13): The poweron GPIO being forwarded to keycodes really
// isn't the right way to abstract this in Zephyr. It
// really is more like a GPIO expander with a single
// GPIO input, and should be abstracted as a child
// node on the axp2101 parent node. Not high priority.

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
#define AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_MASK_BUTTON_CHARGER BIT(2)

// IRQ enable registers
#define AXP2101_IRQ_ENABLE_0_REG 0x40U
#define AXP2101_IRQ_ENABLE_1_REG 0x41U
#define AXP2101_IRQ_ENABLE_1_MASK_PWRON_POSITIVE_EDGE BIT(0) // PWRON positive edge
#define AXP2101_IRQ_ENABLE_1_MASK_PWRON_NEGATIVE_EDGE BIT(1) // PWRON negative edge
#define AXP2101_IRQ_ENABLE_1_MASK_PWRON_LONG_PRESS BIT(2)    // PWRON long press
#define AXP2101_IRQ_ENABLE_1_MASK_PWRON_SHORT_PRESS BIT(3)   // PWRON short press
#define AXP2101_IRQ_ENABLE_2_REG 0x42U

// IRQ status registers
#define AXP2101_IRQ_STATUS_0_REG 0x48U
#define AXP2101_IRQ_STATUS_1_REG 0x49U
#define AXP2101_IRQ_STATUS_1_MASK_PWRON_POSITIVE_EDGE BIT(0) // PWRON positive edge
#define AXP2101_IRQ_STATUS_1_MASK_PWRON_NEGATIVE_EDGE BIT(1) // PWRON negative edge
#define AXP2101_IRQ_STATUS_1_MASK_PWRON_LONG_PRESS BIT(2)    // PWRON long press
#define AXP2101_IRQ_STATUS_1_MASK_PWRON_SHORT_PRESS BIT(3)   // PWRON short press
#define AXP2101_IRQ_STATUS_2_REG 0x4AU

struct axp2101_config
{
    struct i2c_dt_spec i2c;
    bool button_battery_charge_enable;
    struct gpio_dt_spec int_gpio;
    uint16_t short_press_code;
    uint16_t long_press_code;
};

struct axp2101_data
{
    struct gpio_callback gpio_cb;
    struct k_sem sem;
    const struct axp2101_config *config;
    const struct device *self;
};

#define CHECK_OK(ret)                  \
    do                                 \
    {                                  \
        if (ret < 0)                   \
        {                              \
            LOG_ERR("Error: %d", ret); \
            return ret;                \
        }                              \
    } while (0)

static int axp2101_clear_interrupt_reg(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t *value)
{
    CHECK_OK(i2c_reg_read_byte_dt(i2c, reg, value));
    CHECK_OK(i2c_reg_write_byte_dt(i2c, reg, *value));
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
        axp2101_clear_interrupt_reg(&config->i2c, AXP2101_IRQ_STATUS_0_REG, &value);

        value = 0;
        axp2101_clear_interrupt_reg(&config->i2c, AXP2101_IRQ_STATUS_1_REG, &value);

        if ((value & AXP2101_IRQ_STATUS_1_MASK_PWRON_SHORT_PRESS) && (config->short_press_code != INPUT_KEY_RESERVED))
        {
            // short press - by convention, 0 is sent on release
            input_report_key(data->self, config->short_press_code, 0, false, K_NO_WAIT);
        }

        if ((value & AXP2101_IRQ_STATUS_1_MASK_PWRON_LONG_PRESS) && (config->long_press_code != INPUT_KEY_RESERVED))
        {
            // long press - by convention, 0 is sent on release
            input_report_key(data->self, config->long_press_code, 0, false, K_NO_WAIT);
        }

        axp2101_clear_interrupt_reg(&config->i2c, AXP2101_IRQ_STATUS_2_REG, &value);
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

    LOG_DBG("Initializing instance");

    if (!i2c_is_ready_dt(&config->i2c))
    {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    // Check if axp2101 chip is available
    uint8_t chip_id;
    CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_CHIP_ID, &chip_id));
    if (chip_id != AXP2101_CHIP_ID)
    {
        LOG_ERR("Invalid Chip detected (%d)", chip_id);
        return -EINVAL;
    }

    // enable coin battery charging through VBACKUP if requested
    if (config->button_battery_charge_enable)
    {
        CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_REG, BIT(2), BIT(2)));
    }

    // disable all interrupts
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_0_REG, 0xFF, 0x00));
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_1_REG, 0xFF, 0x00));
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_2_REG, 0xFF, 0x00));

    // clear all pending interrupts
    uint8_t value;
    CHECK_OK(axp2101_clear_interrupt_reg(&config->i2c, AXP2101_IRQ_STATUS_0_REG, &value));
    CHECK_OK(axp2101_clear_interrupt_reg(&config->i2c, AXP2101_IRQ_STATUS_1_REG, &value));
    CHECK_OK(axp2101_clear_interrupt_reg(&config->i2c, AXP2101_IRQ_STATUS_2_REG, &value));

    // configure button and register the interrupt callback
    CHECK_OK(gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | GPIO_ACTIVE_LOW));
    CHECK_OK(gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE));
    gpio_init_callback(&data->gpio_cb, axp2101_interrupt_callback, BIT(config->int_gpio.pin));
    CHECK_OK(gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb));

    // and configure the interrupt register for short and long presses
    const uint8_t mask =
        AXP2101_IRQ_ENABLE_1_MASK_PWRON_LONG_PRESS |
        AXP2101_IRQ_ENABLE_1_MASK_PWRON_SHORT_PRESS;
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_1_REG, mask, mask));

    return 0;
}

// I2C must be initialized before the AXP2101 driver
BUILD_ASSERT(CONFIG_AXP2101_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY);

#define AXP2101_DEFINE(inst)                                                                 \
    static const struct axp2101_config config##inst = {                                      \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                   \
        .button_battery_charge_enable = DT_INST_PROP(inst, button_battery_charge_enable),    \
        .int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                  \
        .short_press_code = DT_INST_PROP_OR(inst, short_press_code, INPUT_KEY_RESERVED),     \
        .long_press_code = DT_INST_PROP_OR(inst, long_press_code, INPUT_KEY_RESERVED)};      \
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
