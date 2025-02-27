#include <zephyr/kernel.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include "axp2101.h"
#define DT_DRV_COMPAT x_powers_axp2101_charger

#define AXP2101_REG_PMU_STATUS_1 0x00
#define AXP2101_REG_PMU_STATUS_1_MASK_BATTERY_PRESENT BIT(3)
#define AXP2101_REG_PMU_STATUS_1_MASK_VBUS_GOOD BIT(5)

#define AXP2101_REG_PMU_STATUS_2 0x01
#define AXP2101_REG_PMU_STATUS_2_MASK_BATTERY_CURRENT_CHARGE BIT(5)
#define AXP2101_REG_PMU_STATUS_2_MASK_BATTERY_CURRENT_DISCHARGE BIT(6)

#define AXP2101_REG_IPRECHG_CURRENT_SETTING 0x61
#define AXP2101_REG_ICC_CHARGER_SETTING 0x62
#define AXP2101_REG_ITERM_CHARGER_SETTING_AND_CONTROL 0x63
#define AXP2101_REG_CV_CHARGER_VOLTAGE_SETTING 0x64

LOG_MODULE_REGISTER(charger_axp2101, CONFIG_CHARGER_LOG_LEVEL);

struct charger_axp2101_config
{
    struct i2c_dt_spec i2c;

    uint32_t ocv_capacity_table_0[11];
    uint32_t charge_full_design_microamp_hours;
    uint32_t re_charge_voltage_microvolt;
    uint32_t precharge_current_microamp;
    uint32_t charge_term_current_microamp;
    uint32_t constant_charge_current_max_microamp;
    uint32_t constant_charge_voltage_max_microvolt;

    LOG_INSTANCE_PTR_DECLARE(log);
};

struct charger_axp2101_desc
{
    const uint8_t reg;
    const uint8_t mask;
    const uint8_t bitpos;
    const struct linear_range *ranges;
    const uint8_t num_ranges;
};

static const struct linear_range precharge_ranges_ua[] = {
    LINEAR_RANGE_INIT(0, 200000, 0x00U, 0x08U),
};

static const struct charger_axp2101_desc precharge_desc = {
    .reg = AXP2101_REG_IPRECHG_CURRENT_SETTING,
    .mask = 0x0FU,
    .bitpos = 0U,
    .ranges = precharge_ranges_ua,
    .num_ranges = ARRAY_SIZE(precharge_ranges_ua),
};

static const struct linear_range icc_ranges_ua[] = {
    LINEAR_RANGE_INIT(0, 200000, 0x00U, 0x08U),
    LINEAR_RANGE_INIT(300000, 1000000, 0x09U, 0x10U),
};

static const struct charger_axp2101_desc icc_desc = {
    .reg = AXP2101_REG_ICC_CHARGER_SETTING,
    .mask = 0x1FU,
    .bitpos = 0U,
    .ranges = icc_ranges_ua,
    .num_ranges = ARRAY_SIZE(icc_ranges_ua),
};

static const struct linear_range iterm_ranges_ua[] = {
    LINEAR_RANGE_INIT(0, 200000, 0x00U, 0x08U),
};

static const struct charger_axp2101_desc iterm_desc = {
    .reg = AXP2101_REG_ITERM_CHARGER_SETTING_AND_CONTROL,
    .mask = 0x0FU,
    .bitpos = 0U,
    .ranges = iterm_ranges_ua,
    .num_ranges = ARRAY_SIZE(iterm_ranges_ua),
};

static const struct linear_range cv_ranges_uv[] = {
    LINEAR_RANGE_INIT(4000000, 4200000, 0x01U, 0x03U),
    LINEAR_RANGE_INIT(4350000, 4400000, 0x04U, 0x05U),
};

static const struct charger_axp2101_desc cv_desc = {
    .reg = AXP2101_REG_CV_CHARGER_VOLTAGE_SETTING,
    .mask = 0x07U,
    .bitpos = 0U,
    .ranges = cv_ranges_uv,
    .num_ranges = ARRAY_SIZE(cv_ranges_uv),
};

