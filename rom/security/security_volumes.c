
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include <dos/dos.h>

#include "security_intern.h"
#include "security_plugins.h"
#include "security_crypto.h"
#include "security_enforce.h"
#include "security_memory.h"
#include "security_support.h"

extern const char PasswdFileName[];
extern const char GroupFileName[];

/*
 *      Find all MultiUserFileSystem volumes
 */

static BOOL FindVolumes(struct Library *_Base)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;
    struct DosList *dl;
    struct FileSysStartupMsg *sm;
    struct DosEnvec *de;
    BOOL res = FALSE;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    struct secVolume *vollist;
    dl = LockDosList(LDF_DEVICES|LDF_READ);
    while ((dl = NextDosEntry(dl, LDF_DEVICES)))
    {
        if ((dl->dol_Task) && ((SIPTR)(sm = BADDR(dl->dol_misc.dol_handler.dol_Startup)) > 1024) &&
                        (de = BADDR(sm->fssm_Environ)) && (de->de_TableSize >= DE_DOSTYPE) &&
                        ((de->de_DosType == ID_muFS_DISK) ||
                         (de->de_DosType == ID_AFS_muFS_DISK) ||
                         (de->de_DosType == ID_PFS2_muFS_DISK)))
        {
            if ((vollist = (struct secVolume *)MAlloc(sizeof(struct secVolume)))) {
                D(bug( DEBUG_NAME_STR " %s: New Volume @ %p for %p\n", __func__, vollist, dl);)
                vollist->DosList = dl;
                vollist->Process = dl->dol_Task;
                vollist->FS_Flags = 0L; /* Explicitly clear this */
                vollist->Next = secBase->Volumes;
                secBase->Volumes = vollist;
                res = TRUE;
            } else
                Die(NULL, AN_Unknown | AG_NoMemory);
        }
    }	
    UnLockDosList(LDF_DEVICES|LDF_READ);
    return(res);
}

/*
 *      Initialise the Volume Information
 */

BOOL InitVolumes(struct Library *_Base)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;
    static BOOL bootstrapped = FALSE;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    ClearBuffer();
    ObtainSemaphore(&secBase->VolumesSem);
    if (!FindVolumes(_Base))
    {
        /* Check for bootstrap Rendevous 
         * set bootstrapped to TRUE if we find it
         */
        if (!bootstrapped && !(bootstrapped = BootStrapRendevous(secBase)))
        {
            /* Temporary hack while we develop... */
            secBase->_cfgLock = Lock("ETC:", ACCESS_READ);
            secBase->_pwdLock = Lock("ETC:", ACCESS_READ);
            bootstrapped = TRUE;
            /* Die(GetLocStr(secBase, MSG_NOMUFS), NULL); Maybe we should have this
             * somewhere in here in the final version ? */
        }
    }
    if (!bootstrapped)
    {
        if (!ReadKeyFiles(_Base)) {
            secBase->SecurityViolation = TRUE;
            Die(GetLocStr(secBase, MSG_BADKEYFILE), 0);
        }
    }
    ReleaseSemaphore(&secBase->VolumesSem);

    LoadConfig(_Base);
    ReadFSTab(secBase);

    CurrentDir(secBase->_pwdLock);		
    SetMem(&secBase->PasswdNotifyReq, 0, sizeof(secBase->PasswdNotifyReq));
    secBase->PasswdNotifyReq.nr_Name = (char *)PasswdFileName;
    secBase->PasswdNotifyReq.nr_Flags = NRF_SEND_SIGNAL;
    secBase->PasswdNotifyReq.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
    secBase->PasswdNotifyReq.nr_stuff.nr_Signal.nr_SignalNum = secBase->NotifySig;
    StartNotify(&secBase->PasswdNotifyReq);

    CurrentDir(secBase->_cfgLock);		
    SetMem(&secBase->GroupNotifyReq, 0, sizeof(secBase->GroupNotifyReq));
    secBase->GroupNotifyReq.nr_Name = (char *)GroupFileName;
    secBase->GroupNotifyReq.nr_Flags = NRF_SEND_SIGNAL;
    secBase->GroupNotifyReq.nr_stuff.nr_Signal.nr_Task = secBase->PasswdNotifyReq.nr_stuff.nr_Signal.nr_Task;
    secBase->GroupNotifyReq.nr_stuff.nr_Signal.nr_SignalNum = secBase->NotifySig;
    StartNotify(&secBase->GroupNotifyReq);

    return(TRUE);
}

/*
 *      Free all Volume Information
 */

void FreeVolumes(struct Library *_Base)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;
    struct secVolume *vollist, *vl2;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    FreeDefs();
    EndNotify(&secBase->GroupNotifyReq);
    EndNotify(&secBase->PasswdNotifyReq);
    if (secBase->_pwdLock) {
        UnLock(secBase->_pwdLock);
        secBase->_pwdLock = BNULL;
    }
    if (secBase->_cfgLock) {
        UnLock(secBase->_cfgLock);
        secBase->_cfgLock = BNULL;
    }
    ObtainSemaphore(&secBase->VolumesSem);
    vollist = secBase->Volumes;
    secBase->Volumes = NULL;
    while (vollist) {
        vl2 = vollist->Next;
        /* Preserve Enforced Volumes */
        if (vollist->FS_Flags) {
                vollist->Next = secBase->Volumes;
                secBase->Volumes = vollist;
        } else {
                Free(vollist, sizeof(struct secVolume));
        }
        vollist = vl2;
    }
    ReleaseSemaphore(&secBase->VolumesSem);
    PurgeKeyBuffer();
    FreeBuffer();
}
