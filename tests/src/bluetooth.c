#include <zephyr/ztest.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

BUILD_ASSERT(IS_ENABLED(CONFIG_BT_OBSERVER));

struct bluetooth_fixture
{
    atomic_t bt_enabled;
    struct k_sem scan_sem;
};

// annoyingly, can't figure out how to retrieve the fixture
// from the scan or ready callback. Seems that it must be global.
static struct bluetooth_fixture bt_fixture = {
    // doesn't need to be as large as the total number
    // of advertisements we expect, but good to have
    // it be a bit larger so there's some buffer.
    .scan_sem = Z_SEM_INITIALIZER(bt_fixture.scan_sem, 0, 5),
    .bt_enabled = ATOMIC_INIT(0),
};

static void bt_ready_cb(int err)
{
    atomic_set(&bt_fixture.bt_enabled, (err == 0));
}

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
                    struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN] = {0};
    size_t addr_len = bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    LOG_INF("Device found: %.*s (RSSI %d), adv_type: %u", addr_len, addr_str, rssi, adv_type);

    if (ad)
    {
        LOG_INF("Advertisement data length: %u", ad->len);
    }

    k_sem_give(&bt_fixture.scan_sem);
}

// Test that initializes Bluetooth and scans for advertisements
ZTEST_F(bluetooth, test_scan)
{
    struct bluetooth_fixture *f = fixture;
    static struct bt_le_scan_param scan_params = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    // the setup should've initialized bluetooth and waited for
    // it to be ready.
    zassert_true(f->bt_enabled, "Bluetooth failed to initialize");

    // start scanning
    int ret = bt_le_scan_start(&scan_params, scan_cb);
    zassert_equal(ret, 0, "Scanning failed to start (err %d)", ret);
    LOG_INF("Scanning started successfully");

    // wait for 5 advertisements withing 5 seconds
    int count = 0;
    k_timepoint_t end = sys_timepoint_calc(K_SECONDS(5));
    k_timeout_t timeout;
    do
    {
        timeout = sys_timepoint_timeout(end);
        ret = k_sem_take(&f->scan_sem, timeout);
        zassert_equal(ret, 0, "Scan timeout reached (%d ms)", 5000);
        count++;
    } while (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) && count < 5);
    zassert_equal(count, 5);

    // stop scanning
    ret = bt_le_scan_stop();
    zassert_equal(ret, 0, "Scanning failed to stop (err %d)", ret);
    LOG_INF("Scanning stopped");
}

static void *bluetooth_tests_setup(void)
{
    // There is a bug in the esp32 bluetooth driver that asserts on enable
    // after the driver is disabled. Ideally, in the teardown, we'd do a
    // bt_disable, but we can't for now. And since we also shouldn't do
    // a double-enable, we only do an enable if the bt_fixture says that
    // we haven't been enabled yet.
    //
    // (BLE assert intc.c 222, param 00000008 ffffffed)
    if (!atomic_get(&bt_fixture.bt_enabled))
    {
        bt_enable(bt_ready_cb);
        WAIT_FOR(atomic_get(&bt_fixture.bt_enabled), 1000000, k_yield());
    }
    return &bt_fixture;
}

static void bluetooth_tests_before(void *fixture)
{
    struct bluetooth_fixture *f = fixture;
    atomic_set(&f->bt_enabled, bt_is_ready());
    k_sem_reset(&f->scan_sem);
}

ZTEST_SUITE(bluetooth, NULL, bluetooth_tests_setup, bluetooth_tests_before, NULL, NULL);
