#include <zephyr/ztest.h>
#include <zephyr/drivers/rmt.h>

ZTEST(rmt, test_rmt)
{
    const struct device *rmt = DEVICE_DT_GET(DT_ALIAS(rmt));
    zassert_true(device_is_ready(rmt), "RMT device is not ready");

    int res = rmt_test(rmt);
    zassert_equal(res, 0, "RMT test failed");
}