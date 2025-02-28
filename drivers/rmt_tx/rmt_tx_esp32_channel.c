#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rmt_tx.h>
#include <zephyr/logging/log.h>
#include <hal/rmt_ll.h>

LOG_MODULE_DECLARE(rmt_tx_esp32, CONFIG_RMT_TX_LOG_LEVEL);

#define DT_DRV_COMPAT espressif_esp32_rmt_tx

struct rmt_tx_esp32_channel_config
{
    const struct device *parent;
    const struct gpio_dt_spec gpio;
    uint8_t channel;
};

static int rmt_tx_esp32_set_carrier(const struct device *dev, bool carrier_en,
                                    k_timeout_t high_duration, k_timeout_t low_duration,
                                    rmt_tx_carrier_level_t carrier_level)
{
    return 0;
}

static int rmt_tx_esp32_transmit(const struct device *dev, const struct rmt_symbol *symbols,
                                 size_t num_symbols, k_timeout_t timeout)
{
    return 0;
}

static struct rmt_tx_driver_api rmt_tx_esp32_driver_api = {
    .set_carrier = rmt_tx_esp32_set_carrier,
    .transmit = rmt_tx_esp32_transmit,
};

static int rmt_tx_esp32_channel_init(const struct device *dev)
{
    const struct rmt_tx_esp32_channel_config *config = dev->config;
    if (!device_is_ready(config->parent))
    {
        LOG_ERR("Parent device not ready");
        return -ENODEV;
    }

    // configure the gpio as an output

    return 0;
}

// parent node must be initialized first
BUILD_ASSERT(CONFIG_RMT_TX_CHANNEL_INIT_PRIORITY >= CONFIG_RMT_TX_INIT_PRIORITY);

#define RMT_TX_ESP32_CHANNEL_DEFINE(node)                                        \
    BUILD_ASSERT(DT_REG_ADDR(node) < 4);                                         \
    static const struct rmt_tx_esp32_channel_config config##node = {             \
        .parent = DEVICE_DT_GET(DT_PARENT(node)),                                \
        .gpio = GPIO_DT_SPEC_GET(node, gpios),                                   \
        .channel = DT_REG_ADDR(node),                                            \
    };                                                                           \
    DEVICE_DT_DEFINE(node, rmt_tx_esp32_channel_init, NULL, NULL, &config##node, \
                     POST_KERNEL, CONFIG_RMT_TX_CHANNEL_INIT_PRIORITY,           \
                     &rmt_tx_esp32_driver_api);

#define RMT_TX_ESP32_CHANNEL_DEFINE_ALL(inst) \
    DT_FOREACH_CHILD(DT_DRV_INST(inst), RMT_TX_ESP32_CHANNEL_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(RMT_TX_ESP32_CHANNEL_DEFINE_ALL)
