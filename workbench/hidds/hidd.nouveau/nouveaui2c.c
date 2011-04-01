/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"
#include "nouveau_intern.h"
#include "nouveau_i2c.h"

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
        
        i2cdata->i2c_chan = GetTagData(aHidd_I2C_Nouveau_Chan, (IPTR)0, msg->attrList);
        
        if (i2cdata->i2c_chan == (IPTR)0)
        {
            /* Fail creation of driver is nouveau_i2c_chan was not passed */
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
    struct nouveau_i2c_chan * i2c_chan = (struct nouveau_i2c_chan *)i2cdata->i2c_chan;
 
    i2c_chan->bit.setsda(i2c_chan, msg->sda);
    i2c_chan->bit.setscl(i2c_chan, msg->scl);
}

void METHOD(NouveauI2C, Hidd_I2C, GetBits)
{
    struct HIDDNouveauI2CData * i2cdata = OOP_INST_DATA(cl, o);
    struct nouveau_i2c_chan * i2c_chan = (struct nouveau_i2c_chan *)i2cdata->i2c_chan;

    *msg->sda = i2c_chan->bit.getsda(i2c_chan);
    *msg->scl = i2c_chan->bit.getscl(i2c_chan);
}

ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);
