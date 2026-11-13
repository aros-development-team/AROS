/*
 * This file implements the runtime module function, LoadModule.
 */

//#define DEBUG 1
#include <aros/debug.h>

#include "dynmodule_modules.h"
#include <dos/dos.h>
#include <dos/dostags.h>

#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>

extern const char *DYNMODULE_EntryPoint_Sym;
extern const char *DYNMODULE_FindResource_Sym;
extern const char *DYNMODULE_LoadResource_Sym;
extern const char *DYNMODULE_FreeResource_Sym;

void *dynmoduleLoadModule(const char *modname, const char *port)
{
    void *mhandle;

    D(bug("[DynLink] %s('%s','%s')\n", __func__, modname, port));

    if ((mhandle = dynmodule__InternalLoadModule(modname, port, TRUE)) != NULL) {
        int (*EntryPFn)(void *, long, void *);
        EntryPFn = dynmoduleGetProcAddress(mhandle, DYNMODULE_EntryPoint_Sym);
        if (EntryPFn) {
            if (!EntryPFn(mhandle, 0, NULL)) {
                dynmoduleFreeModule(mhandle);
                mhandle = NULL;
            }
        }
    }
    return mhandle;
}

BOOL dynmodule__InternalHandleOpenMsg(__dynmoduleinstance_t *dynmod, int slotid, __dynmodulemsg_t *msg)
{
    D(bug("[DynLink] %s()\n", __func__));
    if (msg->IFOpenRequest.Error != DMIFERR_Ok) {
        D(bug("[DynLink] %s: Open error %08x\n", __func__, msg->IFOpenRequest.Error));
        dynmodule__InternalDestroyDynModEntry(dynmod, NULL);
        return FALSE;
    }

    if ((dynmod->dmi_FindResFunc = dynmoduleGetProcAddress(dynmod, DYNMODULE_FindResource_Sym)) != NULL)
        if ((dynmod->dmi_LoadResFunc = dynmoduleGetProcAddress(dynmod, DYNMODULE_LoadResource_Sym)) != NULL)
            if ((dynmod->dmi_FreeResFunc = dynmoduleGetProcAddress(dynmod, DYNMODULE_FreeResource_Sym)) != NULL)
                return TRUE;

    dynmoduleslots[slotid].mhandle = dynmod;
    dynmodule__InternalFreeModule(slotid);
    return FALSE;
}

BOOL dynmodule__InternalValidModuleFile(const char *modname)
{
    BPTR modf;
    if ((!modname) || !(modf = Open(modname, MODE_OLDFILE)))
        return FALSE;
    Close(modf);
    return TRUE;
}

void *dynmodule__InternalLoadModule(const char *modname, const char *port, BOOL registeropen)
{
    __dynmoduleinstance_t *dynmod;
    struct MsgPort *dmodport;
    struct MsgPort *replyport;
    __dynmodulemsg_t msg,*reply;
    int slotid;

    D(bug("[DynLink] %s('%s','%s')\n", __func__, modname, port));

    if (!DYNMODULE_Init(&dynmoduleslots,
        sizeof(__dynmoduleentry_t) * DYNMODULE_MAX,
        (stdexitfunc_t)dynmodule__InternalCleanup))
        return NULL;

    if (!dynmodule__InternalValidModuleFile(modname))
        return NULL;

    if (!port)
        port = modname;

    if ((dynmod = dynmodule__InternalOpenPortEntry(port, registeropen)) != NULL)
        return dynmod;

    D(bug("[DynLink] %s: first open ..\n", __func__));
    if (!dynmodule__InternalFindFreeSlot(&slotid))
        return NULL;
    D(bug("[DynLink] %s: using slot %u\n", __func__, slotid));

    if ((dynmod = dynmodule__InternalAllocDynModEntry(&replyport)) == NULL)
        return NULL;

    D(bug("[DynLink] %s: dynamicmodule instance @ 0x%p\n", __func__, dynmod));
    D(bug("[DynLink] %s:                 ifport @ 0x%p\n", __func__, replyport));

    if (!(dmodport = FindPort(port)))
        dmodport = dynmodule__InternalBootstrapDynMod(modname, port, 10);

    if (!dmodport) {
        dynmodule__InternalDestroyDynModEntry(dynmod, replyport);
        return NULL;
    }

    D(bug("[DynLink] %s: found port for '%s' @ 0x%p\n", __func__, port, dmodport));

    memset(&msg.Message, 0, sizeof(struct Message));
    msg.Message.mn_ReplyPort        = replyport;
    msg.IFMsgType                   = DMIFMSG_Open;
    msg.IFOpenRequest.Error         = DMIFERR_Ok;
    msg.IFOpenRequest.StackType     = DYNMOD_STACKF_DEFAULT;

    dynmod->dmi_StackFType          = DYNMOD_STACKF_DEFAULT;
    dynmod->dmi_IFMsgPort           = dmodport;
    PutMsg(dynmod->dmi_IFMsgPort, (struct Message *)&msg);

    WaitPort(replyport);

    if (((reply = (__dynmodulemsg_t *)GetMsg(replyport)) != NULL) &&
          (reply->IFMsgType == DMIFMSG_Open)) {
        D(bug("[DynLink] %s: Open reply @ 0x%p\n", __func__, reply));
        if (!dynmodule__InternalHandleOpenMsg(dynmod, slotid, reply)) {
            dynmod = NULL;
        }
    } else {
        D(bug("[DynLink] %s: no reply from IF port\n", __func__));
        dynmodule__InternalDestroyDynModEntry(dynmod, NULL);
        dynmod = NULL;
    }
    DeleteMsgPort(replyport);
    if (dynmod)
        dynmodule__InternalInitDynModEntry(slotid, dynmod, port);

    D(bug("[DynLink] %s: returning 0x%p\n", __func__, dynmod));

    return dynmod;
}
