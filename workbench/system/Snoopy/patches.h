/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PATCHES_H
#define PATCHES_H

void patches_init(void);
void patches_set(void);
void patches_reset(void);

enum {LIB_Dos, LIB_Exec, LIB_Icon, LIB_Intuition, LIB_Graphics, LIB_last};

enum {
    PATCH_CreateDir,
    PATCH_CurrentDir,
    PATCH_DeleteFile,
    PATCH_DeleteVar,
    PATCH_Execute,
    PATCH_FindVar,
    PATCH_GetVar,
    PATCH_LoadSeg,
    PATCH_Lock,
    PATCH_MakeLink,
    PATCH_NewLoadSeg,
    PATCH_Open,
    PATCH_Rename,
    PATCH_RunCommand,
    PATCH_SetVar,
    PATCH_SystemTagList,
    PATCH_FindPort,
    PATCH_FindResident,
    PATCH_FindSemaphore,
    PATCH_FindTask,
    PATCH_OpenDevice,
    PATCH_OpenLibrary,
    PATCH_OpenResource,
    PATCH_LockPubScreen,
    PATCH_OpenFont,
    PATCH_FindToolType,
    PATCH_MatchToolValue,
    PATCH_last
};

#endif

