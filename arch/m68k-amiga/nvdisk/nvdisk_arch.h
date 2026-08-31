#ifndef NVDISK_ARCH_H
#define NVDISK_ARCH_H

#include <exec/types.h>
#include <exec/lists.h>
#include <libraries/nonvolatile.h>

struct NVDBase;

BOOL NVDisk_ArchInit(struct NVDBase *base);
BOOL NVDisk_ArchActive(const struct NVDBase *base);
void NVDisk_ArchExpunge(struct NVDBase *base);
APTR NVDisk_ArchRead(struct NVDBase *base, CONST_STRPTR appName,
    CONST_STRPTR itemName);
LONG NVDisk_ArchWrite(struct NVDBase *base, CONST_STRPTR appName,
    CONST_STRPTR itemName, CONST_APTR data, ULONG length);
BOOL NVDisk_ArchDelete(struct NVDBase *base, CONST_STRPTR appName,
    CONST_STRPTR itemName);
BOOL NVDisk_ArchInfo(struct NVDBase *base, struct NVInfo *info);
BOOL NVDisk_ArchProtect(struct NVDBase *base, CONST_STRPTR appName,
    CONST_STRPTR itemName, ULONG mask);
struct MinList *NVDisk_ArchList(struct NVDBase *base,
    CONST_STRPTR appName);

#endif
