#include <zephyr/init.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

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

SYS_INIT(t_watch_s3_display_on, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);