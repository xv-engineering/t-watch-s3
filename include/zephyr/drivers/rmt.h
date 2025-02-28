/* clang-format off */
#ifndef __ZEPHYR_INCLUDE_DRIVERS_RMT_H__
#define __ZEPHYR_INCLUDE_DRIVERS_RMT_H__

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C"
{
#endif

__subsystem struct rmt_driver_api
{
    int (*test)(const struct device *dev);
};

__syscall int rmt_test(const struct device *dev);

static inline int z_impl_rmt_test(const struct device *dev)
{
    const struct rmt_driver_api *api =
        (const struct rmt_driver_api *)dev->api;
    return api->test(dev);
}

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/rmt.h>

#endif /* __ZEPHYR_INCLUDE_DRIVERS_RMT_H__ */
