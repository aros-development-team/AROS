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
    OOP_Object  *driver;
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
  
#endif /* _I2C_H */

