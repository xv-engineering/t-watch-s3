#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <bma4xx.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT bosch_bma4xx

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
    const struct sensor_decoder_api *original = &SENSOR_DECODER_NAME();
    original_decode = original->decode;

    // TECHNICALLY, THIS IS UNDEFINED BEHAVIOR: the hot-patching of the original decode
    // method means we are writing into a struct that was originally declared const.
    //
    // Practically, this works, but should be fixed legitimately by fixing the in-tree driver
    ((struct sensor_decoder_api *)original)->decode = patched_bma4xx_decoder_decode;
    return 0;
}

SYS_INIT(bma423_hot_patch_decoder, PRE_KERNEL_1, 0);