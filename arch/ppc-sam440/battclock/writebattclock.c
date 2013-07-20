/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: WriteBattClock()
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
#include <stdarg.h>

/* See rom/battclock/writebattclock.c for documentation */

AROS_LH1(void, WriteBattClock,
    AROS_LHA(ULONG, time, D0),
    APTR *, BattClockBase, 3, Battclock)
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    OOP_Object *i2c;
    OOP_AttrBase __IHidd_I2CDevice;

    /* The code here looks more complex than it really is */

    struct pHidd_I2C_ProbeAddress p, *msg=&p;
    struct Library *OOPBase = OpenLibrary("oop.library", 0);
    struct Library *I2CBase = OpenLibrary("i2c-amcc440.library", 0);

    Amiga2Date(time, &date);

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
        		char wb[8] = {1,};

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
        			msg.readBuffer = &wb[1];
        			msg.readLength = 7;
        			msg.writeBuffer = &wb[0];
        			msg.writeLength = 1;

        			OOP_DoMethod(obj, &msg.mID);

        			int i;

        			D(bug("[BATT] Old dump:  "));
        			for (i=1; i < 8; i++)
        			{
        				D(bug("%02x ", wb[i]));
        			}
        			D(bug("\n"));

        			/* Data read. Modify the bits and pieces now */
        			wb[1] = (wb[1] & 0x80) | (date.sec / 10) << 4 | (date.sec % 10);
        			wb[2] = (date.min / 10) << 4 | (date.min % 10);
        			wb[3] = (wb[3] & 0xc0) | (date.hour / 10) << 4 | (date.hour % 10);
        			wb[4] = (wb[4] & 0x80) | (date.wday);
        			wb[5] = (date.mday / 10) << 4 | (date.mday % 10);
        			wb[6] = (date.month / 10) << 4 | (date.month % 10);
        			wb[7] = ((date.year - 2000) / 10) << 4 | ((date.year - 2000) % 10);

        			D(bug("[BATT] New dump:  "));
        			for (i=1; i < 8; i++)
        			{
        				D(bug("%02x ", wb[i]));
        			}
        			D(bug("\n"));

        			/* Write data to RTC */
        			wb[0] = 1;
        			msg.readBuffer = NULL;
        			msg.readLength = 0;
        			msg.writeBuffer = &wb[0];
        			msg.writeLength = 8;

        			OOP_DoMethod(obj, &msg.mID);

        			/* Done with i2c device */
        			OOP_DisposeObject(obj);
        		}
        	}

        	/* Done with i2c bus */
        	OOP_DisposeObject(i2c);
        }

        /* Cleanup */
        OOP_ReleaseAttrBase(IID_Hidd_I2CDevice);
        if (OOPBase)
        	CloseLibrary(OOPBase);
        if (I2CBase)
        	CloseLibrary(I2CBase);

    AROS_LIBFUNC_EXIT
} /* WriteBattClock */

