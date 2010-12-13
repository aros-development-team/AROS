#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <dos/filesystem.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/resident.h>

#include "asfsbase.h"
// #include "cdrom.h"
//#include "devsupp.h"

#include LC_LIBDEFS_FILE

void ASFS_work(struct ASFSBase *);
void *ASFS_GetData(struct ASFSBase *);

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR asfsbase)
{
        struct Task *task;
        APTR stack;

        asfsbase->DOSBase = (struct DOSBase *)OpenLibrary("dos.library",41);
        if (asfsbase->DOSBase)
        {
                NEWLIST(&asfsbase->port.mp_MsgList);
                asfsbase->port.mp_Node.ln_Type = NT_MSGPORT;
                asfsbase->port.mp_SigBit = SIGBREAKB_CTRL_F;
                NEWLIST(&asfsbase->rport.mp_MsgList);
                asfsbase->rport.mp_Node.ln_Type = NT_MSGPORT;
                asfsbase->rport.mp_Flags = PA_SIGNAL;
                asfsbase->rport.mp_SigBit = SIGB_SINGLE;
                NEWLIST(&asfsbase->prport.mp_MsgList);
                asfsbase->rport.mp_Node.ln_Type = NT_MSGPORT;
                asfsbase->rport.mp_Flags = PA_SIGNAL;
                /* signal will be initialized in task */
                task = (struct Task *)AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
                if (task != NULL)
                {
                        asfsbase->port.mp_SigTask = task;
                        asfsbase->port.mp_Flags = PA_IGNORE;
                        asfsbase->prport.mp_SigTask = task;
                        NEWLIST(&task->tc_MemEntry);
                        task->tc_Node.ln_Type = NT_TASK;
                        task->tc_Node.ln_Name = "sfs.handler task";
                        task->tc_Node.ln_Pri = 10;
                        stack = AllocMem(AROS_STACKSIZE, MEMF_PUBLIC);
                        if (stack != NULL)
                        {
                                struct TagItem tags[] =
                                {
                                    {TASKTAG_ARG1, (IPTR)asfsbase},
                                    {TAG_DONE                    }
                                };

                                task->tc_SPLower = stack;
                                task->tc_SPUpper = (BYTE *)stack+AROS_STACKSIZE;
                            #if AROS_STACK_GROWS_DOWNWARDS
                                task->tc_SPReg = (BYTE *)task->tc_SPUpper-SP_OFFSET;
                            #else
                                task->tc_SPReg = (BYTE *)task->tc_SPLower+SP_OFFSET;
                            #endif
                                NEWLIST(&asfsbase->process_list);
                                asfsbase->GetData=ASFS_GetData;
                                if (NewAddTask(task,ASFS_work,NULL,tags) != NULL)
                                        return TRUE;
                                FreeMem(stack, AROS_STACKSIZE);
                        }
                                    FreeMem(task, sizeof(struct Task));
                }
                CloseLibrary((struct Library *)asfsbase->DOSBase);
        }
        return FALSE;
}

#ifdef SysBase
#       undef SysBase
#endif
#define SysBase asfsbase->SysBase


static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR asfsbase,
    struct IOFileSys *iofs,
    ULONG unitnum,
    ULONG flags
)
{
        unitnum = flags = 0;
        asfsbase->rport.mp_SigTask=FindTask(NULL);

        iofs->IOFS.io_Command = -1;
        PutMsg(&asfsbase->port, &iofs->IOFS.io_Message);
        WaitPort(&asfsbase->rport);
        (void)GetMsg(&asfsbase->rport);
        if (iofs->io_DosError == NULL)
        {
                iofs->IOFS.io_Device = &asfsbase->device;
                iofs->IOFS.io_Error = 0;
                return TRUE;
        }
        iofs->IOFS.io_Error = IOERR_OPENFAIL;
        return FALSE;
}

#ifdef DOSBase
#       undef DOSBase
#endif
#define DOSBase asfsbase->DOSBase

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR asfsbase)
{
        RemTask(asfsbase->port.mp_SigTask);
        FreeMem(((struct Task *)asfsbase->port.mp_SigTask)->tc_SPLower,AROS_STACKSIZE);
        FreeMem(asfsbase->port.mp_SigTask, sizeof(struct Task));
        CloseLibrary((struct Library *)DOSBase);
	return TRUE;
}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR asfsbase,
    struct IOFileSys *iofs
)
{
        asfsbase->rport.mp_SigTask=FindTask(NULL);
        iofs->IOFS.io_Command = -2;
        PutMsg(&asfsbase->port, &iofs->IOFS.io_Message);
        WaitPort(&asfsbase->rport);
        (void)GetMsg(&asfsbase->rport);
        if (iofs->io_DosError)
                return FALSE;                               // there is still something to do on this volume
        iofs->IOFS.io_Device=(struct Device *)-1;
        return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0) 
ADD2OPENDEV(GM_UNIQUENAME(Open),0) 
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0) 
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0) 

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
           struct ASFSBase *, asfsbase, 5, asfsdev)
{
        AROS_LIBFUNC_INIT
        /* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        /* Nothing is done quick */
    iofs->IOFS.io_Flags &= ~IOF_QUICK;
        /* So let the device task do it */
    PutMsg(&asfsbase->port, &iofs->IOFS.io_Message);
        AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
           struct ASFSBase *, asfsbase, 6, asfsdev)
{
        AROS_LIBFUNC_INIT
        return 0;
        AROS_LIBFUNC_EXIT
}
