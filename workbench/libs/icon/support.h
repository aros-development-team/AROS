#ifndef _SUPPORT_H_
#define _SUPPORT_H_

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "icon_intern.h"

/*** Prototypes *************************************************************/
BPTR __OpenIcon_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase);
BOOL __CloseIcon_WB(BPTR file, struct IconBase *IconBase);
BPTR __OpenDefaultIcon_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase);
BOOL __CloseDefaultIcon_WB(BPTR file, struct IconBase *IconBase);

struct DiskObject *__ReadIcon_WB(BPTR file, struct IconBase *IconBase);
BOOL __WriteIcon_WB(BPTR file, struct DiskObject *icon, struct TagItem *tags, struct IconBase *IconBase);
VOID __PrepareIcon_WB(struct DiskObject *icon, struct IconBase *IconBase);
VOID __FetchIconARGB_WB(struct DiskObject *icon, int id, struct IconBase *IconBase);
VOID __FetchIconImage_WB(struct DiskObject *icon, int id, struct IconBase *IconBase);

BPTR __LockObject_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase);
VOID __UnLockObject_WB(BPTR lock, struct IconBase *IconBase);

CONST_STRPTR GetDefaultIconName(LONG type);

LONG CalcIconHash(struct DiskObject *dobj);
APTR AllocMemIcon(struct DiskObject *icon, IPTR size, ULONG req,
    struct IconBase *IconBase);
VOID AddIconToList(struct NativeIcon *icon, struct IconBase *IconBase);
VOID RemoveIconFromList(struct NativeIcon *icon, struct IconBase *IconBase);
struct NativeIcon *GetNativeIcon(struct DiskObject *dobj, struct IconBase *IconBase);

/*** Macros *****************************************************************/
#define OpenIcon(name, mode) (__OpenIcon_WB((name), (mode), LB(IconBase)))
#define CloseIcon(file) (__CloseIcon_WB((file), LB(IconBase)))
#define OpenDefaultIcon(name, mode) (__OpenDefaultIcon_WB((name), (mode), LB(IconBase)))
#define CloseDefaultIcon(file) (__CloseDefaultIcon_WB((file), LB(IconBase)))

#define ReadIcon(file) (__ReadIcon_WB((file), LB(IconBase)))
#define WriteIcon(file, icon, tags) (__WriteIcon_WB((file), (icon), (tags), LB(IconBase)))
#define FetchIconARGB(icon, id) (__FetchIconARGB_WB((icon), (id), LB(IconBase)))
#define FetchIconImage(icon, id) (__FetchIconImage_WB((icon), (id), LB(IconBase)))
#define PrepareIcon(icon) (__PrepareIcon_WB((icon), LB(IconBase)))

#define LockObject(name, mode) (__LockObject_WB((name), (mode), IconBase))
#define UnLockObject(lock) (__UnLockObject_WB((lock), IconBase))

#endif /* _SUPPORT_H_ */
