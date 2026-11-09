#ifndef NVGFX_I2C_H
#define NVGFX_I2C_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The nvgfx i2c class (nvgfx_i2cclass.c): the hidd.i2c bus driver that
    bit-bangs an nvkm i2c bus for the drm-compat i2c glue
    (drm-compat/linux_i2c.c). Both include this file, so the class the driver
    registers and the class the glue creates are always the same one.
*/

#define CLID_Hidd_I2C_NVGfx         "hidd.i2c.nvgfx"
#define IID_Hidd_I2C_NVGfx          "hidd.i2c.nvgfx"

#define HiddI2CNVGfxAttrBase        __IHidd_I2C_NVGfx

enum
{
    aoHidd_I2C_NVGfx_Adapter,       /* [I..] The i2c_adapter object */

    num_Hidd_I2C_NVGfx_Attrs
};

#define aHidd_I2C_NVGfx_Adapter     (HiddI2CNVGfxAttrBase + aoHidd_I2C_NVGfx_Adapter)

#endif /* NVGFX_I2C_H */
