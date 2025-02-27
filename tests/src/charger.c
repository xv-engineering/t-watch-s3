#include <zephyr/ztest.h>
#include <zephyr/drivers/charger.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(charger, CONFIG_BRINGUP_LOG_LEVEL);

ZTEST(charger, test_charger)
{
    const struct device *dev = DEVICE_DT_GET(DT_ALIAS(charger));
    zassert_true(device_is_ready(dev), "charger device not ready");

    // the charger supports reporting of "ONLINE" (meaning plugged in)
    union charger_propval val;
    int ret = charger_get_prop(dev, CHARGER_PROP_ONLINE, &val);
    zassert_equal(ret, 0);
    zassert_equal(val.online, CHARGER_ONLINE_FIXED);

    ret = charger_get_prop(dev, CHARGER_PROP_PRESENT, &val);
    zassert_equal(ret, 0);
    LOG_PRINTK("present: %d\n", val.present);

    ret = charger_get_prop(dev, CHARGER_PROP_STATUS, &val);
    zassert_equal(ret, 0);
    LOG_PRINTK("status: %d\n", val.status);
}

ZTEST_SUITE(charger, NULL, NULL, NULL, NULL, NULL);