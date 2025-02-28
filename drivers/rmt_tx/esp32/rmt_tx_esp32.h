#ifndef __RMT_TX_ESP32_H__
#define __RMT_TX_ESP32_H__

#include <zephyr/drivers/rmt_tx.h>

int periph_rmt_tx_esp32_set_carrier(const struct device *dev, uint8_t channel, bool carrier_en,
                                    k_timeout_t high_duration, k_timeout_t low_duration,
                                    rmt_tx_carrier_level_t carrier_level);

int periph_rmt_tx_esp32_transmit(const struct device *dev, uint8_t channel, const struct rmt_symbol *symbols,
                                 size_t num_symbols, k_timeout_t timeout);

#endif
