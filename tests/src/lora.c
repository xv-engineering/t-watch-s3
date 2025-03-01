#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>

ZTEST(lora, test_lora_init)
{
    const struct device *lora = DEVICE_DT_GET(DT_ALIAS(lora));
    zassert_true(device_is_ready(lora), "Lora device not ready");

    struct lora_modem_config config = {
        .frequency = 915000000,
        .bandwidth = BW_125_KHZ,
        .datarate = SF_7,
        .coding_rate = CR_4_5,
        .preamble_len = 12,
        .tx_power = 14,
        .tx = true,
        .iq_inverted = false,
        .public_network = false,
    };

    int ret = lora_config(lora, &config);
    zassert_true(ret == 0, "Lora config failed (%d)", ret);

    uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
    ret = lora_send(lora, data, sizeof(data));
    zassert_true(ret == 0, "Lora send failed (%d)", ret);
}

ZTEST_SUITE(lora, NULL, NULL, NULL, NULL, NULL);