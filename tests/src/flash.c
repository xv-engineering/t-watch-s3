#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

// in any normal app, you'd probably want to use one of the many
// available filesystems (tinyfs, ZMS, etc.) to actually deal
// with the flash memory.
ZTEST(flash, test_flash_rw)
{
    const struct device *flash = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
    zassert_true(device_is_ready(flash), "flash device not ready");

    const off_t address = FIXED_PARTITION_OFFSET(storage_partition);
    const char data[] = "Hello, World!";

    const struct flash_parameters *parameters = flash_get_parameters(flash);
    zassert_not_null(parameters, "flash parameters not found");
    // annoyingly, the flash parameters does not contain the erase block
    // size. We have to get it from the flash node instead.
    const size_t erase_size = DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size);

    int ret = flash_erase(flash, address, erase_size);
    zassert_equal(ret, 0);

    ret = flash_write(flash, address, data, sizeof(data));
    zassert_equal(ret, 0);

    // read it back
    char buffer[sizeof(data)];
    ret = flash_read(flash, address, buffer, sizeof(data));
    zassert_equal(ret, 0);
    zassert_mem_equal(buffer, data, sizeof(data));

    // erase it and make sure the read now returns empty data
    ret = flash_erase(flash, address, erase_size);
    zassert_equal(ret, 0);
    ret = flash_read(flash, address, buffer, sizeof(data));
    zassert_equal(ret, 0);

    char expected[sizeof(data)];
    memset(expected, parameters->erase_value, sizeof(expected));
    zassert_mem_equal(buffer, expected, sizeof(data));
}

ZTEST(flash, test_flash_fill)
{
    const struct device *flash = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
    zassert_true(device_is_ready(flash), "flash device not ready");

    const struct flash_parameters *parameters = flash_get_parameters(flash);
    zassert_not_null(parameters, "flash parameters not found");

    const off_t address = FIXED_PARTITION_OFFSET(storage_partition);
    const size_t size = FIXED_PARTITION_SIZE(storage_partition);

    int ret = flash_erase(flash, address, size);
    zassert_equal(ret, 0);

    ret = flash_fill(flash, 0xA5, address, size);
    zassert_equal(ret, 0);

    char buffer[32];
    char expected[sizeof(buffer)];
    memset(expected, 0xA5, sizeof(expected));
    BUILD_ASSERT(size % sizeof(buffer) == 0, "size must be a multiple of buffer size");
    for (off_t pos = address; pos < address + size; pos += sizeof(buffer))
    {
        ret = flash_read(flash, pos, buffer, sizeof(buffer));
        zassert_equal(ret, 0);
        zassert_mem_equal(buffer, expected, sizeof(buffer));
    }

    ret = flash_erase(flash, address, size);
    zassert_equal(ret, 0);
    memset(expected, parameters->erase_value, sizeof(expected));

    for (off_t pos = address; pos < address + size; pos += sizeof(buffer))
    {
        ret = flash_read(flash, pos, buffer, sizeof(buffer));
        zassert_equal(ret, 0);
        zassert_mem_equal(buffer, expected, sizeof(buffer));
    }
}

ZTEST_SUITE(flash, NULL, NULL, NULL, NULL, NULL);