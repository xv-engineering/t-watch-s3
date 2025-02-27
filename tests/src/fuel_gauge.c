#include <zephyr/ztest.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

ZTEST(fuel_gauge, test_fuel_gauge)
{
    LOG_PRINTK("This test assumes the battery is connected and not completely dead\n");
    const struct device *fuel = DEVICE_DT_GET(DT_NODELABEL(fuel_gauge));
    zassert_true(device_is_ready(fuel), "Fuel gauge device not ready");

    union fuel_gauge_prop_val val;
    int ret = fuel_gauge_get_prop(fuel, FUEL_GAUGE_VOLTAGE, &val);
    zassert_equal(ret, 0, "Failed to get voltage");
    LOG_INF("voltage: %d.%03d volts", val.voltage / 1000000, (val.voltage % 1000000) / 1000);
    zassert_between_inclusive(val.voltage, 3000000, 4200000);

    ret = fuel_gauge_get_prop(fuel, FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE, &val);
    zassert_equal(ret, 0, "Failed to get absolute state of charge");
    LOG_INF("absolute state of charge: %d%%", val.absolute_state_of_charge);
    zassert_between_inclusive(val.absolute_state_of_charge, 0, 100);
}

ZTEST_SUITE(fuel_gauge, NULL, NULL, NULL, NULL, NULL);