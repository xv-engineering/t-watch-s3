#include <zephyr/init.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

// exposed privately by this board's CMakeLists.txt
#include <bma4xx.h>

LOG_MODULE_REGISTER(t_watch_s3, CONFIG_T_WATCH_S3_LOG_LEVEL);

int t_watch_s3_display_on(void)
{
    LOG_DBG("Initializing Display");
    // Confirm the appropriate regulator is available and ready
    // before enabling the backlight.
    const struct device *lcd_vdd = DEVICE_DT_GET(DT_NODELABEL(lcd_vdd));
    if (!device_is_ready(lcd_vdd))
    {
        return -EIO;
    }

    // Turn on the backlight
    if (IS_ENABLED(CONFIG_T_WATCH_S3_BACKLIGHT_BOOT_ON))
    {
        LOG_DBG("Turning on backlight");
        const struct pwm_dt_spec backlight = PWM_DT_SPEC_GET(DT_ALIAS(backlight));
        int ret = pwm_set_pulse_dt(&backlight, PWM_USEC(100));
        if (ret < 0)
        {
            LOG_ERR("Error setting backlight pulse: %d", ret);
            return ret;
        }
    }

    return 0;
}

// The current bma4xx driver often complains about being unable to soft-reset the imu.
// Adding a second reset if the first one fails works around this, but that involves
// a change to bma4xx.c. In the interest of keeping this board modular & out-of-tree,
// the following code manually sends two raw reset commands to the bma4xx device.
//
// The build asserts enforce this happening *after* i2c is ready, but before the
// bma4xx sensor driver is initialized.
#define BMA4XX_REG_CMD (0x7E)
#define BMA4XX_CMD_SOFT_RESET (0xB6)
BUILD_ASSERT(CONFIG_T_WATCH_S3_IMU_HACKS_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY);
BUILD_ASSERT(CONFIG_T_WATCH_S3_IMU_HACKS_PRIORITY > CONFIG_I2C_INIT_PRIORITY);

int t_watch_force_reset_imu(void)
{
    LOG_DBG("Resetting IMU");
    const struct i2c_dt_spec i2c = I2C_DT_SPEC_GET(DT_ALIAS(accel));
    if (!device_is_ready(i2c.bus))
    {
        return -EIO;
    }

    // even if this fails, it still helps
    i2c_reg_write_byte_dt(&i2c, BMA4XX_REG_CMD, BMA4XX_CMD_SOFT_RESET);
    return 0;
}

static __typeof__(((struct sensor_decoder_api *)NULL)->decode) original_decode = NULL;

#define INT12_TO_INT16(x) (((x) & 0x800) ? ((x) | 0xF000) : ((x) & 0x7FF))

static int patched_bma4xx_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec ch, uint32_t *fit,
                                         uint16_t max_count, void *data_out)
{
    __ASSERT(original_decode != NULL, "Original decoder is not set");
    LOG_DBG("Decoding with patched decoder");

    if (*fit != 0)
    {
        return 0;
    }

    if (max_count == 0 || ch.chan_idx != 0)
    {
        return -EINVAL;
    }

    struct bma4xx_encoded_data edata = *(const struct bma4xx_encoded_data *)buffer;
    switch (ch.chan_type)
    {
    case SENSOR_CHAN_ACCEL_X:
    case SENSOR_CHAN_ACCEL_Y:
    case SENSOR_CHAN_ACCEL_Z:
    case SENSOR_CHAN_ACCEL_XYZ:
        if (!edata.has_accel)
        {
            return -ENODATA;
        }

        edata.accel_xyz[0] = INT12_TO_INT16(edata.accel_xyz[0]);
        edata.accel_xyz[1] = INT12_TO_INT16(edata.accel_xyz[1]);
        edata.accel_xyz[2] = INT12_TO_INT16(edata.accel_xyz[2]);

        break;
    default:
        break;
    }

    // the original decoding method assumes that the values in the buffer
    // are 16-bit. On the bma423, they are actually 12-bit 2's complement.
    // So, if we see that we are decoding SENSOR_CHAN_ACCEL_XYZ, then we
    // need to copy and change the buffer.
    return original_decode((const uint8_t *)&edata, ch, fit, max_count, data_out);
}

// The current bma4xx driver does not correctly decode the IMU values. It makes
// an assumption that the values coming off of the device are 16 bit. On the bma423
// they are actually 12-bit, two's complement. In-tree, this can be fixed by adding
// a simple conversion from 12 to 16 bit 2's complement in `bma4xx_one_shot_decode`
// but in the interest in keeping this out-of-tree, we essentially patch in a different
// decoder that does the right thing.
//
// This is definitely undefined behavior (writing to a field of a const struct)
// but it is actually working (for now, but be weary).
int bma423_hot_patch_decoder(void)
{
    const struct device *dev = DEVICE_DT_GET(DT_ALIAS(accel));
    const struct sensor_decoder_api *original;
    if (sensor_get_decoder(dev, &original) != 0)
    {
        return -EIO;
    }
    original_decode = original->decode;

    // TECHNICALLY, THIS IS UNDEFINED BEHAVIOR: the hot-patching of the original decode
    // method means we are writing into a struct that was originally declared const.
    //
    // Practically, this works, but should be fixed legitimately by fixing the in-tree driver
    ((struct sensor_decoder_api *)original)->decode = patched_bma4xx_decoder_decode;
    return 0;
}

SYS_INIT(t_watch_force_reset_imu, POST_KERNEL, CONFIG_T_WATCH_S3_IMU_HACKS_PRIORITY);
SYS_INIT(bma423_hot_patch_decoder, PRE_KERNEL_1, 0);
SYS_INIT(t_watch_s3_display_on, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);