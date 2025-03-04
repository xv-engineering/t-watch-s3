#include <zephyr/ztest.h>
#include <zephyr/lorawan/lorawan.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

static uint8_t dev_eui[] = {0xE5, 0x03, 0x67, 0x53, 0xB2, 0x07, 0xCC, 0x1B};
static uint8_t app_eui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t app_skey[] = {0x84, 0x10, 0xE5, 0xA8, 0xEF, 0xA1, 0xA2, 0x1A, 0xD0, 0xEF, 0xFF, 0xD0, 0x08, 0x5A, 0x89, 0x91};
static uint8_t nwk_skey[] = {0x68, 0x6D, 0x92, 0x21, 0x1E, 0xF0, 0x72, 0xD9, 0x5A, 0xF8, 0x4B, 0xC5, 0x30, 0x7A, 0x97, 0x25};
static const uint32_t dev_addr = 0x78000007;

static const struct lorawan_join_config join_cfg = {
    .dev_eui = &dev_eui[0],
    .mode = LORAWAN_ACT_ABP,
    .abp = {
        .app_eui = &app_eui[0],
        .app_skey = &app_skey[0],
        .nwk_skey = &nwk_skey[0],
        .dev_addr = dev_addr,
    },
};

ZTEST(lorawan, test_lorawan_init)
{
    // The device doesn't actually need to be named lora
    // under the aliases node. It is automatically used by the
    // loramac library simply by existing (via the `Radio` symbol)
    const struct device *lora = DEVICE_DT_GET(DT_ALIAS(lora));
    zassert_true(device_is_ready(lora), "Lora device not ready");

    int ret = lorawan_start();
    zassert_equal(ret, 0, "Lora start failed (%d)", ret);

    ret = lorawan_join(&join_cfg);
    zassert_equal(ret, 0, "Lora join failed (%d)", ret);

    uint8_t payload[] = "Hello, world!";
    ret = lorawan_send(0, payload, sizeof(payload), LORAWAN_MSG_CONFIRMED);
    zassert_equal(ret, 0, "Lora send failed (%d)", ret);
}

ZTEST_SUITE(lorawan, NULL, NULL, NULL, NULL, NULL);