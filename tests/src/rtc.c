#include <zephyr/ztest.h>
#include <zephyr/drivers/rtc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

static void zassert_time_equal(const struct rtc_time *time1, const struct rtc_time *time2)
{
    zassert_equal(time1->tm_sec, time2->tm_sec, "Seconds do not match (expected %d, got %d)", time1->tm_sec, time2->tm_sec);
    zassert_equal(time1->tm_min, time2->tm_min, "Minutes do not match (expected %d, got %d)", time1->tm_min, time2->tm_min);
    zassert_equal(time1->tm_hour, time2->tm_hour, "Hours do not match (expected %d, got %d)", time1->tm_hour, time2->tm_hour);
    zassert_equal(time1->tm_mday, time2->tm_mday, "Days do not match (expected %d, got %d)", time1->tm_mday, time2->tm_mday);
    zassert_equal(time1->tm_mon, time2->tm_mon, "Months do not match (expected %d, got %d)", time1->tm_mon, time2->tm_mon);
    zassert_equal(time1->tm_year, time2->tm_year, "Years do not match (expected %d, got %d)", time1->tm_year, time2->tm_year);
}

ZTEST(rtc, test_rtc_init)
{
    const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
    zassert_true(device_is_ready(rtc), "RTC device not ready");

    // just make sure seconds is less than 55
    // because later we check that time is incrementing
    // for 5 seconds, and we don't want seconds to rollover
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

    // // Read back and verify initial time
    struct rtc_time time_read;
    ret = rtc_get_time(rtc, &time_read);
    zassert_equal(ret, 0, "Failed to get time (%d)", ret);
    zassert_time_equal(&time, &time_read);

    // for 5 seconds, read back the time and make sure it's
    // incrementing by 1 second per second.
    // (a failure here usually indicuates the coin-cell charging is not enabled
    // or just that the coin cell is dead)
    struct rtc_time expected_time = time;

    k_sleep(K_SECONDS(1));
    expected_time.tm_sec++;
    ret = rtc_get_time(rtc, &time_read);
    zassert_equal(ret, 0, "Failed to get time (%d)", ret);
    zassert_time_equal(&expected_time, &time_read);

    k_sleep(K_SECONDS(1));
    expected_time.tm_sec++;
    ret = rtc_get_time(rtc, &time_read);
    zassert_equal(ret, 0, "Failed to get time (%d)", ret);
    zassert_time_equal(&expected_time, &time_read);

    k_sleep(K_SECONDS(1));
    expected_time.tm_sec++;
    ret = rtc_get_time(rtc, &time_read);
    zassert_equal(ret, 0, "Failed to get time (%d)", ret);
    zassert_time_equal(&expected_time, &time_read);

    k_sleep(K_SECONDS(1));
    expected_time.tm_sec++;
    ret = rtc_get_time(rtc, &time_read);
    zassert_equal(ret, 0, "Failed to get time (%d)", ret);
    zassert_time_equal(&expected_time, &time_read);

    k_sleep(K_SECONDS(1));
    expected_time.tm_sec++;
    ret = rtc_get_time(rtc, &time_read);
    zassert_equal(ret, 0, "Failed to get time (%d)", ret);
    zassert_time_equal(&expected_time, &time_read);

    ztest_test_pass();
}

ZTEST_SUITE(rtc, NULL, NULL, NULL, NULL, NULL);