#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rmt_tx.h>
#include <zephyr/logging/log.h>
#include <hal/rmt_ll.h>
#include <soc/rmt_struct.h>

LOG_MODULE_DECLARE(rmt_tx_esp32, CONFIG_RMT_TX_LOG_LEVEL);

#define DT_DRV_COMPAT espressif_esp32_rmt_tx

struct rmt_tx_esp32_channel_config
{
    const struct device *parent;
    const struct gpio_dt_spec gpio;
    uint8_t channel;
    rmt_dev_t *rmt_dev;
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

    // Configure GPIO
    int ret = gpio_pin_configure_dt(&config->gpio, GPIO_OUTPUT);
    if (ret < 0)
    {
        LOG_ERR("Failed to configure GPIO pin %d", config->gpio.pin);
        return ret;
    }

    // Enable RMT peripheral clock
    rmt_ll_enable_periph_clock(config->rmt_dev, true);

    // Power up RMT memory block
    rmt_ll_mem_force_power_on(config->rmt_dev);

    // Reset TX channel
    rmt_ll_tx_reset_pointer(config->rmt_dev, config->channel);
    rmt_ll_tx_reset_channels_clock_div(config->rmt_dev, BIT(config->channel));

    // Configure channel clock - use APB clock by default
    rmt_ll_set_group_clock_src(config->rmt_dev, config->channel, RMT_CLK_SRC_APB, 1, 1, 1);

    // Set default clock divider (assuming 80MHz APB clock, divide by 80 to get 1MHz resolution)
    rmt_ll_tx_set_channel_clock_div(config->rmt_dev, config->channel, 80);

    // Allocate one memory block for the channel
    rmt_ll_tx_set_mem_blocks(config->rmt_dev, config->channel, 1);

    // Enable wrap mode for continuous transmission
    rmt_ll_tx_enable_wrap(config->rmt_dev, config->channel, true);

    // Set idle output level to low
    rmt_ll_tx_fix_idle_level(config->rmt_dev, config->channel, 0, true);

    // Clear and disable all interrupts for this channel
    rmt_ll_enable_interrupt(config->rmt_dev, RMT_LL_EVENT_TX_MASK(config->channel), false);
    rmt_ll_clear_interrupt_status(config->rmt_dev, RMT_LL_EVENT_TX_MASK(config->channel));

    LOG_DBG("RMT TX channel %d initialized on GPIO %d", config->channel, config->gpio.pin);
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
        .rmt_dev = (rmt_dev_t *)DT_REG_ADDR(DT_PARENT(node))};                   \
    DEVICE_DT_DEFINE(node, rmt_tx_esp32_channel_init, NULL, NULL, &config##node, \
                     POST_KERNEL, CONFIG_RMT_TX_CHANNEL_INIT_PRIORITY,           \
                     &rmt_tx_esp32_driver_api);

#define RMT_TX_ESP32_CHANNEL_DEFINE_ALL(inst) \
    DT_FOREACH_CHILD(DT_DRV_INST(inst), RMT_TX_ESP32_CHANNEL_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(RMT_TX_ESP32_CHANNEL_DEFINE_ALL)
