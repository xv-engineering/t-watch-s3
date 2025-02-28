#include <zephyr/drivers/rmt_tx.h>

#define DT_DRV_COMPAT espressif_esp32_rmt_tx

struct rmt_tx_esp32_config
{
    // intentionally empty
};

struct rmt_tx_esp32_data
{
    // intentionally empty
};

static int rmt_tx_esp32_set_carrier(const struct device *dev, bool carrier_en,
                                    k_timeout_t high_duration, k_timeout_t low_duration,
                                    rmt_tx_carrier_level_t carrier_level)
{
    return 0;
}

static struct rmt_tx_driver_api rmt_tx_esp32_driver_api = {
    .set_carrier = rmt_tx_esp32_set_carrier,
};

static int rmt_tx_esp32_init(const struct device *dev)
{
    return 0;
}

#define RMT_TX_ESP32_DEFINE(inst)                                                    \
    static struct rmt_tx_esp32_data data##inst = {};                                 \
    static const struct rmt_tx_esp32_config config##inst = {};                       \
    DEVICE_DT_INST_DEFINE(inst, rmt_tx_esp32_init, NULL, &data##inst, &config##inst, \
                          POST_KERNEL, CONFIG_RMT_TX_INIT_PRIORITY,                  \
                          &rmt_tx_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RMT_TX_ESP32_DEFINE)
