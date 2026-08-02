/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Public interface to the DesignWare i2c bus driver.

    This is what a driver for a chip sitting on an i2c bus - an RTC, a
    sensor, an EEPROM - needs to see. It is installed as <hidd/dwi2c.h>.

    The driver is an ordinary hidd.i2c subclass, so a client reaches a
    chip the way it would on any other i2c bus: make a bus object, hang
    a hidd.i2c.device off it, and use the device interface.

        struct Library *drv = OpenLibrary(DWI2C_NAME, 0);

    Opening the module is what registers its class and makes the device
    tree be read, so it has to come first - OOP_FindClass(), and hence
    OOP_NewObject() by name, only ever finds a class some module has
    already added. A machine that describes no controller fails to open.

        struct TagItem busTags[] =
        {
            { aHidd_I2C_DW_Unit, 2  },
            { TAG_DONE,          0  }
        };
        OOP_Object *bus = OOP_NewObject(NULL, CLID_Hidd_I2C_DW, busTags);

        struct TagItem devTags[] =
        {
            { aHidd_I2CDevice_Driver,  (IPTR)bus       },
            { aHidd_I2CDevice_Address, 0x68 << 1       },
            { aHidd_I2CDevice_Name,    (IPTR)"my chip" },
            { TAG_DONE, 0                              }
        };
        OOP_Object *dev = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, devTags);

    after which HIDD_I2CDevice_ReadBytes() and friends work.

    Note the shift: hidd.i2c carries the slave address in the form it
    takes on the wire, with room for the read/write bit at the bottom,
    which is twice the 7 bit address the part is documented with.

    Every transfer is polled - it busies the caller until the controller
    finishes or a bounded spin gives up - so keep transactions short and
    do not start one from interrupt context.
*/

#ifndef HIDD_DWI2C_H
#define HIDD_DWI2C_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef OOP_OOP_H
#include <oop/oop.h>
#endif

/*
 * The bus class. It adds no methods of its own - it is a hidd.i2c whose
 * transfers are carried out by the controller rather than by toggling
 * the two wires - so the only thing its own interface carries is the
 * attribute that says which controller an object is for.
 */
#define CLID_Hidd_I2C_DW    "hidd.i2c.dw"
#define IID_Hidd_I2C_DW     "hidd.i2c.dw"

#define HiddI2CDWAttrBase   __IHidd_I2C_DW

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddI2CDWAttrBase;
#endif

enum
{
    /*
     * Which controller, counted from zero in the order the controllers
     * appear in the device tree. There is no default - a bus object
     * that names no hardware could only fail every transfer - so this
     * has to be given at creation time, and creation fails without it.
     * [ISG]
     */
    aoHidd_I2C_DW_Unit,
    /*
     * The controller's register base, as an alternative selector. A
     * client that found the part in the device tree knows which
     * controller it hangs off by address, and matching on that avoids
     * having to reproduce this driver's enumeration order.
     */
    aoHidd_I2C_DW_MMIOBase,

    num_Hidd_I2C_DW_Attrs
};

#define aHidd_I2C_DW_Unit   (HiddI2CDWAttrBase + aoHidd_I2C_DW_Unit)
#define aHidd_I2C_DW_MMIOBase \
                            (HiddI2CDWAttrBase + aoHidd_I2C_DW_MMIOBase)

#define IS_I2CDW_ATTR(attr, idx) \
    (((idx) = (attr) - HiddI2CDWAttrBase) < num_Hidd_I2C_DW_Attrs)

/* The module to open before any of the above means anything */
#define DWI2C_NAME          "dwi2c.hidd"

#endif /* HIDD_DWI2C_H */
