/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: filesysres.h 14582 2007-05-19 23:47:21Z sonic $
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
    LONG        fse_Task;
    BPTR        fse_Lock;
    BSTR        fse_Handler;
    ULONG       fse_StackSize;
    LONG        fse_Priority;
    BPTR        fse_Startup;
    BPTR        fse_SegList;
    BPTR        fse_GlobalVec;
};

#endif /* RESOURCES_FILESYSRES_H */
