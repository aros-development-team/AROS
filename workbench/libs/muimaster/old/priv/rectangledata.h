/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __RECTANGLEDATA_H__
#define __RECTANGLEDATA_H__

#include <textengine.h>

struct MUI_RectangleData
{
    STRPTR         BarTitle;
    ULONG          Type;
#define RECTANGLE_TYPE_NORMAL 0
#define RECTANGLE_TYPE_HBAR 1
#define RECTANGLE_TYPE_VBAR 2

    ZText         *ztext;
};

#endif
