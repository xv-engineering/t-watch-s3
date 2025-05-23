#ifndef REG_AXP2101_H
#define REG_AXP2101_H

// Chip ID value and register
#define AXP2101_CHIP_ID 0x4A
#define AXP2101_REG_CHIP_ID 0x03U

// Register and bit to enable button battery charging (for RTC)
#define AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_REG 0x18U
#define AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_MASK_CELL_CHARGER BIT(1)
#define AXP2101_CHARGER_FUEL_GAUGE_WATCHDOG_CTRL_MASK_BUTTON_CHARGER BIT(2)

// IRQ enable registers
#define AXP2101_IRQ_ENABLE_0_REG 0x40U
#define AXP2101_IRQ_ENABLE_1_REG 0x41U
#define AXP2101_IRQ_ENABLE_1_MASK_PWRON_POSITIVE_EDGE BIT(0) // PWRON positive edge
#define AXP2101_IRQ_ENABLE_1_MASK_PWRON_NEGATIVE_EDGE BIT(1) // PWRON negative edge
#define AXP2101_IRQ_ENABLE_1_MASK_PWRON_LONG_PRESS BIT(2)    // PWRON long press
#define AXP2101_IRQ_ENABLE_1_MASK_PWRON_SHORT_PRESS BIT(3)   // PWRON short press
#define AXP2101_IRQ_ENABLE_2_REG 0x42U

// IRQ status registers
#define AXP2101_IRQ_STATUS_0_REG 0x48U
#define AXP2101_IRQ_STATUS_1_REG 0x49U
#define AXP2101_IRQ_STATUS_1_MASK_PWRON_POSITIVE_EDGE BIT(0) // PWRON positive edge
#define AXP2101_IRQ_STATUS_1_MASK_PWRON_NEGATIVE_EDGE BIT(1) // PWRON negative edge
#define AXP2101_IRQ_STATUS_1_MASK_PWRON_LONG_PRESS BIT(2)    // PWRON long press
#define AXP2101_IRQ_STATUS_1_MASK_PWRON_SHORT_PRESS BIT(3)   // PWRON short press
#define AXP2101_IRQ_STATUS_2_REG 0x4AU

// check return code, return code on error
#define CHECK_OK(ret, logger)                        \
    do                                               \
    {                                                \
        int ret_ = (ret);                            \
        if (ret_ < 0)                                \
        {                                            \
            LOG_INST_ERR(logger, "Error: %d", ret_); \
            return ret_;                             \
        }                                            \
    } while (0)

#endif // REG_AXP2101_H
