/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef SETPATCH_INTERN_H
#define SETPATCH_INTERN_H

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>

#define SETPATCH_1_NAME "SetPatch-01"

struct SetPatch_1 {
    struct MsgPort sp_MsgPort;
};

#define SETPATCH_2_NAME "SetPatch Port"

struct SetPatch_2_Entry {
   ULONG        se_Valid;       /* 0 terminates the list */
   CONST_STRPTR se_Name;
};

struct SetPatch_2 {
   struct MsgPort sp_MsgPort;
   UWORD    sp_Version_Major;
   UWORD    sp_Version_Minor;
   struct SetPatch_2_Entry *sp_PatchTable;
   ULONG    sp_PatchEntrySize;
   ULONG    sp_ThisIsTheValue2;
};


#define SETPATCH_3_NAME "\253 SetPatch \273"

struct SetPatch_3 {
    struct SignalSemaphore  sp_Semaphore;
    struct MinList          sp_PatchList;
    UWORD                   sp_Version_Major;
    UWORD                   sp_Version_Minor;
};

struct SetPatchBase {
    struct Library  sp_Library;
    struct SetPatch_1   sp_Patch1;
    struct SetPatch_2   sp_Patch2;
    struct SetPatch_2_Entry sp_Entry2[2];   /* Fake list */
    struct SetPatch_3   sp_Patch3;          /* Actual list */
    APTR   sp_OldFindSemaphore;
#if 0
    APTR   sp_OldFindPort;
#endif
};

#endif /* SETPATCH_INTERN_H */
