#ifndef _COLORADJUST_PRIVATE_H_
#define _COLORADJUST_PRIVATE_H_

#ifndef _MUIMASTER_SUPPORT_H
#include "support.h"
#endif

/*** Instance data **********************************************************/
struct Coloradjust_DATA
{
    struct Library *colorwheelbase;
    struct Library *gradientsliderbase;
    struct IClass  *notifyclass;
    struct Hook     sliderhook, wheelhook, gradhook;
    Object         *rslider, *gslider, *bslider, *colfield, *wheel, *grad;       
    ULONG           rgb[3];
    UWORD           gradpenarray[3];
    LONG            gradpen;
    BOOL            truecolor;

    EXEC_INTERFACE_DECLARE(struct ColorWheelIFace * icolorwheel);
};

#endif /* _COLORADJUST_PRIVATE_H_ */
