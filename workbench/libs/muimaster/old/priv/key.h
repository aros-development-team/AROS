/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __ZUNEKEY_H__
#define __ZUNEKEY_H__

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef __AROS__
#include <gdk/gdktypes.h>
#endif

#include <muikey.h>

#ifndef __AROS__
BOOL zune_key_translate (Object *win, GdkEventKey *event, ULONG *muikey);
#endif

void zune_keyspec_parse (ZuneKeySpec *spec);


#endif
