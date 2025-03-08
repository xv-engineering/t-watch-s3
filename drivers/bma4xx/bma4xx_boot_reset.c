#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <bma4xx.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT bosch_bma4xx

#define CREATE_I2C_ARRAY_ENTRY(inst) \
    I2C_DT_SPEC_GET(DT_DRV_INST(inst))

// The current bma4xx driver often complains about being unable to soft-reset the imu.
// Adding a second reset if the first one fails works around this, but that involves
// a change to bma4xx.c. In the interest of keeping this board modular & out-of-tree,
// the following code manually sends two raw reset commands to the bma4xx device.
//
// The build asserts enforce this happening *after* i2c is ready, but before the
// bma4xx sensor driver is initialized.
BUILD_ASSERT(CONFIG_BMA4XX_BOOT_RESET_HACK_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY);
BUILD_ASSERT(CONFIG_BMA4XX_BOOT_RESET_HACK_PRIORITY > CONFIG_I2C_INIT_PRIORITY);

int bma4xx_boot_reset(void)
{
    const struct i2c_dt_spec to_reset[] = {
        DT_INST_FOREACH_STATUS_OKAY(CREATE_I2C_ARRAY_ENTRY)};

    for (size_t i = 0; i < ARRAY_SIZE(to_reset); i++)
    {
        if (!device_is_ready(to_reset[i].bus))
        {
            LOG_ERR("Bus %s is not ready", to_reset[i].bus->name);
            return -EIO;
        }

        // even if this fails, it still helps
        i2c_reg_write_byte_dt(&to_reset[i], BMA4XX_REG_CMD, BMA4XX_CMD_SOFT_RESET);
    }

    return 0;
}

SYS_INIT(bma4xx_boot_reset, POST_KERNEL, CONFIG_BMA4XX_BOOT_RESET_HACK_PRIORITY);