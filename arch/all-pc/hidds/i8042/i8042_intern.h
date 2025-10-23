#ifndef I8042_INTERN_H
#define I8042_INTERN_H

#include <dos/bptr.h>
#include <exec/libraries.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/mouse.h>

/***** Common static data *******************/

struct i8042_staticdata
{
    OOP_Class          *kbdclass;
    OOP_Object         *kbdhidd;

    OOP_Class          *mouseclass;
    OOP_Object         *mousehidd;

    OOP_AttrBase        hiddAttrBase;
    OOP_AttrBase        hiddInputAB;
    OOP_AttrBase        hiddMouseAB;
    OOP_MethodID        hwMethodBase;

    BPTR                cs_SegList;
    APTR                cs_KernelBase;
    struct Library     *cs_OOPBase;
    struct Library     *cs_UtilityBase;
    struct Interrupt    cs_ResetInt;
    UWORD               cs_Flags;
    UBYTE               cs_pad;
    UBYTE               cs_intbits;
};

#define PS2B_DISABLEKEYB    0
#define PS2F_DISABLEKEYB    (1 << PS2B_DISABLEKEYB)
#define PS2B_DISABLEMOUSE   1
#define PS2F_DISABLEMOUSE   (1 << PS2B_DISABLEMOUSE)

struct i8042base
{
    struct Library library;    
    struct i8042_staticdata csd;
};

/****************************************************************************************/

#define XSD(cl) (&((struct i8042base *)cl->UserData)->csd)

#undef HiddAttrBase
#undef HiddInputAB
#undef HiddMouseAB
#undef HWBase
#define HiddAttrBase (XSD(cl)->hiddAttrBase)
#define HiddInputAB    (XSD(cl)->hiddInputAB)
#define HiddMouseAB    (XSD(cl)->hiddMouseAB)
#define HWBase       (XSD(cl)->hwMethodBase)

#define KernelBase  (XSD(cl)->cs_KernelBase)
#define OOPBase     (XSD(cl)->cs_OOPBase)
#define UtilityBase (XSD(cl)->cs_UtilityBase)

#endif
