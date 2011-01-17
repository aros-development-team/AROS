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

#endif /* RESOURCES_FILESYSRES_H */
