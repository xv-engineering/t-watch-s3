//
// Copyright (c) 2025 Noah Luskey <noah@vvvvvvvvvv.io>
// SPDX-License-Identifier: Apache-2.0
//
#include "axp2101.h"

#include <errno.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(axp2101, CONFIG_AXP2101_LOG_LEVEL);

#define DT_DRV_COMPAT x_powers_axp2101

struct axp2101_config
{
    struct i2c_dt_spec i2c;
    bool button_battery_charge_enable;

    LOG_INSTANCE_PTR_DECLARE(log);
};

static int axp2101_clear_interrupt_reg(const struct axp2101_config *config, uint8_t reg, uint8_t *value)
{
    CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, reg, value), config->log);
    CHECK_OK(i2c_reg_write_byte_dt(&config->i2c, reg, *value), config->log);
    return 0;
}

static int axp2101_init(const struct device *dev)
{
    const struct axp2101_config *config = dev->config;
    LOG_INST_DBG(config->log, "Initializing instance");

    if (!i2c_is_ready_dt(&config->i2c))
    {
        LOG_INST_ERR(config->log, "I2C bus not ready");
        return -ENODEV;
    }

    // Check if axp2101 chip is available
    uint8_t chip_id;
    CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_CHIP_ID, &chip_id), config->log);
    if (chip_id != AXP2101_CHIP_ID)
    {
        LOG_INST_ERR(config->log, "Invalid Chip detected (%d)", chip_id);
        return -EINVAL;
    }

    // enable coin battery charging through VBACKUP if requested
    if (config->button_battery_charge_enable)
    {
        CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_REG, BIT(2), BIT(2)), config->log);
    }

    // disable all interrupts
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_0_REG, 0xFF, 0x00), config->log);
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_1_REG, 0xFF, 0x00), config->log);
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, AXP2101_IRQ_ENABLE_2_REG, 0xFF, 0x00), config->log);

    // clear all pending interrupts
    uint8_t value;
    CHECK_OK(axp2101_clear_interrupt_reg(config, AXP2101_IRQ_STATUS_0_REG, &value), config->log);
    CHECK_OK(axp2101_clear_interrupt_reg(config, AXP2101_IRQ_STATUS_1_REG, &value), config->log);
    CHECK_OK(axp2101_clear_interrupt_reg(config, AXP2101_IRQ_STATUS_2_REG, &value), config->log);

    // Subdevices that care about interrupts will re-enable them, and service them,
    // as appropriate. Somewhat lazily, each subdevice has its own service thread.
    // It's probably worth the effort to combine all into a single thread for
    // memory savings eventually (#16)
    return 0;
}

// I2C must be initialized before the AXP2101 driver
BUILD_ASSERT(CONFIG_AXP2101_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY);

#define AXP2101_DEFINE(inst)                                                              \
    LOG_INSTANCE_REGISTER(axp2101, inst, CONFIG_AXP2101_LOG_LEVEL);                       \
    static const struct axp2101_config config##inst = {                                   \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                \
        .button_battery_charge_enable = DT_INST_PROP(inst, button_battery_charge_enable), \
        LOG_INSTANCE_PTR_INIT(log, axp2101, inst)};                                       \
    DEVICE_DT_INST_DEFINE(inst, axp2101_init, NULL, NULL, &config##inst, POST_KERNEL,     \
                          CONFIG_AXP2101_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(AXP2101_DEFINE)
