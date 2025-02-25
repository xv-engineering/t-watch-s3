#include <zephyr/ztest.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

struct test_data
{
    struct k_sem external_short_sem;
    struct k_sem external_long_sem;
    struct k_sem internal_sem;
    struct k_sem unexpected_sem;
};

static struct test_data button_events = {
    .external_short_sem = Z_SEM_INITIALIZER(button_events.external_short_sem, 0, 2),
    .external_long_sem = Z_SEM_INITIALIZER(button_events.external_long_sem, 0, 2),
    .internal_sem = Z_SEM_INITIALIZER(button_events.internal_sem, 0, 2),
    .unexpected_sem = Z_SEM_INITIALIZER(button_events.unexpected_sem, 0, 1),
};

static void button_callback(struct input_event *evt, void *user_data)
{
    struct test_data *data = user_data;
    if (evt->dev == DEVICE_DT_GET(DT_ALIAS(button0)))
    {
        if (evt->code == INPUT_KEY_MENU && evt->value == 0)
        {
            k_sem_give(&data->external_short_sem);
        }
        else if (evt->code == INPUT_KEY_POWER && evt->value == 0)
        {
            k_sem_give(&data->external_long_sem);
        }
    }
    else if (evt->dev == DEVICE_DT_GET(DT_ALIAS(button1)))
    {
        if (evt->code == INPUT_BTN_0 && evt->value == 0)
        {
            k_sem_give(&data->internal_sem);
        }
    }
    else
    {
        k_sem_give(&data->unexpected_sem);
    }
}

INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(DT_ALIAS(button0)), button_callback, &button_events, external);
INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(DT_ALIAS(button1)), button_callback, &button_events, internal);

ZTEST(button, test_power_button_short)
{
    const struct device *external_button = DEVICE_DT_GET(DT_ALIAS(button0));
    zassert_true(device_is_ready(external_button), "Button device not ready");
    int ret;

    LOG_PRINTK("Short-Press the external button twice\n");
    ret = k_sem_take(&button_events.external_short_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button pressed\n");
    ret = k_sem_take(&button_events.external_short_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button pressed again\n");

    zassert_equal(k_sem_count_get(&button_events.unexpected_sem), 0, "Unexpected button event");
}

ZTEST(button, test_power_button_long)
{
    const struct device *external_button = DEVICE_DT_GET(DT_ALIAS(button0));
    zassert_true(device_is_ready(external_button), "Button device not ready");
    int ret;

    LOG_PRINTK("Long-Press the external button twice\n");
    ret = k_sem_take(&button_events.external_long_sem, K_SECONDS(6));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button pressed\n");
    ret = k_sem_take(&button_events.external_long_sem, K_SECONDS(6));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button pressed again\n");
}

ZTEST(button, test_internal_button)
{
    const struct device *internal_button = DEVICE_DT_GET(DT_ALIAS(button1));
    zassert_true(device_is_ready(internal_button), "Button device not ready");
    int ret;

    // just read the GPIO in a loop. I'm debugging
    LOG_PRINTK("Press the internal button twice\n");
    ret = k_sem_take(&button_events.internal_sem, K_SECONDS(3));
    zassert_equal(ret, 0); // a timeout indicates button wasn't pressed in time
    LOG_PRINTK("Internal button pressed\n");
    ret = k_sem_take(&button_events.internal_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("Internal button pressed again\n");

    zassert_equal(k_sem_count_get(&button_events.unexpected_sem), 0, "Unexpected button event");
    ztest_test_pass();
}

static void button_tests_before(void *)
{
    k_sem_reset(&button_events.external_short_sem);
    k_sem_reset(&button_events.external_long_sem);
    k_sem_reset(&button_events.internal_sem);
    k_sem_reset(&button_events.unexpected_sem);
}

ZTEST_SUITE(button, NULL, NULL, button_tests_before, NULL, NULL);
