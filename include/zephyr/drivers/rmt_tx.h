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

typedef enum
{
    RMT_TX_SYMBOL_LEVEL_LOW,
    RMT_TX_SYMBOL_LEVEL_HIGH,
    RMT_TX_SYMBOL_LEVEL_MAX,
} rmt_tx_symbol_level_t;

/**
 * @brief RMT symbol structure representing one item to be transmitted
 */
struct rmt_symbol
{
    k_timeout_t duration;
    rmt_tx_symbol_level_t level;
};

typedef int (*rmt_tx_set_carrier_t)(const struct device *dev, bool carrier_en,
                                    k_timeout_t high_duration, k_timeout_t low_duration,
                                    rmt_tx_carrier_level_t carrier_level);
typedef int (*rmt_tx_transmit_t)(const struct device *dev, const struct rmt_symbol *symbols,
                                 size_t num_symbols, k_timeout_t timeout);

__subsystem struct rmt_tx_driver_api
{
    rmt_tx_set_carrier_t set_carrier;
    rmt_tx_transmit_t transmit;
};

__syscall int rmt_tx_set_carrier(const struct device *dev, bool carrier_en, k_timeout_t high_duration, k_timeout_t low_duration, rmt_tx_carrier_level_t carrier_level);
__syscall int rmt_tx_transmit(const struct device *dev, const struct rmt_symbol *symbols, size_t num_symbols, k_timeout_t timeout);

static inline int z_impl_rmt_tx_set_carrier(const struct device *dev, bool carrier_en, k_timeout_t high_duration, k_timeout_t low_duration, rmt_tx_carrier_level_t carrier_level)
{
    const struct rmt_tx_driver_api *api = (const struct rmt_tx_driver_api *)dev->api;
    return api->set_carrier(dev, carrier_en, high_duration, low_duration, carrier_level);
}

static inline int z_impl_rmt_tx_transmit(const struct device *dev, const struct rmt_symbol *symbols, size_t num_symbols, k_timeout_t timeout)
{
    const struct rmt_tx_driver_api *api = (const struct rmt_tx_driver_api *)dev->api;
    return api->transmit(dev, symbols, num_symbols, timeout);
}

#include <zephyr/syscalls/rmt_tx.h>

#endif /* __ZEPHYR_INCLUDE_DRIVERS_RMT_H__ */
