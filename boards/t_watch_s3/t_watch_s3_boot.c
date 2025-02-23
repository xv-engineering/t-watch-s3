#include <zephyr/init.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(t_watch_s3, CONFIG_T_WATCH_S3_LOG_LEVEL);

int t_watch_s3_display_on(void)
{
    LOG_DBG("Initializing Display");
    // Confirm the appropriate regulator is available and ready
    // before enabling the backlight.
    const struct device *lcd_vdd = DEVICE_DT_GET(DT_NODELABEL(lcd_vdd));
    if (!device_is_ready(lcd_vdd))
    {
        return -EIO;
    }

    // Turn on the backlight
    if (IS_ENABLED(CONFIG_T_WATCH_S3_BACKLIGHT_BOOT_ON))
    {
        LOG_DBG("Turning on backlight");
        const struct pwm_dt_spec backlight = PWM_DT_SPEC_GET(DT_ALIAS(backlight));
        int ret = pwm_set_pulse_dt(&backlight, PWM_USEC(100));
        if (ret < 0)
        {
            LOG_ERR("Error setting backlight pulse: %d", ret);
            return ret;
        }
    }

    return 0;
}

// The current bma4xx driver often complains about being unable to soft-reset the imu.
// Adding a second reset if the first one fails works around this, but that involves
// a change to bma4xx.c. In the interest of keeping this board modular & out-of-tree,
// the following code manually sends two raw reset commands to the bma4xx device.
//
// The build asserts enforce this happening *after* i2c is ready, but before the
// bma4xx sensor driver is initialized.
#define BMA4XX_REG_CMD (0x7E)
#define BMA4XX_CMD_SOFT_RESET (0xB6)
BUILD_ASSERT(CONFIG_T_WATCH_S3_RESET_IMU_HACK_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY);
BUILD_ASSERT(CONFIG_T_WATCH_S3_RESET_IMU_HACK_PRIORITY > CONFIG_I2C_INIT_PRIORITY);

int t_watch_force_reset_imu(void)
{
    LOG_DBG("Resetting IMU");
    const struct i2c_dt_spec i2c = I2C_DT_SPEC_GET(DT_ALIAS(accel));
    if (!device_is_ready(i2c.bus))
    {
        return -EIO;
    }

    // even if this fails, it still helps
    i2c_reg_write_byte_dt(&i2c, BMA4XX_REG_CMD, BMA4XX_CMD_SOFT_RESET);
    return 0;
}

SYS_INIT(t_watch_force_reset_imu, POST_KERNEL, CONFIG_T_WATCH_S3_RESET_IMU_HACK_PRIORITY);
SYS_INIT(t_watch_s3_display_on, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);