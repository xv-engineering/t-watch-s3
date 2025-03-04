#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora, CONFIG_BRINGUP_LOG_LEVEL);

static const struct lora_modem_config lora_base_config = {
    .frequency = 915000000,
    .bandwidth = BW_125_KHZ,
    .datarate = SF_10,
    .coding_rate = CR_4_8,
    .preamble_len = 12,
    .iq_inverted = false,
    .tx = false,
};

ZTEST(lora, test_lora_send)
{
    const struct device *lora = DEVICE_DT_GET(DT_ALIAS(lora));
    zassert_true(device_is_ready(lora), "Lora device not ready");
    struct lora_modem_config config = lora_base_config;
    config.tx_power = 20;
    config.tx = true;

    int ret = lora_config(lora, &config);
    zassert_equal(ret, 0, "Lora config failed (%d)", ret);

    uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
    ret = lora_send(lora, data, sizeof(data));
    zassert_equal(ret, 0, "Lora send failed (%d)", ret);
}

ZTEST(lora, test_lora_receive)
{
    LOG_PRINTK("This test expects test_lora_send to be run on another device\n");
    const struct device *lora = DEVICE_DT_GET(DT_ALIAS(lora));
    zassert_true(device_is_ready(lora), "Lora device not ready");
    struct lora_modem_config config = lora_base_config;

    int ret = lora_config(lora, &config);
    zassert_equal(ret, 0, "Lora config failed (%d)", ret);

    uint8_t data[20];
    int16_t rssi;
    int8_t snr;
    LOG_PRINTK("Waiting for data...\n");
    const char *expected_data = "Hello, World!";
    ret = lora_recv(lora, data, sizeof(data), K_SECONDS(10), &rssi, &snr);
    zassert_equal(ret, strlen(expected_data), "Lora recv failed (%d)", ret);

    LOG_INF("Received data: %.*s", ret, data);
    LOG_INF("RSSI: %d, SNR: %d", rssi, snr);

    zassert_mem_equal(data, expected_data, strlen(expected_data));
}

ZTEST_SUITE(lora, NULL, NULL, NULL, NULL, NULL);