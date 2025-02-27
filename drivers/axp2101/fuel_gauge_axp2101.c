#include "axp2101.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/fuel_gauge.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fuel_gauge_axp2101, CONFIG_AXP2101_LOG_LEVEL);

#define DT_DRV_COMPAT x_powers_axp2101_fuel_gauge

#define AXP2101_REG_VBAT_H 0x34U
#define AXP2101_REG_VBAT_H_MASK_VBAT (~(BIT(7) | BIT(6)))
#define AXP2101_REG_VBAT_L 0x35U
#define AXP2101_REG_VBAT_L_MASK_VBAT 0xFF
#define AXP2101_REG_BATTERY_PERCENTAGE_DATA 0xA4U

struct fuel_gauge_axp2101_config
{
    struct i2c_dt_spec i2c;
    LOG_INSTANCE_PTR_DECLARE(log);
};

static int fuel_gauge_axp2101_get_property(const struct device *dev, fuel_gauge_prop_t prop,
                                           union fuel_gauge_prop_val *val)
{
    __ASSERT_NO_MSG(dev != NULL && val != NULL);
    const struct fuel_gauge_axp2101_config *config = dev->config;
    uint8_t reg_value;
    switch (prop)
    {
    case FUEL_GAUGE_VOLTAGE:
        CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_VBAT_H, &reg_value), config->log);
        val->voltage = (uint16_t)(reg_value & AXP2101_REG_VBAT_H_MASK_VBAT) << 8;
        CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_VBAT_L, &reg_value), config->log);
        val->voltage |= reg_value & AXP2101_REG_VBAT_L_MASK_VBAT;
        val->voltage *= 1000; // chip units are mV, API expects uV
        break;
    case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
        CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_BATTERY_PERCENTAGE_DATA, &reg_value), config->log);
        val->absolute_state_of_charge = reg_value;
        break;
    default:
        LOG_INST_WRN(config->log, "property %d not supported", prop);
        return -ENOTSUP;
    }
    return 0;
}

static int fuel_gauge_axp2101_set_property(const struct device *dev, fuel_gauge_prop_t,
                                           union fuel_gauge_prop_val)
{
    __ASSERT_NO_MSG(dev != NULL);
    const struct fuel_gauge_axp2101_config *config = dev->config;
    LOG_INST_WRN(config->log, "Not supported");
    return -ENOTSUP;
}

static int fuel_gauge_axp2101_get_buffer_property(const struct device *dev,
                                                  fuel_gauge_prop_t,
                                                  void *, size_t)
{
    __ASSERT_NO_MSG(dev != NULL);
    // not a clue why you'd use this over just get_property
    const struct fuel_gauge_axp2101_config *config = dev->config;
    LOG_INST_WRN(config->log, "Not implemented");
    return -ENOTSUP;
}

static int fuel_gauge_axp2101_battery_cutoff(const struct device *dev)
{
    // axp2101 does not support manual control of BATFET
    const struct fuel_gauge_axp2101_config *config = dev->config;
    LOG_INST_WRN(config->log, "Not supported");
    return -ENOTSUP;
}

static struct fuel_gauge_driver_api fuel_gauge_axp2101_driver_api = {
    .battery_cutoff = fuel_gauge_axp2101_battery_cutoff,
    .get_buffer_property = fuel_gauge_axp2101_get_buffer_property,
    .get_property = fuel_gauge_axp2101_get_property,
    .set_property = fuel_gauge_axp2101_set_property,
};

static int fuel_gauge_axp2101_init(const struct device *dev)
{
    const struct fuel_gauge_axp2101_config *config = dev->config;
    LOG_INST_DBG(config->log, "Initialized");
    return 0;
}

#define FUEL_GAUGE_AXP2101_DEFINE(inst)                                             \
    LOG_INSTANCE_REGISTER(fuel_gauge_axp2101, inst, CONFIG_AXP2101_LOG_LEVEL);      \
    static const struct fuel_gauge_axp2101_config config##inst = {                  \
        .i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                               \
        LOG_INSTANCE_PTR_INIT(log, fuel_gauge_axp2101, inst)};                      \
    DEVICE_DT_INST_DEFINE(inst, fuel_gauge_axp2101_init, NULL, NULL, &config##inst, \
                          POST_KERNEL, CONFIG_FUEL_GAUGE_AXP2101_INIT_PRIORITY,     \
                          &fuel_gauge_axp2101_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FUEL_GAUGE_AXP2101_DEFINE)
