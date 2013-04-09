/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PATCHES_H
#define PATCHES_H

void patches_init(void);
void patches_set(void);
void patches_reset(void);

char *MyNameFromLock(BPTR lock, char *filename, char *buf, int maxlen);
void GetVolName(BPTR lock, char *buf, int maxlen);

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

/*
 * Macro (courtesy of Doug Walker) used to allocate longword-aligned
 * data on the stack. We can't use __aligned inside our patches
 * because the caller may not have a longword-aligned stack.
 */
#define D_S(name, type) char c_##name[sizeof(type)+3]; \
    type *name = (type *)((long)(c_##name+3) & ~3)

#endif

