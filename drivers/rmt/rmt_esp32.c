#include <zephyr/drivers/rmt.h>

#define DT_DRV_COMPAT espressif_esp32_rmt

struct rmt_esp32_config
{
    // intentionally empty
};

struct rmt_esp32_data
{
    // intentionally empty
};

static int rmt_esp32_test(const struct device *dev)
{
    return 0;
}

static struct rmt_driver_api rmt_esp32_driver_api = {
    .test = rmt_esp32_test,
};

static int rmt_esp32_init(const struct device *dev)
{
    return 0;
}

#define RMT_ESP32_DEFINE(inst)                                                    \
    static struct rmt_esp32_data data##inst = {};                                 \
    static const struct rmt_esp32_config config##inst = {};                       \
    DEVICE_DT_INST_DEFINE(inst, rmt_esp32_init, NULL, &data##inst, &config##inst, \
                          POST_KERNEL, CONFIG_RMT_INIT_PRIORITY,                  \
                          &rmt_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RMT_ESP32_DEFINE)
