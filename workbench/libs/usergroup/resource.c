/*
 * resource.c -- handle credential resource
 *
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include "base.h"
#include <exec/memory.h>
#include <proto/exec.h>

#include "credential.h"

#define VERSION 4
#define REVISION 1
#define VSTRING "credential.resource 4.1 (01.10.2025)"

/*
 * Create credential resource
 *
 * Note - this is mindless single-user-version
 */
struct CredentialResource *CredentialInit(const char *name)
{
    struct CredentialResource *res;
    ULONG ressize = sizeof(*res) + sizeof(VSTRING);

    D(bug("[UserGroup] %s()\n", __func__));

    res = AllocVec(ressize, MEMF_CLEAR|MEMF_PUBLIC);
    if (res) {
        D(bug("[UserGroup] %s: resource allocated @ 0x%p\n", __func__, res));

        CopyMem(CREDENTIALNAME, res->r_name, sizeof(CREDENTIALNAME));
        CopyMem(VSTRING, res->r_vstring, sizeof(VSTRING));

        res->r_Lib.lib_Node.ln_Type = NT_RESOURCE;
        res->r_Lib.lib_Node.ln_Name =  (STRPTR)res->r_name;
        res->r_Lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
        /* res->r_Lib.lib_NegSize = 0; */
        res->r_Lib.lib_PosSize = ressize;
        res->r_Lib.lib_Version = VERSION;
        res->r_Lib.lib_Revision = REVISION;
        res->r_Lib.lib_IdString = (APTR)res->r_vstring;
        /* res->r_Lib.lib_Sum = 0; */
        res->r_Lib.lib_OpenCnt = 1;

        res->r_proc->p_ucred = res->r_ucred;
        res->r_proc->p_cred->p_ngroups = 1;

        AddResource(res);
    }
    return res;
}

struct proc *procfind(pid_t pid)
{
    return CredentialBase->r_proc;
}

struct ucred *crcopy(struct ucred *cred)
{
    return cred;
}
