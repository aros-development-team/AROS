#ifndef _SUPPORT_H_
#define _SUPPORT_H_

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <workbench/icon.h>

#include "icon_intern.h"

/*** Prototypes *************************************************************/
BPTR __OpenIcon_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase);
BOOL __CloseIcon_WB(BPTR file, struct IconBase *IconBase);


VOID GetDefIconName(LONG, UBYTE *);
LONG CalcIconHash(struct DiskObject *dobj);
VOID AddIconToList(struct NativeIcon *icon, struct IconBase *IconBase);
VOID RemoveIconFromList(struct NativeIcon *icon, struct IconBase *IconBase);
struct NativeIcon *GetNativeIcon(struct DiskObject *dobj, struct IconBase *IconBase);

/*** Macros *****************************************************************/
#define OpenIcon(name, mode) (__OpenIcon_WB((name), (mode), IconBase))
#define CloseIcon(file) (__CloseIcon_WB((file), IconBase))

#endif /* _SUPPORT_H_ */
