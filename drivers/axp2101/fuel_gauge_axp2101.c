#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/fuel_gauge.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fuel_gauge_axp2101, CONFIG_AXP2101_LOG_LEVEL);

#define DT_DRV_COMPAT x_powers_axp2101_fuel_gauge

struct fuel_gauge_axp2101_config
{
    struct i2c_dt_spec i2c;
    LOG_INSTANCE_PTR_DECLARE(log);
};

static int fuel_gauge_axp2101_get_property(const struct device *dev, fuel_gauge_prop_t prop,
                                           union fuel_gauge_prop_val *val)
{
    const struct fuel_gauge_axp2101_config *config = dev->config;
    LOG_INST_WRN(config->log, "Not implemented");
    return 0;
}

static int fuel_gauge_axp2101_set_property(const struct device *dev, fuel_gauge_prop_t prop,
                                           union fuel_gauge_prop_val val)
{
    const struct fuel_gauge_axp2101_config *config = dev->config;
    LOG_INST_WRN(config->log, "Not implemented");
    return 0;
}

static int fuel_gauge_axp2101_get_buffer_property(const struct device *dev,
                                                  fuel_gauge_prop_t prop_type,
                                                  void *dst, size_t dst_len)
{
    const struct fuel_gauge_axp2101_config *config = dev->config;
    LOG_INST_WRN(config->log, "Not implemented");
    return 0;
}

static int fuel_gauge_axp2101_battery_cutoff(const struct device *dev)
{
    const struct fuel_gauge_axp2101_config *config = dev->config;
    LOG_INST_WRN(config->log, "Not implemented");
    return 0;
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
                          POST_KERNEL, CONFIG_FUEL_GAUGE_AXP2101_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FUEL_GAUGE_AXP2101_DEFINE)
