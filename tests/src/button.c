#include <zephyr/ztest.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

struct test_data
{
    struct k_sem external_press_sem;
    struct k_sem external_release_sem;
    struct k_sem internal_press_sem;
    struct k_sem internal_release_sem;
    struct k_sem unexpected_sem;
};

static struct test_data button_events = {
    .external_press_sem = Z_SEM_INITIALIZER(button_events.external_press_sem, 0, 2),
    .external_release_sem = Z_SEM_INITIALIZER(button_events.external_release_sem, 0, 2),
    .internal_press_sem = Z_SEM_INITIALIZER(button_events.internal_press_sem, 0, 2),
    .internal_release_sem = Z_SEM_INITIALIZER(button_events.internal_release_sem, 0, 2),
    .unexpected_sem = Z_SEM_INITIALIZER(button_events.unexpected_sem, 0, 1),
};

static void button_callback(struct input_event *evt, void *user_data)
{
    struct test_data *data = user_data;
    if (evt->dev == DEVICE_DT_GET(DT_ALIAS(buttons)))
    {
        if (evt->code == INPUT_BTN_SIDE)
        {
            if (evt->value == 1)
            {
                k_sem_give(&data->external_press_sem);
            }
            else
            {
                k_sem_give(&data->external_release_sem);
            }
        }
        else if (evt->code == INPUT_BTN_EXTRA)
        {
            if (evt->value == 1)
            {
                k_sem_give(&data->internal_press_sem);
            }
            else
            {
                k_sem_give(&data->internal_release_sem);
            }
        }
    }
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(buttons)), button_callback, &button_events);
ZTEST(button, test_power_button)
{
    int ret;
    LOG_PRINTK("Short-Press the external button twice\n");
    ret = k_sem_take(&button_events.external_press_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button pressed\n");
    ret = k_sem_take(&button_events.external_release_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button released\n");
    ret = k_sem_take(&button_events.external_press_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button pressed again\n");
    ret = k_sem_take(&button_events.external_release_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button released again\n");

    zassert_equal(k_sem_count_get(&button_events.unexpected_sem), 0, "Unexpected button event");
}

ZTEST(button, test_internal_button)
{
    int ret;
    LOG_PRINTK("Press the internal button twice\n");
    ret = k_sem_take(&button_events.internal_press_sem, K_SECONDS(3));
    zassert_equal(ret, 0); // a timeout indicates button wasn't pressed in time
    LOG_PRINTK("Internal button pressed\n");
    ret = k_sem_take(&button_events.internal_release_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("Internal button released\n");
    ret = k_sem_take(&button_events.internal_press_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("Internal button pressed again\n");
    ret = k_sem_take(&button_events.internal_release_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("Internal button released again\n");

    zassert_equal(k_sem_count_get(&button_events.unexpected_sem), 0, "Unexpected button event");
    ztest_test_pass();
}

static void button_tests_before(void *)
{
    k_sem_reset(&button_events.external_press_sem);
    k_sem_reset(&button_events.external_release_sem);
    k_sem_reset(&button_events.internal_press_sem);
    k_sem_reset(&button_events.internal_release_sem);
    k_sem_reset(&button_events.unexpected_sem);
}

ZTEST_SUITE(button, NULL, NULL, button_tests_before, NULL, NULL);
