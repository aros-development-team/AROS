#ifndef GADGETS_COLORWHEEL_H
#define GADGETS_COLORWHEEL_H

/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: MethodIDs and AttrIDs for the colorwheel class.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define COLORWHEELCLASS "colorwheel.aros"
#define COLORWHEELNAME "Gadgets/colorwheel.gadget"


struct ColorWheelHSB
{
    ULONG cw_Hue;
    ULONG cw_Saturation;
    ULONG cw_Brightness;
};

struct ColorWheelRGB
{
    ULONG cw_Red;
    ULONG cw_Green;
    ULONG cw_Blue;
};


#define WHEEL_Dummy          (TAG_USER+0x04000000)
#define WHEEL_Hue            (WHEEL_DUMMY+1)
#define WHEEL_Saturation     (WHEEL_DUMMY+2)
#define WHEEL_Brightness     (WHEEL_DUMMY+3)
#define WHEEL_HSB            (WHEEL_DUMMY+4)
#define WHEEL_Red            (WHEEL_DUMMY+5)
#define WHEEL_GREen          (WHEEL_DUMMY+6)
#define WHEEL_Blue           (WHEEL_DUMMY+7)
#define WHEEL_RGB            (WHEEL_DUMMY+8)
#define WHEEL_Screen         (WHEEL_DUMMY+9)
#define WHEEL_Abbrv          (WHEEL_DUMMY+10)
#define WHEEL_Donation       (WHEEL_DUMMY+11)
#define WHEEL_BevelBox       (WHEEL_DUMMY+12)
#define WHEEL_GradientSlider (WHEEL_DUMMY+13)
#define WHEEL_MaxPens        (WHEEL_DUMMY+14)


#endif /* GADGETS_COLORWHEEL_H */
