/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <drm-compat/drm_compat_i2c.h>
#include <drm-compat/drm_compat_funcs.h>

/* I2C handling */
#include <hidd/i2c.h>

OOP_AttrBase HiddI2CDeviceAttrBase = 0; /* TODO: Implement  freeing */

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    /* FIXME: This function is not generic. It has hardcoded cases that are present in nouveau */
    if (adap->i2cdriver == (IPTR)0)
    {
        bug("ERROR: i2c_transfer called without driver present\n");
NOT_IMPLEMENTED_STOP
        return 0;
    }
    
    if (HiddI2CDeviceAttrBase == 0)
        HiddI2CDeviceAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2CDevice);

    if (HiddI2CDeviceAttrBase == 0)
    {
        bug("ERROR: i2c_trasfer not able to obtain HiddI2CDeviceAttrBase\n");
        return 0;
    }

    /* Go through supported cases */
    if ((num == 2) && (msgs[0].addr == 0x50) && (msgs[1].addr == 0x50) && (msgs[0].len == 1) && (msgs[1].len == 1))
    {
        /* This is probing for DDC */
        D(bug("i2c_transfer - probing for DDC\n"));
NOT_IMPLEMENTED_STOP
#if 0
        if (HIDD_I2C_ProbeAddress((OOP_Object *)adap->i2cdriver, 0xa0)) /* AROS has shifted addresses (<< 1) */
            return 2;
        else
            return 0;
#endif
    }
    else if ((num == 2) && (msgs[0].addr == 0x75) && (msgs[1].addr == 0x75) && (msgs[0].len == 1) && (msgs[1].len == 1))
    {
NOT_IMPLEMENTED_STOP
#if 0
        /* This is probing for some hardware related to TV output on nv04 */
        D(bug("i2c_transfer - probing for some hardware related to TV output on nv04\n"));
        if (HIDD_I2C_ProbeAddress((OOP_Object *)adap->i2cdriver, 0xea)) /* AROS has shifted addresses (<< 1) */
            return 2;
        else
            return 0;
#endif
    }
    else if ((num == 2) && (msgs[0].addr == 0x50) && (msgs[1].addr == 0x50) && (msgs[0].len == 1) && (msgs[1].len != 1))
    {
        /* This is reading data from DDC */
        struct pHidd_I2CDevice_WriteRead msg;
        BOOL result = FALSE;
        
        struct TagItem attrs[] = 
        {
            { aHidd_I2CDevice_Driver,   (IPTR)adap->i2cdriver   },
            { aHidd_I2CDevice_Address,  0xa0                    },
            { aHidd_I2CDevice_Name,     (IPTR)"Read DDC"        },
            { TAG_DONE, 0UL }
        };

        (bug("i2c_transfer - reading from DDC\n"));

        OOP_Object *obj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);
        
        msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteRead);
        msg.readBuffer = msgs[1].buf;
        msg.readLength = msgs[1].len;
        msg.writeBuffer = msgs[0].buf;
        msg.writeLength = msgs[0].len;

        result = OOP_DoMethod(obj, &msg.mID);
        
        OOP_DisposeObject(obj);
        
        if (result)
            return 2;
        else
            return 0;
    }
    else if ((num == 2) && (msgs[0].addr == 0x54) && (msgs[1].addr == 0x54) && (msgs[0].len == 1) && (msgs[1].len == 1))
    {
        /* This is reading data from register 0x54 */
        struct pHidd_I2CDevice_WriteRead msg;
        BOOL result = FALSE;
        
        struct TagItem attrs[] = 
        {
            { aHidd_I2CDevice_Driver,   (IPTR)adap->i2cdriver   },
            { aHidd_I2CDevice_Address,  0xa8                    },
            { aHidd_I2CDevice_Name,     (IPTR)"Read Register"   },
            { TAG_DONE, 0UL }
        };

        (bug("i2c_transfer - reading from register 0x54\n"));

        OOP_Object *obj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);
        
        msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteRead);
        msg.readBuffer = msgs[1].buf;
        msg.readLength = msgs[1].len;
        msg.writeBuffer = msgs[0].buf;
        msg.writeLength = msgs[0].len;

        result = OOP_DoMethod(obj, &msg.mID);
        
        OOP_DisposeObject(obj);
        
        if (result)
            return 2;
        else
            return 0;
    }
    else
    {
        /* Not supported case */
        bug("i2c_transfer case not supported: num = %d\n", num);
NOT_IMPLEMENTED_STOP
    }
    
    /* Failure */
    return 0;
}

int i2c_del_adapter(struct i2c_adapter * adap)
{
    IMPLEMENT("\n");
    return 0;
}