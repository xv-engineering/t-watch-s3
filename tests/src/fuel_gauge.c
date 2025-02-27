#include <zephyr/ztest.h>
#include <zephyr/drivers/fuel_gauge.h>

ZTEST(fuel_gauge, test_fuel_gauge)
{
    const struct device *fuel = DEVICE_DT_GET(DT_NODELABEL(fuel_gauge));
    zassert_true(device_is_ready(fuel), "Fuel gauge device not ready");
}

ZTEST_SUITE(fuel_gauge, NULL, NULL, NULL, NULL, NULL);