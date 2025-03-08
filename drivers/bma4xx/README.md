# BMA423 (BMA4XX) Driver Extensions

The code contained here works around some weirdness in the in-tree bma4xx driver.


## Boot Reset

The bma4xx driver does a soft reset of the IMU on initialization.
Unfortunately, this command often fails due to a timeout. The driver
then gives up, which results in the device being marked as not ready.

A pre-emptive soft reset seems to fix this, so the "boot reset hack"
issues a soft reset to every bma4xx device before the driver does.


## Decoder Patch

The decoder method makes a false assumption about the size/type of the
acceleration data coming from the IMU. It assumes it's 16 bit 2's complement
when in reality, it's 12 bit 2's complement.

The fix is actually very simple (convert to 16 bit). **However** the
way to implement this fix without writing into the tree itself is... iffy.

Basically, we overwrite the decoder api struct (which is declared as const)
with a new implementation of the decode function, which converts the data
to 16 bit and then forwards it to the original function.

**This is technically leveraging undefined behavior (a write to const) but
practically, it works.**
