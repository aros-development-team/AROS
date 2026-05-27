/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#include "nouveau_intern.h"

#include <drm-compat/drm_compat_i2c.h>

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#undef HiddI2CNouveauAttrBase
#define HiddI2CNouveauAttrBase (SD(cl)->i2cNouveauAttrBase)

OOP_Object * METHOD(NouveauI2C, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    
    if(o)
    {
        struct HIDDNouveauI2CData * i2cdata = OOP_INST_DATA(cl, o);
        
        i2cdata->i2c_adapter = GetTagData(aHidd_I2C_Nouveau_Adapter, (IPTR)0, msg->attrList);
        
        if (i2cdata->i2c_adapter == (IPTR)0)
        {
            /* Fail creation of driver if i2c_adapter was not passed */
            OOP_MethodID disp_mid = OOP_GetMethodID((STRPTR)IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
            o = NULL;
        }
    }
    
    return o;
}

void METHOD(NouveauI2C, Hidd_I2C, PutBits)
{
    struct HIDDNouveauI2CData * i2cdata = OOP_INST_DATA(cl, o);
    struct i2c_adapter *adap = (struct i2c_adapter *)i2cdata->i2c_adapter;
    struct i2c_algo_bit_data *bit = adap->algo_data;

    bit->setsda(bit->data, msg->sda);
    bit->setscl(bit->data, msg->scl);
}

void METHOD(NouveauI2C, Hidd_I2C, GetBits)
{
    struct HIDDNouveauI2CData * i2cdata = OOP_INST_DATA(cl, o);
    struct i2c_adapter *adap = (struct i2c_adapter *)i2cdata->i2c_adapter;
    struct i2c_algo_bit_data *bit = adap->algo_data;

    *msg->sda = bit->getsda(bit->data);
    *msg->scl = bit->getscl(bit->data);
}

ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);
