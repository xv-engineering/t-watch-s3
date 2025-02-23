#include <zephyr/ztest.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/haptics/drv2605.h>

ZTEST(haptics, test_haptics)
{
    // LDO5 (Really BLDO2 - bad name from datasheet) must be enabled
    const struct device *ldo5 = DEVICE_DT_GET(DT_NODELABEL(ldo5));
    zassert_true(device_is_ready(ldo5), "LDO5 device is not ready");
    zassert_true(regulator_is_enabled(ldo5), "LDO5 is not enabled");

    const struct device *haptic = DEVICE_DT_GET(DT_ALIAS(haptic));
    zassert_true(device_is_ready(haptic), "Haptic device is not ready");

    // Play 4 strong clicks, each separated by
    // a 330 millisecond pause.
    static struct drv2605_rom_data rom_data = {
        .library = DRV2605_LIBRARY_TS2200_D,
        .brake_time = 0,
        .overdrive_time = 0,
        .sustain_neg_time = 0,
        .sustain_pos_time = 0,
        .trigger = DRV2605_MODE_INTERNAL_TRIGGER,
        .seq_regs[0] = 4,
        .seq_regs[1] = 33 | 0x80,
        .seq_regs[2] = 4,
        .seq_regs[3] = 33 | 0x80,
        .seq_regs[4] = 4,
        .seq_regs[5] = 33 | 0x80,
        .seq_regs[6] = 4,
    };

    union drv2605_config_data config = {
        .rom_data = &rom_data,
    };

    // Configure the haptic device
    int ret = drv2605_haptic_config(haptic, DRV2605_HAPTICS_SOURCE_ROM, &config);
    zassert_true(ret == 0, "Failed to configure haptic device");

    // Start haptic output
    ret = haptics_start_output(haptic);
    zassert_true(ret == 0, "Failed to start haptic output");

    // Wait for effect to complete (rough estimate)
    k_sleep(K_MSEC(1200));

    // Stop haptic output
    ret = haptics_stop_output(haptic);
    zassert_true(ret == 0, "Failed to stop haptic output");

    ztest_test_pass();
}

ZTEST_SUITE(haptics, NULL, NULL, NULL, NULL, NULL);
