#include <zephyr/ztest.h>

ZTEST(hello_world, test_hello_world)
{
    // Placeholder for now
    printk("Hello, world!\n");
    ztest_test_pass();
}

ZTEST_SUITE(hello_world, NULL, NULL, NULL, NULL, NULL);
