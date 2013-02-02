/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef CDFS_H
#define CDFS_H

#include <exec/execbase.h>
#include <dos/dosextens.h>

struct BCache;
struct CDFSDevice;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   ((sizeof(x)/sizeof((x)[0])))
#endif

struct CDFS {
    struct SignalSemaphore cb_Semaphore;    /* Master CD controller */
    struct ExecBase       *cb_SysBase;
    struct Library        *cb_UtilityBase;

    struct List            cb_Devices;
    struct List            cb_Volumes;
};

/* Allocated by the volume - CDFS does not alloc nor free it! */
struct CDFSLock {
    struct MinNode       cl_Node;           /* For use by CDFS */
    struct FileLock      cl_FileLock;       /* fl_Key must be unique per file - set by filesystem */
    struct FileInfoBlock cl_FileInfoBlock;
    /* Private filesystem data follows */
};

#undef  container_of
#define container_of(ptr, type, member) ({          \
    const typeof(((type *)0)->member) *__mptr = (ptr);    \
             (type *)((char *)__mptr - offsetof(type, member)); })


#define B_LOCK(x)   (((BPTR)(x) == BNULL) ? NULL : container_of(BADDR(x), struct CDFSLock, cl_FileLock))
#define MKB_LOCK(x) MKBADDR(&(x)->cl_FileLock)

struct CDFSVolume;

struct CDFSOps {
    LONG      op_Type;
    LONG    (*op_Mount)(struct CDFSVolume *vol);
    LONG    (*op_Unmount)(struct CDFSVolume *vol);

    LONG    (*op_Locate)(struct CDFSVolume *vol, struct CDFSLock *ilock, CONST_STRPTR file, ULONG mode, struct CDFSLock **nlock);
    VOID    (*op_Close)(struct CDFSVolume *vol, struct CDFSLock *ilock);
    LONG    (*op_ExamineNext)(struct CDFSVolume *vol, struct CDFSLock *ifile, struct FileInfoBlock *fib);
    LONG    (*op_Seek)(struct CDFSVolume *vol, struct CDFSLock *ifile, SIPTR pos, LONG mode, SIPTR *oldpos);
    LONG    (*op_Read)(struct CDFSVolume *vol, struct CDFSLock *ifile, APTR buff, SIPTR len, SIPTR *actual);
};

struct CDFSVolume {
    struct MinNode        cv_Node;        
    struct DeviceList     cv_DosVolume;   /* ISOFS: dl_VolumeDate, dl_VolumeType, dl_Name */
    struct CDFS          *cv_CDFSBase;    /* CDFS: Main reference */
    struct CDFSDevice    *cv_Device;      /* CDFS: Current device */
    struct List           cv_FileLocks;   /* CDFS: Lock list */
    struct InfoData       cv_InfoData;    /* ISOFS: Data about filesystem */
    const struct CDFSOps *cv_Ops;         /* ISOFS: Operations */
    APTR                  cv_Private;     /* ISOFS: Volume private data */
};

#define B_VOLUME(x)   (((BPTR)(x) == BNULL) ? NULL : container_of(BADDR(x), struct CDFSVolume, cv_DosVolume))
#define MKB_VOLUME(x) MKBADDR(&(x)->cv_DosVolume)


struct CDFSDevice {
    struct MinNode      cd_Node;
    struct BCache      *cd_BCache;
    struct CDFSVolume  *cd_Volume;      /* NULL if no volume is present */
};


#define SysBase (cdfs->cb_SysBase)
#define UtilityBase (cdfs->cb_UtilityBase)

LONG CDFS_Handler(struct ExecBase *sysBase);

#endif /* CDFS_H */
