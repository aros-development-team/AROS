/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <drm-compat/drm_compat_i2c.h>
#include <drm-compat/drm_compat_funcs.h>

#include <proto/oop.h>
#include <hidd/i2c.h>


OOP_AttrBase HiddI2CDeviceAttrBase = 0; /* TODO: Implement  freeing */

/* This function assumes there are two messages in msgs[] */
static int i2c_writeread(struct i2c_adapter *adap, struct i2c_msg *msgs)
{
    struct pHidd_I2CDevice_WriteRead msg;
    BOOL result = FALSE;

    struct TagItem attrs[] =
    {
        { aHidd_I2CDevice_Driver,   (IPTR)adap->i2cdriver   },
        { aHidd_I2CDevice_Address,  msgs[0].addr << 1       }, /* AROS has shifted addresses (<< 1) */
        { aHidd_I2CDevice_Name,     (IPTR)"WriteRead Call"  },
        { TAG_DONE, 0UL }
    };

    D(bug("i2c_transfer -WriteRead Call\n"));

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

/* This function assumes there is one message in msgs[] */
static int i2c_write(struct i2c_adapter *adap, struct i2c_msg *msgs)
{
    struct pHidd_I2CDevice_Write msg;
    BOOL result = FALSE;

    struct TagItem attrs[] =
    {
        { aHidd_I2CDevice_Driver,   (IPTR)adap->i2cdriver   },
        { aHidd_I2CDevice_Address,  msgs[0].addr << 1       }, /* AROS has shifted addresses (<< 1) */
        { aHidd_I2CDevice_Name,     (IPTR)"Write Call"      },
        { TAG_DONE, 0UL }
    };

    D(bug("i2c_transfer -Write Call\n"));

    OOP_Object *obj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);

    msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_Write);
    msg.writeBuffer = msgs[0].buf;
    msg.writeLength = msgs[0].len;

    result = OOP_DoMethod(obj, &msg.mID);

    OOP_DisposeObject(obj);

    if (result)
        return 1;
    else
        return 0;
}

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{

    if (adap->type == ADAP_TYPE_ALGO)
    {
NOT_IMPLEMENTED_STOP
return 0;
    }

    /* FIXME: This function is not generic. It has hardcoded cases that are present in nouveau */
    if (adap->i2cdriver == (IPTR)0)
    {
        bug("ERROR: i2c_transfer called without driver present for adapter %p\n", adap);
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
        if (HIDD_I2C_ProbeAddress((OOP_Object *)adap->i2cdriver, msgs[0].addr << 1)) /* AROS has shifted addresses (<< 1) */
            return 2;
        else
            return 0;
    }
    else if ((num == 2) && (msgs[0].addr == 0x4c) && (msgs[1].addr == 0x4c) && (msgs[0].len == 1) && (msgs[1].len == 1))
    {
        /* G96 probing monitoring devices */
        D(bug("i2c_transfer - G96 probing for monitoring devices \n"));
        if (HIDD_I2C_ProbeAddress((OOP_Object *)adap->i2cdriver, msgs[0].addr << 1)) /* AROS has shifted addresses (<< 1) */
            return 2;
        else
            return 0;
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
            { aHidd_I2CDevice_Address,  0xa0                    }, /* AROS has shifted addresses (<< 1) */
            { aHidd_I2CDevice_Name,     (IPTR)"Read DDC"        },
            { TAG_DONE, 0UL }
        };

        D(bug("i2c_transfer - reading from DDC\n"));

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
NOT_IMPLEMENTED_STOP
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
    else if ((num == 2) && (msgs[0].addr == 0x54) && (msgs[1].addr == 0x54) && (msgs[0].len == 2) && (msgs[1].len == 1))
    {
        /* Read during BIOS init, triggered first on GTX 550 Ti */
        return i2c_writeread(adap, msgs);
    }
    else if ((num == 1) && (msgs[0].flags == 0x0))
    {
        /* Generic write call */
        bug("i2c_transfer: generic WRITE call at addr 0x%x len %d\n", msgs[0].addr, msgs[0].len);
        return i2c_write(adap, msgs);
    }
    else
    {
        /* Not supported case */
        bug("i2c_transfer case not supported: num = %d\n", num);
        for (int i = 0; i < num; i++)
            bug("   msg%d addr 0x%x len %d\n", i, msgs[i].addr, msgs[i].len);
NOT_IMPLEMENTED_STOP
    }
    
    /* Failure */
    return 0;
}

int i2c_del_adapter(struct i2c_adapter *adap)
{
    NOT_IMPLEMENTED_STOP
    return 0;
}

/* FIXME: Duplicate defines here. Don't include nouveau_intern.h */
/* Ugly hack actually */
#define CLID_Hidd_I2C_Nouveau       "hidd.i2c.nouveau"
#define IID_Hidd_I2C_Nouveau        "hidd.i2c.nouveau"

#define HiddI2CNouveauAttrBase      __IHidd_I2C_Nouveau
#define aoHidd_I2C_Nouveau_Adapter  0
#define aHidd_I2C_Nouveau_Adapter   (HiddI2CNouveauAttrBase + aoHidd_I2C_Nouveau_Adapter)

OOP_AttrBase HiddI2CNouveauAttrBase = 0;

int i2c_bit_add_bus(struct i2c_adapter *adap)
{
    if (HiddI2CNouveauAttrBase == 0)
        HiddI2CNouveauAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2C_Nouveau);

    struct TagItem i2c_attrs[] =
    {
        { aHidd_I2C_Nouveau_Adapter, (IPTR)adap },
        { TAG_DONE, 0UL }
    };

    adap->i2cdriver = (IPTR)OOP_NewObject(NULL, CLID_Hidd_I2C_Nouveau, i2c_attrs);
    if (adap->i2cdriver == (IPTR)0)
    {
        bug("Failed to create CLID_Hidd_I2C_Nouveau object\n");
        return -EINVAL;
    }
    adap->type = ADAP_TYPE_DEFAULT;

    return 0;
}

int i2c_add_adapter(struct i2c_adapter *adap)
{
    adap->type = ADAP_TYPE_ALGO;
    return 0;
}
