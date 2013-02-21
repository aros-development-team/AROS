/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

TEXT CONST aros_patchname[] = "AROS";

int SetPatch_Init(struct SetPatchBase *sp)
{
    /* Make dummy port for WB 1.x's SetPatch */
    sp->sp_Patch1.sp_MsgPort.mp_Node.ln_Name = SETPATCH_1_NAME;
    sp->sp_Patch1.sp_MsgPort.mp_Node.ln_Pri = -128;
    sp->sp_Patch1.sp_MsgPort.mp_Flags = PA_IGNORE;
    NEWLIST(&sp->sp_Patch1.sp_MsgPort.mp_MsgList);
    AddPort(&sp->sp_Patch1.sp_MsgPort);

    /* Make dummy port for WB 2.x's SetPatch */
    sp->sp_Entry2[0].se_Valid = 1;
    sp->sp_Entry2[0].se_Name  = aros_patchname;
    sp->sp_Entry2[1].se_Valid = 0;

    sp->sp_Patch2.sp_PatchEntrySize = sizeof(sp->sp_Entry2[0]);
    sp->sp_Patch2.sp_ThisIsTheValue2 = 2;
    sp->sp_Patch2.sp_PatchTable = &sp->sp_Entry2[0];

    sp->sp_Patch2.sp_Version_Major = sp->sp_Library.lib_Version;
    sp->sp_Patch2.sp_Version_Minor = sp->sp_Library.lib_Revision;
   
    sp->sp_Patch2.sp_MsgPort.mp_Node.ln_Name = SETPATCH_2_NAME;
    sp->sp_Patch2.sp_MsgPort.mp_Node.ln_Pri = -128;
    sp->sp_Patch2.sp_MsgPort.mp_Flags = PA_IGNORE;
    NEWLIST(&sp->sp_Patch2.sp_MsgPort.mp_MsgList);
    AddPort(&sp->sp_Patch2.sp_MsgPort);

    /* Make dummy semaphore for WB 3.x's SetPatch */
    sp->sp_Patch3.sp_Version_Major = sp->sp_Library.lib_Version;
    sp->sp_Patch3.sp_Version_Minor = sp->sp_Library.lib_Revision;
   
    NEWLIST(&sp->sp_Patch3.sp_PatchList);
    
    sp->sp_Patch3.sp_Semaphore.ss_Link.ln_Name = SETPATCH_3_NAME;
    sp->sp_Patch3.sp_Semaphore.ss_Link.ln_Pri  = -128;
    AddSemaphore(&sp->sp_Patch3.sp_Semaphore);

    return 1;
}
     
ADD2INITLIB(SetPatch_Init, 0);
