/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#    include <exec/types.h>
#endif

#define DEF_COOLIMAGEHEIGHT 16

struct CoolImage
{
    const UBYTE *data;
    const UBYTE *pal;
    WORD        width;
    WORD        height;
    WORD        depth;
};

extern const struct CoolImage cool_saveimage,
                              cool_loadimage,
                              cool_useimage,
                              cool_cancelimage,
                              cool_dotimage,
                              cool_dotimage2,
                              cool_warnimage,
                              cool_diskimage,
                              cool_switchimage,
                              cool_monitorimage,
                              cool_infoimage,
                              cool_askimage,
                              cool_keyimage;
