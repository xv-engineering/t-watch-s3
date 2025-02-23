#include <zephyr/ztest.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/display.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_PWM), "PWM is not enabled");
BUILD_ASSERT(IS_ENABLED(CONFIG_REGULATOR), "Regulator is not enabled");

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

ZTEST(display, test_backlight)
{
    const struct device *lcd_vdd = DEVICE_DT_GET(DT_NODELABEL(lcd_vdd));
    zassert_not_null(lcd_vdd, "Failed to get lcd_vdd device");
    zassert_true(device_is_ready(lcd_vdd), "lcd_vdd device is not ready");
    zassert_true(regulator_is_enabled(lcd_vdd), "lcd_vdd is not enabled");

    const struct pwm_dt_spec backlight = PWM_DT_SPEC_GET(DT_ALIAS(backlight));

    // cycle from dimmest to brightest
    for (int i = 0; i < 100; i++)
    {
        LOG_INF("Setting backlight to %d%%", i);
        int ret = pwm_set_pulse_dt(&backlight, PWM_USEC(i));
        zassert_equal(ret, 0, "Failed to set backlight");
        k_sleep(K_MSEC(1));
    }

    for (int i = 100; i >= 0; i--)
    {
        LOG_INF("Setting backlight to %d%%", i);
        int ret = pwm_set_pulse_dt(&backlight, PWM_USEC(i));
        zassert_equal(ret, 0, "Failed to set backlight");
        k_sleep(K_MSEC(1));
    }

    // flash the display to make sure extremes work
    for (int i = 0; i < 10; i++)
    {
        LOG_INF("Flashing display");
        int ret = pwm_set_pulse_dt(&backlight, PWM_USEC(0));
        zassert_equal(ret, 0, "Failed to set backlight");
        k_sleep(K_MSEC(20));
        ret = pwm_set_pulse_dt(&backlight, PWM_USEC(100));
        zassert_equal(ret, 0, "Failed to set backlight");
        k_sleep(K_MSEC(20));
    }

    ztest_test_pass();
}

ZTEST(display, test_capabilities)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    zassert_true(device_is_ready(display));

    struct display_capabilities capabilities;
    display_get_capabilities(display, &capabilities);
    zassert_equal(capabilities.x_resolution, 240);
    zassert_equal(capabilities.y_resolution, 240);
    zassert_equal(capabilities.current_pixel_format, PIXEL_FORMAT_BGR_565);

    // The orientation reported by the driver is wrong, but it doesn't matter.
    // The orientation is directly set by the mdac settings in the devicetree.

    ztest_test_pass();
}

ZTEST(display, test_colors)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    zassert_true(device_is_ready(display), "Display not ready");

    // Turn on display
    int ret = display_blanking_off(display);
    zassert_equal(ret, 0, "Failed to turn on display");

    // full background in small chunks
    const size_t rect_w = 60;
    const size_t rect_h = 20;
    const size_t buf_size = rect_w * rect_h * 2; // 2 bytes per pixel for RGB565
    uint8_t *buf = k_malloc(buf_size);
    zassert_not_null(buf, "Failed to allocate buffer");

    struct display_buffer_descriptor buf_desc = {
        .buf_size = buf_size,
        .width = rect_w,
        .height = rect_h,
        .pitch = rect_w};

    memset(buf, 0x00, buf_size);

    // full background, black, in small chunks
    BUILD_ASSERT(240 % rect_h == 0, "240 is not divisible by rect_h");
    BUILD_ASSERT(240 % rect_w == 0, "240 is not divisible by rect_w");
    for (size_t y = 0; y < 240; y += rect_h)
    {
        for (size_t x = 0; x < 240; x += rect_w)
        {
            ret = display_write(display, x, y, &buf_desc, buf);
            zassert_equal(ret, 0, "Failed to write full background");
        }
    }

    ret = display_blanking_off(display);
    zassert_equal(ret, 0, "Failed to disable blanking");

    // Draw red rectangle in top left
    for (size_t i = 0; i < buf_size; i += 2)
    {
        buf[i] = 0xF8;     // Red: 11111000
        buf[i + 1] = 0x00; // 00000000
    }
    ret = display_write(display, 0, 0, &buf_desc, buf);
    zassert_equal(ret, 0, "Failed to write red rectangle");

    // Draw green rectangle in top right
    for (size_t i = 0; i < buf_size; i += 2)
    {
        buf[i] = 0x07;     // Green: 00000111
        buf[i + 1] = 0xE0; // 11100000
    }
    ret = display_write(display, 240 - rect_w, 0, &buf_desc, buf);
    zassert_equal(ret, 0, "Failed to write green rectangle");

    // Draw blue rectangle in bottom right
    for (size_t i = 0; i < buf_size; i += 2)
    {
        buf[i] = 0x00;     // Blue: 00000000
        buf[i + 1] = 0x1F; // 00011111
    }
    ret = display_write(display, 240 - rect_w, 240 - rect_h, &buf_desc, buf);
    zassert_equal(ret, 0, "Failed to write blue rectangle");

    // Draw white rectangle in bottom left
    for (size_t i = 0; i < buf_size; i += 2)
    {
        buf[i] = 0xFF;     // White: 11111111
        buf[i + 1] = 0xFF; // 11111111
    }
    ret = display_write(display, 0, 240 - rect_h, &buf_desc, buf);
    zassert_equal(ret, 0, "Failed to write white rectangle");

    k_free(buf);
    ztest_test_pass();
}

ZTEST_SUITE(display, NULL, NULL, NULL, NULL, NULL);
