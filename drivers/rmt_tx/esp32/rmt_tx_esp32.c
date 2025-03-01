#include <esp_attr.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/rmt_tx.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <hal/rmt_ll.h>
#include <hal/rmt_hal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rmt_tx_esp32, CONFIG_RMT_TX_LOG_LEVEL);

#define DT_DRV_COMPAT espressif_esp32_rmt_tx

struct rmt_tx_esp32_config
{
    const struct pinctrl_dev_config *pcfg;
    const struct device *clock_dev;
    const clock_control_subsys_t clock_subsys;
    int irq_source;
    int irq_priority;
    int irq_flags;
};

struct rmt_tx_esp32_data
{
    rmt_hal_context_t hal;
};
// the data must be exactly castable to a rmt_hal_context_t type
// (the channel subdevice relies on this).
BUILD_ASSERT(offsetof(struct rmt_tx_esp32_data, hal) == 0);

static void IRAM_ATTR rmt_tx_esp32_isr(void *arg)
{
    const struct device *dev = (const struct device *)arg;
    __maybe_unused const struct rmt_tx_esp32_config *config = dev->config;
    struct rmt_tx_esp32_data *data = dev->data;
    rmt_hal_context_t *hal = &data->hal;
    rmt_dev_t *rmt_dev = hal->regs;

    LOG_DBG("rmt_tx_esp32_isr:  regs=%p", rmt_dev);
}

static int rmt_tx_esp32_init(const struct device *dev)
{
    const struct rmt_tx_esp32_config *config = dev->config;
    struct rmt_tx_esp32_data *data = dev->data;
    int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
    if (ret < 0)
    {
        LOG_ERR("Failed to configure RMT TX pins");
        return ret;
    }

    if (!device_is_ready(config->clock_dev))
    {
        LOG_ERR("clock control device not ready");
        return -ENODEV;
    }

    ret = clock_control_on(config->clock_dev, config->clock_subsys);
    if (ret < 0)
    {
        LOG_ERR("Failed to enable RMT clock");
        return ret;
    }

    const int flags = ESP_PRIO_TO_FLAGS(config->irq_priority) |
                      ESP_INT_FLAGS_CHECK(config->irq_flags) | ESP_INTR_FLAG_IRAM;
    ret = esp_intr_alloc(config->irq_source,
                         flags,
                         rmt_tx_esp32_isr,
                         (void *)dev,
                         NULL);
    if (ret < 0)
    {
        LOG_ERR("Failed to allocate RMT TX interrupt");
        return ret;
    }

    // this function internally hardcodes the RMT peripheral address.
    // That's fine, but just for ultimate correctness the RMT address
    // is compared to the address from the devicetree too.
    void *original_address = data->hal.regs;
    rmt_hal_init(&data->hal);
    if (original_address != data->hal.regs)
    {
        LOG_ERR("RMT peripheral address mismatch");
        return -EINVAL;
    }

    // TODO: continue implementation here.

    return 0;
}

#define RMT_TX_ESP32_DEFINE(inst)                                                          \
    PINCTRL_DT_DEFINE(DT_DRV_INST(inst));                                                  \
    static struct rmt_tx_esp32_data data##inst = {                                         \
        .hal = {.regs = (rmt_soc_handle_t)DT_REG_ADDR(DT_DRV_INST(inst))},                 \
    };                                                                                     \
    static const struct rmt_tx_esp32_config config##inst = {                               \
        .pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(inst)),                              \
        .clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_DRV_INST(inst))),                     \
        .clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(DT_DRV_INST(inst), offset), \
        .irq_source = DT_IRQ_BY_IDX(DT_DRV_INST(inst), 0, irq),                            \
        .irq_priority = DT_IRQ_BY_IDX(DT_DRV_INST(inst), 0, priority),                     \
        .irq_flags = DT_IRQ_BY_IDX(DT_DRV_INST(inst), 0, flags)};                          \
    DEVICE_DT_INST_DEFINE(inst, rmt_tx_esp32_init, NULL, &data##inst, &config##inst,       \
                          POST_KERNEL, CONFIG_RMT_TX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(RMT_TX_ESP32_DEFINE)
