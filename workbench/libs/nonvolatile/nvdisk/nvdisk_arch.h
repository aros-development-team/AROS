#ifndef NVDISK_ARCH_H
#define NVDISK_ARCH_H

/* Platforms without a native nonvolatile-storage backend use the filesystem
 * implementation in nvdisk.library.  These constants let the compiler remove
 * the architecture dispatch entirely. */
#define NVDisk_ArchInit(base)                         FALSE
#define NVDisk_ArchActive(base)                       FALSE
#define NVDisk_ArchExpunge(base)                      ((void)0)
#define NVDisk_ArchRead(base, appName, itemName)      ((APTR)0)
#define NVDisk_ArchWrite(base, appName, itemName, data, length) 0
#define NVDisk_ArchDelete(base, appName, itemName)    FALSE
#define NVDisk_ArchInfo(base, info)                   FALSE
#define NVDisk_ArchProtect(base, appName, itemName, mask) FALSE
#define NVDisk_ArchList(base, appName)                ((struct MinList *)0)

#endif
