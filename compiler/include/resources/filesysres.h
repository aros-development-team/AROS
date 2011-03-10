/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RESOURCES_FILESYSRES_H
#define RESOURCES_FILESYSRES_H

#include <exec/lists.h>
#include <dos/bptr.h>

#define FSRNAME "FileSystem.resource"

struct FileSysResource
{
    struct Node fsr_Node;
    char       *fsr_Creator;
    struct List fsr_FileSysEntries;
};

struct FileSysEntry
{
    struct Node fse_Node;
    ULONG       fse_DosType;
    ULONG       fse_Version;
    ULONG       fse_PatchFlags;
    ULONG       fse_Type;
    IPTR        fse_Task;
    BPTR        fse_Lock;
    BSTR        fse_Handler;
    ULONG       fse_StackSize;
    LONG        fse_Priority;
    BPTR        fse_Startup;
    BPTR        fse_SegList;
    BPTR        fse_GlobalVec;
};

/* fse_PatchFlags - when set, the corresponding
 * field from struct FileSysEntry is used instead
 * of the DOS defaults.
 */
#define FSEB_TYPE      0
#define FSEF_TYPE      (1 << 0)
#define FSEB_TASK      1
#define FSEF_TASK      (1 << 1)
#define FSEB_LOCK      2
#define FSEF_LOCK      (1 << 2)
#define FSEB_HANDLER   3
#define FSEF_HANDLER   (1 << 3)
#define FSEB_STACKSIZE 4
#define FSEF_STACKSIZE (1 << 4)
#define FSEB_PRIORITY  5
#define FSEF_PRIORITY  (1 << 5)
#define FSEB_STARTUP   6
#define FSEF_STARTUP   (1 << 6)
#define FSEB_SEGLIST   7
#define FSEF_SEGLIST   (1 << 7)
#define FSEB_GLOBALVEC 8
#define FSEF_GLOBALVEC (1 << 8)

#endif /* RESOURCES_FILESYSRES_H */
