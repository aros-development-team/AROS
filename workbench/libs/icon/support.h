#ifndef _SUPPORT_H_
#define _SUPPORT_H_

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <workbench/icon.h>

VOID GetDefIconName(LONG, UBYTE *);
LONG CalcIconHash(struct DiskObject *dobj);
VOID AddIconToList(struct NativeIcon *icon, struct IconBase *IconBase);
VOID RemoveIconFromList(struct NativeIcon *icon, struct IconBase *IconBase);
struct NativeIcon *GetNativeIcon(struct DiskObject *dobj, struct IconBase *IconBase);

#endif /* _SUPPORT_H_ */
