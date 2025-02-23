#include <zephyr/ztest.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bringup, CONFIG_BRINGUP_LOG_LEVEL);

// Define RTIO context with a small pool for our one-shot reads
RTIO_DEFINE_WITH_MEMPOOL(imu_rtio, 1, 1, 1, 16, sizeof(void *));
SENSOR_DT_READ_IODEV(imu_iodev, DT_ALIAS(accel),
                     {SENSOR_CHAN_ACCEL_XYZ, 0});

ZTEST(imu, test_imu)
{
    const struct device *imu = DEVICE_DT_GET(DT_ALIAS(accel));
    zassert_true(device_is_ready(imu), "IMU device is not ready");

    int res = sensor_read_async_mempool(&imu_iodev, &imu_rtio, (void *)imu);
    zassert_equal(res, 0, "Sensor read failed");

    struct rtio_cqe *cqe = rtio_cqe_consume_block(&imu_rtio);
    zassert_not_null(cqe, "No completion event");
    zassert_equal(cqe->result, 0, "Sensor read failed");

    uint8_t *buf = NULL;
    uint32_t buf_len = 0;
    res = rtio_cqe_get_mempool_buffer(&imu_rtio, cqe, &buf, &buf_len);
    zassert_equal(res, 0, "Failed to get mempool buffer");
    zassert_not_null(buf, "Buffer is null");
    zassert_not_equal(buf_len, 0, "Buffer length is 0");
    // sanity check - we don't really need the user data
    // since we only have a single device in the io queue
    struct device *sensor = cqe->userdata;
    zassert_equal(sensor, imu, "Sensor mismatch");

    // done with the completion event, this does not release the buffer
    rtio_cqe_release(&imu_rtio, cqe);

    const struct sensor_decoder_api *decoder;
    res = sensor_get_decoder(sensor, &decoder);
    zassert_equal(res, 0, "Failed to get decoder");

    struct sensor_three_axis_data accel_data;
    uint32_t fit = 0;
    res = decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ACCEL_XYZ, 0}, &fit, 1, &accel_data);
    rtio_release_buffer(&imu_rtio, buf, buf_len);
    zassert_equal(fit, 1, "Fit is not 1");
    zassert_equal(res, 1, "Decode failed");

    LOG_INF("Accel data: %" PRIsensor_three_axis_data "\n", PRIsensor_three_axis_data_arg(accel_data, 0));
    zassert_true(accel_data.readings[0].x > 0, "X is not positive");
    zassert_true(accel_data.readings[0].y > 0, "Y is not positive");
    zassert_true(accel_data.readings[0].z > 0, "Z is not positive");

    ztest_test_pass();
}

ZTEST_SUITE(imu, NULL, NULL, NULL, NULL, NULL);
