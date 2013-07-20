/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadBattClock() function.
    Lang: english
*/
#include "battclock_intern.h"

#define DEBUG 1
#include <aros/debug.h>

#include <proto/battclock.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/i2c.h>
#include <utility/date.h>
#include <asm/amcc440.h>

/* See rom/battclock/readbattclock.c for documentation */

AROS_LH0(ULONG, ReadBattClock,
    struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    ULONG secs=0;
    OOP_Object *i2c;
    OOP_AttrBase __IHidd_I2CDevice;

    /* The code here looks more complex than it really is */

    struct pHidd_I2C_ProbeAddress p, *msg=&p;
    struct Library *OOPBase = OpenLibrary("oop.library", 0);
    struct Library *I2CBase = OpenLibrary("i2c-amcc440.library", 0);

    __IHidd_I2CDevice = OOP_ObtainAttrBase(IID_Hidd_I2CDevice);

    /* New i2c driver */
    i2c = OOP_NewObject(NULL, CLID_I2C_AMCC440, NULL);

    p.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2C, moHidd_I2C_ProbeAddress);

    D(bug("[BATT] i2c=%08x\n", i2c));

    if (i2c)
    {
    	/* i2c class successfully created. Probe for RTC on the bus... */
    	D(bug("[BATT] Probing i2c RTC...\n"));

    	p.address = 0xd0;

    	if (OOP_DoMethod(i2c, (OOP_Msg) msg))
    	{
    		/* Got it. Now read the data (7 bytes) from address 1 in RTC */
    		char data[7];
    		char wb[1] = {1};

    		struct TagItem attrs[] = {
    				{ aHidd_I2CDevice_Driver,   (IPTR)i2c       },
    				{ aHidd_I2CDevice_Address,  0xd0            },
    				{ aHidd_I2CDevice_Name,     (IPTR)"RTC" },
    				{ TAG_DONE, 0UL }
    		};

    		OOP_Object *obj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);

    		D(bug("[BATT] RTC found. Object=%08x\n", obj));

    		/* i2c device object created. read data now */
    		if (obj)
    		{
    			struct pHidd_I2CDevice_WriteRead msg;

    			msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteRead);
    			msg.readBuffer = &data[0];
    			msg.readLength = 7;
    			msg.writeBuffer = &wb[0];
    			msg.writeLength = 1;

    			OOP_DoMethod(obj, &msg.mID);

    			int i;

    			D(bug("[BATT] Dump:  "));
    			for (i=0; i < 7; i++)
    			{
    				D(bug("%02x ", data[i]));
    			}
    			D(bug("\n"));

    			/* Ok, let's hope data was successfuly read from RTC. Convert it to ClockDate structure */
    			date.year = 2000 + (data[6] & 0xf) + 10*(data[6] >> 4);
    			date.month = (data[5] & 0xf) + (data[5] >> 4)*10;
    			date.mday = (data[4] & 0xf) + (data[4] >> 4)*10;
    			date.hour = (data[2] & 0xf) + ((data[2] & 0x30) >> 4)*10;
    			date.min = (data[1] & 0xf) + ((data[1] & 0x70) >> 4)*10;
    			date.sec = (data[0] & 0xf) + ((data[0] & 0x70) >> 4)*10;

    			/* Done with i2c device */
    			OOP_DisposeObject(obj);
    		}
    	}

    	/* Done with i2c bus */
    	OOP_DisposeObject(i2c);

    	secs=Date2Amiga(&date);
    }

    /* Cleanup */
    OOP_ReleaseAttrBase(IID_Hidd_I2CDevice);
    if (OOPBase)
    	CloseLibrary(OOPBase);
    if (I2CBase)
    	CloseLibrary(I2CBase);

    return secs;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */

