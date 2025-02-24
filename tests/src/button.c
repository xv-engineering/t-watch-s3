#include <zephyr/ztest.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

static uint8_t button_event_count = 0;

static void button_callback(struct input_event *evt, void *user_data)
{
    LOG_ERR("Button event: %d", evt->code);
    uint8_t *count = user_data;
    *count += 1;
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(pmic)), button_callback, &button_event_count);

ZTEST(button, test_button)
{
    const struct device *button = DEVICE_DT_GET(DT_ALIAS(button));
    zassert_true(device_is_ready(button), "Button device not ready");

    // just read the GPIO in a loop. I'm debugging
    int ret = gpio_pin_configure(DEVICE_DT_GET(DT_NODELABEL(gpio0)), 0, GPIO_INPUT | GPIO_ACTIVE_LOW);
    zassert_equal(ret, 0, "Failed to configure GPIO");
    LOG_ERR("Button test");
    k_sleep(K_SECONDS(10));
    LOG_ERR("Button test done");
}

ZTEST_SUITE(button, NULL, NULL, NULL, NULL, NULL);
