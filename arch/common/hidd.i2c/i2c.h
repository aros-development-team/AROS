#ifndef _I2C_H
#define _I2C_H

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

/* Private data and structures unavailable outside the i2c base classes */

typedef struct DevInstData {
    struct SignalSemaphore  lock;
    OOP_Object  *driver;
    STRPTR      name;
    UWORD       address;
    ULONG   HoldTime;
    ULONG   BitTimeout;
    ULONG   ByteTimeout;
    ULONG   AcknTimeout;
    ULONG   StartTimeout;
    ULONG   RiseFallTime;
} tDevData;

struct i2c_staticdata {
    struct ExecBase	        *sysbase;
    struct Library	        *utilitybase;
    
    struct SignalSemaphore  driver_lock;
    struct MinList          devices;

    STRPTR          name;
        
    ULONG           HoldTime;
    ULONG           BitTimeout;
    ULONG           ByteTimeout;
    ULONG           AcknTimeout;
    ULONG           StartTimeout;
    ULONG           RiseFallTime;
  
    OOP_AttrBase	hiddAB;
    OOP_AttrBase	hiddI2CAB;
    OOP_AttrBase	hiddI2CDeviceAB;

    OOP_Class		*i2cClass;
    OOP_Class		*i2cDeviceClass;
    
    OOP_MethodID    mid_I2C_UDelay;
    OOP_MethodID    mid_I2C_Start;
    OOP_MethodID    mid_I2C_Stop;
    OOP_MethodID    mid_I2C_PutBits;
    OOP_MethodID    mid_I2C_GetBits;
    OOP_MethodID    mid_I2C_PutByte;
    OOP_MethodID    mid_I2C_GetByte;
    OOP_MethodID    mid_I2C_Address;
    OOP_MethodID    mid_I2C_WriteRead;

};

struct i2cbase {
    struct Library 		    LibNode;
    struct ExecBase		    *sysbase;
    BPTR			        seglist;
    APTR			        MemPool;
    struct i2c_staticdata	sd;
};

#define BASE(lib) ((struct i2cbase*)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#if 0

#define LOCK_DEV    ObtainSemaphore(&dev->lock);
#define UNLOCK_DEV  ReleaseSemaphore(&dev->lock);

#define LOCK_HW     ObtainSemaphore(&SD(cl)->driver_lock);
#define UNLOCK_HW   ReleaseSemaphore(&SD(cl)->driver_lock);

#else

#define LOCK_DEV    /* */
#define UNLOCK_DEV  /* */

#define LOCK_HW     /* */
#define UNLOCK_HW   /* */

#endif

#define I2C_UDelay(__o, __delay) \
    ({ \
        struct pHidd_I2C_UDelay __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_UDelay)               \
            SD(cl)->mid_I2C_UDelay = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_UDelay);    \
        __p.mID   = SD(cl)->mid_I2C_UDelay;        \
        __p.delay = (__delay);                        \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })

#define I2C_Start(__o, __timeout) \
    ({ \
        struct pHidd_I2C_Start __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_Start)               \
            SD(cl)->mid_I2C_Start = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_Start);    \
        __p.mID   = SD(cl)->mid_I2C_Start;        \
        __p.timeout = (__timeout);                        \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })

#define I2C_Stop(__o, __device) \
    ({ \
        struct pHidd_I2C_Stop __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_Stop)               \
            SD(cl)->mid_I2C_Stop = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_Stop);    \
        __p.mID   = SD(cl)->mid_I2C_Stop;        \
        __p.device = (__device);                        \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })

#define I2C_PutBits(__o, __scl, __sda) \
    ({ \
        struct pHidd_I2C_PutBits __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_PutBits)               \
            SD(cl)->mid_I2C_PutBits = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_PutBits);    \
        __p.mID   = SD(cl)->mid_I2C_PutBits;        \
        __p.scl   = (__scl);                        \
        __p.sda   = (__sda);                        \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })

#define I2C_PutByte(__o, __device, __byte) \
    ({ \
        struct pHidd_I2C_PutByte __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_PutByte)               \
            SD(cl)->mid_I2C_PutByte = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_PutByte);    \
        __p.mID   = SD(cl)->mid_I2C_PutByte;        \
        __p.data   = (__byte);                        \
        __p.device   = (__device);                        \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })

#define I2C_GetByte(__o, __device, __byte, __last) \
    ({ \
        struct pHidd_I2C_GetByte __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_GetByte)               \
            SD(cl)->mid_I2C_GetByte = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_GetByte);    \
        __p.mID   = SD(cl)->mid_I2C_GetByte;        \
        __p.data   = (__byte);                        \
        __p.last   = (__last);                        \
        __p.device   = (__device);                        \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })

#define I2C_GetBits(__o, __scl, __sda) \
    ({ \
        struct pHidd_I2C_GetBits __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_GetBits)               \
            SD(cl)->mid_I2C_GetBits = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_GetBits);    \
        __p.mID   = SD(cl)->mid_I2C_GetBits;        \
        __p.scl   = (__scl);                        \
        __p.sda   = (__sda);                        \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })

#define I2C_Address(__o, __device, __addr) \
    ({ \
        struct pHidd_I2C_Address __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_Address)               \
            SD(cl)->mid_I2C_Address = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_Address);    \
        __p.mID   = SD(cl)->mid_I2C_Address;        \
        __p.address  = (__addr);                        \
        __p.device   = (__device);                        \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })

#define I2C_WriteRead(__o, __device, __wb, __wl, __rb, __rl) \
    ({ \
        struct pHidd_I2C_WriteRead __p, *__m=&__p;    \
        if (!SD(cl)->mid_I2C_WriteRead)               \
            SD(cl)->mid_I2C_WriteRead = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_WriteRead);    \
        __p.mID   = SD(cl)->mid_I2C_WriteRead;        \
        __p.device   = (__device);                        \
        __p.writeBuffer = (__wb);   \
        __p.writeLength = (__wl);   \
        __p.readBuffer = (__rb);   \
        __p.readLength = (__rl);   \
        OOP_DoMethod((__o), (OOP_Msg)__m);          \
    })


#endif /* _I2C_H */

