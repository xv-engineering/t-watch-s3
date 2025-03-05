#include <zephyr/ztest.h>
#include <zephyr/lorawan/lorawan.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

static uint8_t dev_eui[] = {0xDD, 0xEE, 0xAA, 0xDD, 0xBB, 0xEE, 0xEE, 0xFF};
static uint8_t join_eui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t app_key[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};

static struct lorawan_join_config join_cfg = {
    .dev_eui = &dev_eui[0],
    .mode = LORAWAN_ACT_OTAA,
    .otaa = {
        .app_key = &app_key[0],
        .join_eui = &join_eui[0],
        .nwk_key = &app_key[0],
        // you may need to manually increase this or reset
        // the join counter within your network console.
        .dev_nonce = 2000,
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
    // the first join always(?) fails. Unclear if this is a bug, the RP doc
    // seems to suggest that you should try again, and internally the loramac
    // node library will shift to a different channel.
    if (ret < 0)
    {
        LOG_ERR("lorawan_join_network failed, trying again: %d", ret);
        // the second join seems to (almost) always succeed
        join_cfg.otaa.dev_nonce++;
        ret = lorawan_join(&join_cfg);
        zassert_equal(ret, 0, "Lora join failed (%d)", ret);
    }

    uint8_t payload[] = "Hello";
    ret = lorawan_send(0, payload, sizeof(payload), LORAWAN_MSG_CONFIRMED);
    zassert_equal(ret, 0, "Lora send failed (%d)", ret);
}

ZTEST_SUITE(lorawan, NULL, NULL, NULL, NULL, NULL);