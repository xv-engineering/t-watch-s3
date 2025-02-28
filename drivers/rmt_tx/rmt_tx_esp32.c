#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/rmt_tx.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rmt_tx_esp32, CONFIG_RMT_TX_LOG_LEVEL);

#define DT_DRV_COMPAT espressif_esp32_rmt_tx

struct rmt_tx_esp32_config
{
    const struct pinctrl_dev_config *pcfg;
};

static int rmt_tx_esp32_init(const struct device *dev)
{
    const struct rmt_tx_esp32_config *config = dev->config;
    int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
    if (ret < 0)
    {
        LOG_ERR("Failed to configure RMT TX pins");
        return ret;
    }
    return -0;
}

#define RMT_TX_ESP32_DEFINE(inst)                                             \
    PINCTRL_DT_DEFINE(DT_DRV_INST(inst));                                     \
    static const struct rmt_tx_esp32_config config##inst = {                  \
        .pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(inst)),                 \
    };                                                                        \
    DEVICE_DT_INST_DEFINE(inst, rmt_tx_esp32_init, NULL, NULL, &config##inst, \
                          POST_KERNEL, CONFIG_RMT_TX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(RMT_TX_ESP32_DEFINE)
