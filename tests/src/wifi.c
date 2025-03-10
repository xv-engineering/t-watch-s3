#include <zephyr/ztest.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

struct wifi_fixture
{
    struct net_mgmt_event_callback wifi_mgmt_cb;
    size_t scan_result_count;
};

static void
wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
    struct wifi_fixture *f = CONTAINER_OF(cb, struct wifi_fixture, wifi_mgmt_cb);
    if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT)
    {
        f->scan_result_count++;
    }
}

static void *wifi_tests_setup(void)
{
    static struct wifi_fixture fixture = {
        .scan_result_count = 0,
    };

    net_mgmt_init_event_callback(&fixture.wifi_mgmt_cb, wifi_mgmt_event_handler, NET_EVENT_WIFI_SCAN_RESULT);
    net_mgmt_add_event_callback(&fixture.wifi_mgmt_cb);
    return &fixture;
}

static void wifi_tests_before(void *fixture)
{
    struct wifi_fixture *f = fixture;
    f->scan_result_count = 0;
}

static void wifi_tests_teardown(void *fixture)
{
    struct wifi_fixture *f = fixture;
    net_mgmt_del_event_callback(&f->wifi_mgmt_cb);
}

ZTEST_F(wifi, test_wifi_scan)
{
    const struct device *wifi = DEVICE_DT_GET(DT_ALIAS(wifi));
    zassert_true(device_is_ready(wifi), "WiFi device not ready");

    struct net_if *iface = net_if_lookup_by_dev(wifi);
    zassert_not_null(iface, "WiFi interface not found");
    zassert_true(net_if_is_wifi(iface), "iface unexpectedely not a WiFi interface");

    zassert_true(fixture->scan_result_count == 0, "Scan result count is not 0");

    int ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
    zassert_equal(ret, 0, "Failed to request WiFi scan");

    ret = net_mgmt_event_wait(NET_EVENT_WIFI_SCAN_DONE, NULL, NULL, NULL, NULL, K_SECONDS(10));
    zassert_equal(ret, 0, "WiFi scan did not complete");

    zassert_true(fixture->scan_result_count > 0, "No scan results received");
}

ZTEST_SUITE(wifi, NULL, wifi_tests_setup, wifi_tests_before, NULL, wifi_tests_teardown);