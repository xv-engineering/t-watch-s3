#include <zephyr/ztest.h>
#include <zephyr/drivers/rtc.h>

ZTEST(rtc, test_rtc_init)
{
    const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    zassert_true(device_is_ready(rtc), "RTC device not ready");

    const struct rtc_time time = {
        .tm_sec = 20,
        .tm_min = 30,
        .tm_hour = 13,
        .tm_mday = 20,
        .tm_mon = 4,
        .tm_year = 25,
    };
    int ret = rtc_set_time(rtc, &time);
    zassert_equal(ret, 0, "Failed to set time (%d)", ret);

    // Read back and verify initial time
    struct rtc_time time_read;
    ret = rtc_get_time(rtc, &time_read);
    zassert_equal(ret, 0, "Failed to get time (%d)", ret);
    zassert_equal(time_read.tm_sec, time.tm_sec, "Seconds do not match (expected %d, got %d)", time.tm_sec, time_read.tm_sec);
    zassert_equal(time_read.tm_min, time.tm_min, "Minutes do not match (expected %d, got %d)", time.tm_min, time_read.tm_min);
    zassert_equal(time_read.tm_hour, time.tm_hour, "Hours do not match (expected %d, got %d)", time.tm_hour, time_read.tm_hour);
    zassert_equal(time_read.tm_mday, time.tm_mday, "Days do not match (expected %d, got %d)", time.tm_mday, time_read.tm_mday);
    zassert_equal(time_read.tm_mon, time.tm_mon, "Months do not match (expected %d, got %d)", time.tm_mon, time_read.tm_mon);
    zassert_equal(time_read.tm_year, time.tm_year, "Years do not match (expected %d, got %d)", time.tm_year, time_read.tm_year);

    while (true)
    {
        // read the time over and over every second and print it out
        ret = rtc_get_time(rtc, &time_read);
        zassert_equal(ret, 0, "Failed to get time (%d)", ret);
        printk("Time: %02d:%02d:%02d %02d/%02d/%d\n", time_read.tm_hour, time_read.tm_min, time_read.tm_sec, time_read.tm_mon, time_read.tm_mday, time_read.tm_year);
        k_sleep(K_SECONDS(1));
    }
}

ZTEST_SUITE(rtc, NULL, NULL, NULL, NULL, NULL);