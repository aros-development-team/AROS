#ifndef __ZUNEKEY_H__
#define __ZUNEKEY_H__

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef _AROS
#include <gdk/gdktypes.h>
#endif

#include <muikey.h>

#ifndef _AROS
BOOL zune_key_translate (Object *win, GdkEventKey *event, ULONG *muikey);
#endif

void zune_keyspec_parse (ZuneKeySpec *spec);


#endif
