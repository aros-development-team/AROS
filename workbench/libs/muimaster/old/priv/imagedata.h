/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __IMAGEDATA_H__
#define __IMAGEDATA_H__

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct MUI_ImageSpec;

struct MUI_ImageData
{
    ULONG  mid_Flags;
#define MIDF_FREEHORIZ (1<<0)
#define MIDF_FREEVERT (1<<1)
#define MIDF_INITIALIZED (1<<2)

    ULONG  mid_State; /* IDS_NORMAL, IDS_SELECTED, IDS_DISABLED */
    ULONG  mid_OrigSpec;
    struct MUI_ImageSpec *mid_ImageSpec;
};

#endif
