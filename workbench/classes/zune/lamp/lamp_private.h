#ifndef _LAMP_PRIVATE_H_
#define _LAMP_PRIVATE_H_

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>

/*** Instance data **********************************************************/
struct Lamp_DATA
{
    /*- Private ------------------------------------------------------------*/
    LONG                lmp_PenNr;
    ULONG               lmp_PenChanged;
    ULONG               lmp_PenChangedOld;

    /*- Protected ----------------------------------------------------------*/

    /*- Public -------------------------------------------------------------*/
    ULONG               lmp_Type;
    ULONG               lmp_ColorSpec;
    ULONG               lmp_Color[3];
    ULONG               lmp_ColorType;
    struct MUI_PenSpec  lmp_PenSpec;
    ULONG               lmp_Style;
    ULONG               lmp_Diameter;
    ULONG               lmp_Intensity;
    LONG                lmp_EffectPen[4];
};

#endif /* _LAMP_PRIVATE_H_ */