static int
axp2101_charger_get_property(const struct device *dev, const charger_prop_t prop, union charger_propval *val)
{
    __ASSERT_NO_MSG(dev != NULL);
    __ASSERT_NO_MSG(val != NULL);
    const struct charger_axp2101_config *config = dev->config;
    uint8_t value;
    switch (prop)
    {
    case CHARGER_PROP_ONLINE:
        CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_PMU_STATUS_1, &value), config->log);
        val->online = (value & AXP2101_REG_PMU_STATUS_1_MASK_VBUS_GOOD) ? CHARGER_ONLINE_FIXED : CHARGER_ONLINE_OFFLINE;
        break;
    case CHARGER_PROP_PRESENT:
        CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_PMU_STATUS_1, &value), config->log);
        val->present = (value & AXP2101_REG_PMU_STATUS_1_MASK_BATTERY_PRESENT) ? true : false;
        break;
    case CHARGER_PROP_STATUS:
        CHECK_OK(i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_PMU_STATUS_2, &value), config->log);
        val->status = CHARGER_STATUS_NOT_CHARGING;
        if (value & AXP2101_REG_PMU_STATUS_2_MASK_BATTERY_CURRENT_CHARGE)
        {
            val->status = CHARGER_STATUS_CHARGING;
        }
        else if (value & AXP2101_REG_PMU_STATUS_2_MASK_BATTERY_CURRENT_DISCHARGE)
        {
            val->status = CHARGER_STATUS_DISCHARGING;
        }
        break;
    default:
        return -ENOTSUP;
    }

    return 0;
}

static int axp2101_charger_set_property(const struct device *dev, const charger_prop_t prop,
                                        const union charger_propval *val)
{
    return -ENOTSUP;
}

static int axp2101_charger_charge_enable(const struct device *dev, const bool enable)
{
    const struct charger_axp2101_config *config = dev->config;
    const uint8_t value = enable ? AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_MASK_CELL_CHARGER : 0U;
    return i2c_reg_update_byte_dt(&config->i2c, AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_REG,
                                  AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_MASK_CELL_CHARGER, value);
}

struct charger_driver_api axp2101_charger_api = {
    .get_property = axp2101_charger_get_property,
    .set_property = axp2101_charger_set_property,
    .charge_enable = axp2101_charger_charge_enable,
};

static int axp2101_charger_set_value(const struct device *dev, const struct charger_axp2101_desc *desc, uint32_t val)
{
    uint16_t idx;
    const struct charger_axp2101_config *config = dev->config;
    CHECK_OK(linear_range_group_get_index(desc->ranges, desc->num_ranges, val, &idx), config->log);
    if (idx > UINT8_MAX)
    {
        LOG_ERR("Invalid index");
        return -EINVAL;
    }
    uint8_t reg_val = idx << desc->bitpos;
    CHECK_OK(i2c_reg_update_byte_dt(&config->i2c, desc->reg, desc->mask, reg_val), config->log);
    return 0;
}

static int
axp2101_charger_init(const struct device *dev)
{
    const struct charger_axp2101_config *config = dev->config;

    if (!device_is_ready(config->i2c.bus))
    {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    // re-charge-voltage-microvolt is automatically set to constant-charge-voltage-max - 100mv
    // precharge-current-microamp
    CHECK_OK(axp2101_charger_set_value(dev, &precharge_desc, config->precharge_current_microamp), config->log);
    // charge-term-current-microamp
    CHECK_OK(axp2101_charger_set_value(dev, &iterm_desc, config->charge_term_current_microamp), config->log);
    // constant-charge-current-max-microamp
    CHECK_OK(axp2101_charger_set_value(dev, &icc_desc, config->constant_charge_current_max_microamp), config->log);
    // constant-charge-voltage-max-microvolt
    CHECK_OK(axp2101_charger_set_value(dev, &cv_desc, config->constant_charge_voltage_max_microvolt), config->log);

    return 0;
}

// must initialize parent device first
BUILD_ASSERT(CONFIG_AXP2101_INIT_PRIORITY < CONFIG_CHARGER_AXP2101_INIT_PRIORITY);

#define CHARGER_AXP2101_DEFINE(inst)                                                                        \
    LOG_INSTANCE_REGISTER(charger_axp2101, inst, CONFIG_CHARGER_LOG_LEVEL);                                 \
    static const struct charger_axp2101_config config##inst = {                                             \
        .i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                                       \
        .precharge_current_microamp = DT_INST_PROP(inst, precharge_current_microamp),                       \
        .charge_term_current_microamp = DT_INST_PROP(inst, charge_term_current_microamp),                   \
        .constant_charge_current_max_microamp = DT_INST_PROP(inst, constant_charge_current_max_microamp),   \
        .constant_charge_voltage_max_microvolt = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt), \
        LOG_INSTANCE_PTR_INIT(log, charger_axp2101, inst)};                                                 \
    DEVICE_DT_INST_DEFINE(inst, axp2101_charger_init, NULL, NULL, &config##inst, POST_KERNEL,               \
                          CONFIG_CHARGER_AXP2101_INIT_PRIORITY, &axp2101_charger_api);

DT_INST_FOREACH_STATUS_OKAY(CHARGER_AXP2101_DEFINE)
