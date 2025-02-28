// /* clang-format off */
#ifndef __ZEPHYR_INCLUDE_DRIVERS_RMT_H__
#define __ZEPHYR_INCLUDE_DRIVERS_RMT_H__

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>

typedef enum
{
    RMT_TX_CARRIER_LEVEL_LOW,
    RMT_TX_CARRIER_LEVEL_HIGH,
    RMT_TX_CARRIER_LEVEL_MAX,
} rmt_tx_carrier_level_t;

typedef int (*rmt_tx_set_carrier_t)(const struct device *dev, bool carrier_en,
                                    k_timeout_t high_duration, k_timeout_t low_duration,
                                    rmt_tx_carrier_level_t carrier_level);

__subsystem struct rmt_tx_driver_api
{
    rmt_tx_set_carrier_t set_carrier;
};

__syscall int rmt_tx_set_carrier(const struct device *dev, bool carrier_en, k_timeout_t high_duration, k_timeout_t low_duration, rmt_tx_carrier_level_t carrier_level);

static inline int z_impl_rmt_tx_set_carrier(const struct device *dev, bool carrier_en, k_timeout_t high_duration, k_timeout_t low_duration, rmt_tx_carrier_level_t carrier_level)
{
    const struct rmt_tx_driver_api *api = (const struct rmt_tx_driver_api *)dev->api;
    return api->set_carrier(dev, carrier_en, high_duration, low_duration, carrier_level);
}

#include <zephyr/syscalls/rmt_tx.h>

#endif /* __ZEPHYR_INCLUDE_DRIVERS_RMT_H__ */
