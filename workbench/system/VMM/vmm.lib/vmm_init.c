/*
 * Copyright (C) 2025, The AROS Development Team
 * All right reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/symbolsets.h>
#include <dos/dostags.h>

#include "../shared_defs.h"

#include LC_LIBDEFS_FILE

const char ManagerName[] = "VMM_Manager";
const char StarterPortName[] = "VMM_Starter_Lib";
const char VMPortName[] = "VMM_Port";
const char VMMName[] = "L:VMM-Handler";

struct TagItem VM_ManagerTags[] = {
    { NP_Seglist, 0},
    { NP_FreeSeglist, 1},
    { NP_Name, (IPTR)ManagerName},
    { NP_StackSize, 4000 },
    { TAG_DONE, 0 }
};

static int VMM_InitLib(LIBBASETYPE *vmmBase)
{
    int retval = FALSE;
    if ((vmmBase->VM_DOSLIB = OpenLibrary("dos.library", 0)) != NULL)
    {
        if ((vmmBase->VM_STARTERPORT = CreateMsgPort()) != NULL)
        {
            vmmBase->VM_STARTERPORT->mp_Node.ln_Name = (char *)StarterPortName;
            AddPort(vmmBase->VM_STARTERPORT);

            if ((vmmBase->VM_PORT = FindPort(VMPortName)) == NULL)
            {
                if ((vmmBase->VM_INIT_MSG = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) != NULL)
                {
                    if ((vmmBase->VM_HNDL_SEGLIST = NewLoadSeg(VMMName, NULL)) != BNULL)
                    {
                        struct Process *vmmProc;
                        VM_ManagerTags[0].ti_Data = (IPTR)vmmBase->VM_HNDL_SEGLIST;
                        if ((vmmProc = CreateNewProc(VM_ManagerTags)) != NULL)
                        {
                            vmmBase->VM_INIT_MSG->VMCommand = VMCMD_Startup;
                            vmmBase->VM_INIT_MSG->StartupParams = 0;
                            vmmBase->VM_INIT_MSG->ReplySignal = 0;

                            PutMsg (&vmmProc->pr_MsgPort, (struct Message*) vmmBase->VM_INIT_MSG);
                            WaitPort (vmmBase->VM_STARTERPORT);
                            vmmBase->VM_INIT_MSG = (struct VMMsg*)GetMsg (vmmBase->VM_STARTERPORT);
                            if (vmmBase->VM_INIT_MSG->VMCommand == VMCMD_InitReady)
                            {
                                vmmBase->VM_PORT = vmmBase->VM_INIT_MSG->VMMessage.mn_ReplyPort;
                            }
                        }
                    }
                }
            }
            if ((vmmBase->VM_PORT) && ((vmmBase->VM_INIT_MSG = AllocMem (sizeof (struct VMMsg), MEMF_PUBLIC)) != NULL))
            {
                vmmBase->VM_INIT_MSG->VMSender = FindTask(NULL);
                vmmBase->VM_INIT_MSG->VMCommand = VMCMD_ReqMemHeader;
                vmmBase->VM_INIT_MSG->ReplySignal = vmmBase->VM_STARTERPORT->mp_SigBit;
                PutMsg (vmmBase->VM_PORT, (struct Message*) vmmBase->VM_INIT_MSG);
                WaitPort (vmmBase->VM_STARTERPORT);
                vmmBase->VM_INIT_MSG = (struct VMMsg*)GetMsg (vmmBase->VM_STARTERPORT);
                vmmBase->VM_SEMA = vmmBase->VM_INIT_MSG->spec_params.init_params.ip_VMSema;
                vmmBase->VM_MEMHEADER = vmmBase->VM_INIT_MSG->spec_params.init_params.ip_VMHeader;

                retval = TRUE;
            }
            RemPort (vmmBase->VM_STARTERPORT);
            DeleteMsgPort (vmmBase->VM_STARTERPORT);
        }
        CloseLibrary(vmmBase->VM_DOSLIB);
        vmmBase->VM_DOSLIB = NULL;
    }
    return retval;
}

static int VMM_ExpungeLib(LIBBASETYPE *vmmBase)
{
    return 1;
}

ADD2INITLIB(VMM_InitLib, 0)
ADD2EXPUNGELIB(VMM_ExpungeLib, 0)
