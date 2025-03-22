#include <zephyr/ztest.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

struct touch_fixture
{
    bool test_in_progress;
    struct k_sem touch_press_sem;
    struct k_sem touch_drag_sem;
    struct k_sem touch_release_sem;
};

static void touch_input_callback(struct input_event *evt, void *user_data)
{
    struct touch_fixture *f = user_data;
    if (!f->test_in_progress)
    {
        // because it's impossible to un-register the callback
        // but I don't want a ton of printing to the console on
        // any touch event when not running a test.
        return;
    }

    if (evt->dev == DEVICE_DT_GET(DT_ALIAS(touch)))
    {
        if (evt->type == INPUT_EV_ABS && (evt->code == INPUT_ABS_X || evt->code == INPUT_ABS_Y))
        {
            LOG_DBG("Touch drag event %d: %d\n", evt->code, evt->value);
            k_sem_give(&f->touch_drag_sem);
        }
        else if (evt->type == INPUT_EV_KEY && evt->code == INPUT_BTN_TOUCH)
        {
            if (evt->value)
            {
                LOG_DBG("Touch press event\n");
                k_sem_give(&f->touch_press_sem);
            }
            else
            {
                LOG_DBG("Touch release event\n");
                k_sem_give(&f->touch_release_sem);
            }
        }
    }
}

static void *touch_tests_setup(void)
{
    static struct touch_fixture fixture = {
        // We expect at least 1 touch event (actually, sematically I really
        // think it should be just 1 touch event, but driver reports an input
        // event for every position report, so the test is written with that
        // in mind)
        .touch_press_sem = Z_SEM_INITIALIZER(fixture.touch_press_sem, 0, 1),
        // doesn't necessarily need to match the expected number of drag events,
        // but larger than 1 allows a rapid amount of events to sort of "buffer"
        .touch_drag_sem = Z_SEM_INITIALIZER(fixture.touch_drag_sem, 0, 10),
        // we expect 1 release event, so set this to 2 so we can catch too many
        .touch_release_sem = Z_SEM_INITIALIZER(fixture.touch_release_sem, 0, 2),
    };

    INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(touch)), touch_input_callback, &fixture);
    return &fixture;
}

static void touch_tests_before(void *fixture)
{
    struct touch_fixture *f = fixture;
    k_sem_reset(&f->touch_press_sem);
    k_sem_reset(&f->touch_release_sem);
    f->test_in_progress = true;
}

static void touch_tests_after(void *fixture)
{
    struct touch_fixture *f = fixture;
    f->test_in_progress = false;
}

ZTEST(touch, test_touch_init)
{
    const struct device *touch = DEVICE_DT_GET(DT_ALIAS(touch));
    zassert_true(device_is_ready(touch), "Touch device is not ready");

    ztest_test_pass();
}

ZTEST_F(touch, test_touch_press)
{
    if (IS_ENABLED(CONFIG_RUNNING_UNDER_CI))
    {
        ztest_test_skip();
    }

    struct touch_fixture *f = fixture;
    int ret;
    LOG_PRINTK("Scroll your finger across the screen\n");
    // Expect at least 1 touch press event, 20 touch drag events, then only 1 touch release event.
    //
    // The reason it's "at least 1" touch press event is because the driver generates a press
    // event for every position report.
    //
    // I actually think this is incorrect behavior, but that's a discussion for another time.
    k_timeout_t timeout = K_SECONDS(3);
    const k_timepoint_t end = sys_timepoint_calc(timeout);
    size_t drag_event_count = 0;

    ret = k_sem_take(&f->touch_press_sem, timeout);
    zassert_equal(ret, 0, "Expected touch press event, got timeout");

    do
    {
        timeout = sys_timepoint_timeout(end);
        ret = k_sem_take(&f->touch_drag_sem, timeout);
        zassert_equal(ret, 0, "Expected touch drag event, got timeout");
        drag_event_count++;
    } while (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) && drag_event_count < 20);

    ret = k_sem_take(&f->touch_release_sem, timeout);
    zassert_equal(ret, 0, "Expected touch release event, got timeout");

    // make sure there's no remaining release events. We expect only 1
    zassert_equal(k_sem_count_get(&f->touch_release_sem), 0);

    zassert_equal(drag_event_count, 20, "Expected 20 touch drag events, got %d", drag_event_count);
    LOG_PRINTK("Touch released\n");
}

ZTEST_SUITE(touch, NULL, touch_tests_setup, touch_tests_before, touch_tests_after, NULL);
