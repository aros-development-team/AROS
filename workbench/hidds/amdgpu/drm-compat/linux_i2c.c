/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/i2c.h>

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/err.h>

#include <drm-compat/drm_compat_funcs.h>


OOP_AttrBase HiddI2CDeviceAttrBase = 0; /* TODO: Implement  freeing */
OOP_AttrBase HiddI2CAttrBase = 0; /* TODO: Implement  freeing */

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

    D(bug("i2c_transfer - WriteRead Call\n"));

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

    D(bug("i2c_transfer - Write Call\n"));

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

/*
 * A bit-banged nvkm bus is driven by the AROS i2c.hidd, which does the
 * protocol on top of the get/set-bit callbacks exported through the
 * hidd.i2c.nouveau class.
 */
static int i2c_bit_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    if (adap->i2cdriver == (IPTR)0)
    {
        bug("ERROR: i2c_transfer called without driver present for adapter %p\n", adap);
        return -ENODEV;
    }
    
    if (HiddI2CDeviceAttrBase == 0)
        HiddI2CDeviceAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2CDevice);

    if (HiddI2CDeviceAttrBase == 0)
    {
        bug("ERROR: i2c_trasfer not able to obtain HiddI2CDeviceAttrBase\n");
        return 0;
    }

    /* Preconfitions met */
    struct i2c_algo_bit_data *algo = adap->algo_data;
    int ret = 0;

    if (algo->pre_xfer)
    {
        ret = algo->pre_xfer(adap);
        if (ret)
            return ret;
    }

    /* Go through supported cases */
    if ((num == 2) && (msgs[0].addr == msgs[1].addr) && (msgs[0].len == 1) && (msgs[1].len == 1) && (msgs[0].flags == 0) && (msgs[1].flags == I2C_M_RD))
    {
        D(bug("i2c_transfer: generic PROBE call at addr 0x%x\n", msgs[0].addr));
        if (HIDD_I2C_ProbeAddress((OOP_Object *)adap->i2cdriver, msgs[0].addr << 1)) /* AROS has shifted addresses (<< 1) */
            ret = 2;
        else
            ret = 0;
    }
    else if ((num == 2) && (msgs[0].addr == msgs[1].addr) && (msgs[0].len > 1 || msgs[1].len > 1) && (msgs[0].flags == 0) && (msgs[1].flags == I2C_M_RD))
    {
        /* Generic writeread call */
        D(bug("i2c_transfer: generic WRITEREAD call at addr 0x%x len0 %d, len1 %d\n", msgs[0].addr, msgs[0].len, msgs[1].len));
        ret = i2c_writeread(adap, msgs);
    }
    else if ((num == 1) && (msgs[0].flags == 0x0))
    {
        /* Generic write call */
        D(bug("i2c_transfer: generic WRITE call at addr 0x%x len %d\n", msgs[0].addr, msgs[0].len));
        ret = i2c_write(adap, msgs);
    }
    else
    {
        /* Not supported case */
        bug("i2c_transfer case not supported: num = %d\n", num);
        for (int i = 0; i < num; i++)
            bug("   msg%d addr 0x%x len %d flags 0x%x\n", i, msgs[i].addr, msgs[i].len, msgs[i].flags);
        ret = -EOPNOTSUPP;
    }

    if (algo->post_xfer)
        algo->post_xfer(adap);

    return ret;
}

static u32 i2c_bit_func(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

const struct i2c_algorithm i2c_bit_algo = {
    .master_xfer = i2c_bit_xfer,
    .functionality = i2c_bit_func,
};

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    if (adap->algo && adap->algo->master_xfer)
        return adap->algo->master_xfer(adap, msgs, num);
    if (adap->algo && adap->algo->xfer)
        return adap->algo->xfer(adap, msgs, num);
    return -EOPNOTSUPP;
}

int __i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    return i2c_transfer(adap, msgs, num);
}

int i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
    struct i2c_msg msg = { .addr = client->addr, .flags = client->flags & I2C_M_TEN, .len = count, .buf = (u8 *)buf };
    int ret = i2c_transfer(client->adapter, &msg, 1);
    return ret == 1 ? count : ret;
}

int i2c_master_recv(const struct i2c_client *client, char *buf, int count)
{
    struct i2c_msg msg = { .addr = client->addr, .flags = (client->flags & I2C_M_TEN) | I2C_M_RD, .len = count, .buf = (u8 *)buf };
    int ret = i2c_transfer(client->adapter, &msg, 1);
    return ret == 1 ? count : ret;
}

struct i2c_client *i2c_new_client_device(struct i2c_adapter *adap, struct i2c_board_info const *info)
{
    /* no slave drivers exist here */
    return ERR_PTR(-ENODEV);
}

void i2c_unregister_device(struct i2c_client *client)
{
}

void i2c_del_adapter(struct i2c_adapter *adap)
{
    if (adap->i2cdriver) {
        OOP_DisposeObject((OOP_Object *)adap->i2cdriver);
        adap->i2cdriver = 0;
    }
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

    if (HiddI2CAttrBase == 0)
        HiddI2CAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_I2C);

    ULONG timeout = jiffies_to_usecs(((struct i2c_algo_bit_data *)adap->algo_data)->timeout);
    timeout /= 10; /* I2C expects values in 10 microsecond units */
    struct TagItem i2c_attrs[] =
    {
        { aHidd_I2C_Nouveau_Adapter,    (IPTR)adap },
        { aHidd_I2C_BitTimeout,         timeout },
        { aHidd_I2C_ByteTimeout,        timeout },
        { aHidd_I2C_StartTimeout,       timeout },
        { aHidd_I2C_AcknTimeout,        timeout },
        { TAG_DONE, 0UL }
    };

    adap->i2cdriver = (IPTR)OOP_NewObject(NULL, CLID_Hidd_I2C_Nouveau, i2c_attrs);
    if (adap->i2cdriver == (IPTR)0)
    {
        bug("Failed to create CLID_Hidd_I2C_Nouveau object\n");
        return -EINVAL;
    }
    adap->type = ADAP_TYPE_DEFAULT;
    adap->algo = &i2c_bit_algo;

    return 0;
}

int i2c_bit_add_numbered_bus(struct i2c_adapter *adap)
{
    return i2c_bit_add_bus(adap);
}

int i2c_add_adapter(struct i2c_adapter *adap)
{
    adap->type = ADAP_TYPE_ALGO;
    return 0;
}
