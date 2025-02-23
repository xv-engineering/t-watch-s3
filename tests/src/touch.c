#include <zephyr/ztest.h>

ZTEST(touch, test_touch)
{
    const struct device *touch = DEVICE_DT_GET(DT_ALIAS(touch));
    zassert_true(device_is_ready(touch), "Touch device is not ready");

    ztest_test_pass();
}

ZTEST_SUITE(touch, NULL, NULL, NULL, NULL, NULL);