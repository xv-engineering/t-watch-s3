#include <zephyr/ztest.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

struct button_fixture
{
    struct k_sem external_press_sem;
    struct k_sem external_release_sem;
    struct k_sem internal_press_sem;
    struct k_sem internal_release_sem;
    struct k_sem unexpected_sem;
};

static void button_callback(struct input_event *evt, void *user_data)
{
    struct button_fixture *f = user_data;
    if (evt->dev == DEVICE_DT_GET(DT_ALIAS(buttons)))
    {
        if (evt->code == INPUT_BTN_SIDE)
        {
            if (evt->value == 1)
            {
                k_sem_give(&f->external_press_sem);
            }
            else
            {
                k_sem_give(&f->external_release_sem);
            }
        }
        else if (evt->code == INPUT_BTN_EXTRA)
        {
            if (evt->value == 1)
            {
                k_sem_give(&f->internal_press_sem);
            }
            else
            {
                k_sem_give(&f->internal_release_sem);
            }
        }
    }
}

ZTEST_F(button, test_power_button)
{
    struct button_fixture *f = fixture;
    int ret;
    LOG_PRINTK("Short-Press the external button twice\n");
    ret = k_sem_take(&f->external_press_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button pressed\n");
    ret = k_sem_take(&f->external_release_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button released\n");
    ret = k_sem_take(&f->external_press_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button pressed again\n");
    ret = k_sem_take(&f->external_release_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("External button released again\n");

    zassert_equal(k_sem_count_get(&f->unexpected_sem), 0, "Unexpected button event");
}

ZTEST_F(button, test_internal_button)
{
    struct button_fixture *f = fixture;
    int ret;
    LOG_PRINTK("Press the internal button twice\n");
    ret = k_sem_take(&f->internal_press_sem, K_SECONDS(3));
    zassert_equal(ret, 0); // a timeout indicates button wasn't pressed in time
    LOG_PRINTK("Internal button pressed\n");
    ret = k_sem_take(&f->internal_release_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("Internal button released\n");
    ret = k_sem_take(&f->internal_press_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("Internal button pressed again\n");
    ret = k_sem_take(&f->internal_release_sem, K_SECONDS(3));
    zassert_equal(ret, 0);
    LOG_PRINTK("Internal button released again\n");

    zassert_equal(k_sem_count_get(&f->unexpected_sem), 0, "Unexpected button event");
    ztest_test_pass();
}

static void *button_tests_setup(void)
{
    static struct button_fixture fixture = {
        .external_press_sem = Z_SEM_INITIALIZER(fixture.external_press_sem, 0, 2),
        .external_release_sem = Z_SEM_INITIALIZER(fixture.external_release_sem, 0, 2),
        .internal_press_sem = Z_SEM_INITIALIZER(fixture.internal_press_sem, 0, 2),
        .internal_release_sem = Z_SEM_INITIALIZER(fixture.internal_release_sem, 0, 2),
        .unexpected_sem = Z_SEM_INITIALIZER(fixture.unexpected_sem, 0, 1),
    };

    INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(buttons)), button_callback, &fixture);
    return &fixture;
}

static void button_tests_before(void *fixture)
{
    struct button_fixture *f = fixture;
    k_sem_reset(&f->external_press_sem);
    k_sem_reset(&f->external_release_sem);
    k_sem_reset(&f->internal_press_sem);
    k_sem_reset(&f->internal_release_sem);
    k_sem_reset(&f->unexpected_sem);
}

ZTEST_SUITE(button, NULL, button_tests_setup, button_tests_before, NULL, NULL);
