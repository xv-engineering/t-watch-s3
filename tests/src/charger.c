#include <zephyr/ztest.h>
#include <zephyr/drivers/charger.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(charger, CONFIG_BRINGUP_LOG_LEVEL);

// For this test to pass, the battery switch inside must
// be switched to the disconnected position (or battery removed entirely)
ZTEST(charger, test_charger_disconnected)
{
    LOG_WRN("This test assumes the battery is disconnected");
    const struct device *dev = DEVICE_DT_GET(DT_ALIAS(charger));
    zassert_true(device_is_ready(dev), "charger device not ready");

    // the charger supports reporting of "ONLINE" (meaning plugged in)
    union charger_propval val;
    int ret = charger_get_prop(dev, CHARGER_PROP_ONLINE, &val);
    zassert_equal(ret, 0);
    zassert_equal(val.online, CHARGER_ONLINE_FIXED);

    ret = charger_get_prop(dev, CHARGER_PROP_PRESENT, &val);
    zassert_equal(ret, 0);
    zassert_false(val.present);

    ret = charger_get_prop(dev, CHARGER_PROP_STATUS, &val);
    zassert_equal(ret, 0);
    zassert_equal(val.status, CHARGER_STATUS_NOT_CHARGING);
}

// For this test to pass, the battery switch inside must
// be switched to the connected position (and a battery connected)
ZTEST(charger, test_charger_connected)
{
    LOG_WRN("This test assumes the battery is connected");
    const struct device *dev = DEVICE_DT_GET(DT_ALIAS(charger));
    zassert_true(device_is_ready(dev), "charger device not ready");

    union charger_propval val;
    int ret = charger_get_prop(dev, CHARGER_PROP_ONLINE, &val);
    zassert_equal(ret, 0);
    zassert_true(val.online);

    ret = charger_get_prop(dev, CHARGER_PROP_PRESENT, &val);
    zassert_equal(ret, 0);
    zassert_true(val.present);

    // it's either charging or in standby, depends how long
    // things have been plugged in, but it's definitely
    // not discharging since we're plugged in to a supply.
    ret = charger_get_prop(dev, CHARGER_PROP_STATUS, &val);
    zassert_equal(ret, 0);
    zassert_not_equal(val.status, CHARGER_STATUS_DISCHARGING);
}

ZTEST_SUITE(charger, NULL, NULL, NULL, NULL, NULL);