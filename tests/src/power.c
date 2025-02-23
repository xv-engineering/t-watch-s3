#include <zephyr/ztest.h>
#include <zephyr/drivers/regulator.h>

ZTEST(power, test_power)
{
    const struct device *lcd_vdd = DEVICE_DT_GET(DT_NODELABEL(lcd_vdd));
    zassert_not_null(lcd_vdd, "Failed to get lcd_vdd device");
    zassert_true(device_is_ready(lcd_vdd), "lcd_vdd device is not ready");
    zassert_true(regulator_is_enabled(lcd_vdd), "lcd_vdd is not enabled");

    const struct device *ldo5 = DEVICE_DT_GET(DT_NODELABEL(ldo5));
    zassert_not_null(ldo5, "Failed to get ldo5 device");
    zassert_true(device_is_ready(ldo5), "ldo5 device is not ready");
    zassert_true(regulator_is_enabled(ldo5), "ldo5 is not enabled");

    ztest_test_pass();
}

ZTEST_SUITE(power, NULL, NULL, NULL, NULL, NULL);
