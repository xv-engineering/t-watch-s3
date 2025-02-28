#include <zephyr/ztest.h>
#include <zephyr/drivers/rmt_tx.h>

ZTEST(rmt, test_rmt_tx)
{
    const struct device *rmt = DEVICE_DT_GET(DT_ALIAS(rmt_tx));
    zassert_true(device_is_ready(rmt), "RMT device is not ready");

    int res = rmt_tx_set_carrier(rmt, true, K_MSEC(100), K_MSEC(100), RMT_TX_CARRIER_LEVEL_HIGH);
    zassert_equal(res, 0, "RMT test failed");
}

ZTEST_SUITE(rmt, NULL, NULL, NULL, NULL, NULL);