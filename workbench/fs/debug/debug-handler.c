/*
 * Copyright (C) 2021, The AROS Development Team.  All rights reserved.
 * Author: Nick Andrews <kalamatee@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <string.h>

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#define UtilityBase     (pb->pb_UtilityBase)

#define OUTBUF_SIZE 1024

struct debugArgs {
    struct Node pa_Node;
    char outbuf[OUTBUF_SIZE];
};

struct debugBase {
    struct Library *pb_UtilityBase;
    struct ExecBase *pb_SysBase;
    struct List pb_Openers;
};

/* Open a handle on the debug-handler for IO */
static struct debugArgs *debugOpen(struct debugBase *pb, struct debugArgs *pa, BPTR name, SIPTR *err)
{
    struct ExecBase *SysBase = pb->pb_SysBase;
    int len = AROS_BSTR_strlen(name);

    if ((pa = AllocVec(sizeof(*pa) + len + 1, MEMF_ANY))) {
        pa->pa_Node.ln_Name = (APTR)(&pa[1]);
        CopyMem(AROS_BSTR_ADDR(name), pa->pa_Node.ln_Name, len);
        pa->pa_Node.ln_Name[len] = 0;
        AddTail(&pb->pb_Openers, &pa->pa_Node);
        *err = 0;
        return pa;
    }
    *err = ERROR_NO_DISK;
    return NULL;
}

static void debugClose(struct debugArgs *pa, struct ExecBase *SysBase)
{
    Remove(&pa->pa_Node);
    FreeVec(pa);
}

void replyPkt(struct DosPacket *dp, struct ExecBase *SysBase)
{
    struct MsgPort *mp = dp->dp_Port;
    struct Message *mn = dp->dp_Link;
    mn->mn_Node.ln_Name = (char*)dp;
    dp->dp_Port = &((struct Process*)FindTask(NULL))->pr_MsgPort;
    PutMsg(mp, mn);
}

static BOOL is_ascii(const signed char *c, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if(c[i] < 0) return FALSE;
  }
  return TRUE;
}

LONG debug_handler(struct ExecBase *SysBase)
{
    struct Process *thisProc = (struct Process *)FindTask(NULL);
    struct MsgPort *mp = &thisProc->pr_MsgPort;
    struct DosPacket *dp;
    struct debugBase pb = {};
    BOOL debugPipe = TRUE;

    pb.pb_SysBase = SysBase;

    NEWLIST(&pb.pb_Openers);

    WaitPort(mp);

    dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);

    if ((pb.pb_UtilityBase = OpenLibrary("utility.library", 0))) {
    }

    ((struct DeviceNode *)BADDR(dp->dp_Arg3))->dn_Task = mp;
    dp->dp_Res1 = DOSTRUE;

    do {
        struct debugArgs *pa;
        replyPkt(dp, SysBase);
        WaitPort(mp);
        dp = (struct DosPacket *)(GetMsg(mp)->mn_Node.ln_Name);
        switch (dp->dp_Type) {
        case ACTION_FINDINPUT:
        case ACTION_FINDOUTPUT:
        case ACTION_FINDUPDATE:
            pa = debugOpen(&pb, pa, (BSTR)dp->dp_Arg3, &dp->dp_Res2);
            if (dp->dp_Res2 == RETURN_OK) {
                struct FileHandle *fh = BADDR(dp->dp_Arg1);
                fh->fh_Arg1 = (SIPTR)pa;
                fh->fh_Type = mp;
            }
            dp->dp_Res1 = (dp->dp_Res2 == 0) ? DOSTRUE : DOSFALSE;
            break;
        case ACTION_READ:
            if ((pa = (struct debugArgs *)dp->dp_Arg1)) {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_READ_PROTECTED;
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_WRITE:
            if ((pa = (struct debugArgs *)dp->dp_Arg1)) {
                ULONG len = dp->dp_Arg3 > OUTBUF_SIZE - 1 ? OUTBUF_SIZE - 1 : dp->dp_Arg3;
                pa->outbuf[0] = '\0';
                if (is_ascii((char *)dp->dp_Arg2, len))
                {
                    CopyMem((APTR)dp->dp_Arg2, pa->outbuf, len);
                    pa->outbuf[len] = '\0';
                }
                else
                {
                    int i;
                    for (i = 0; i < (len >> 1); i ++)
                        pa->outbuf[i] = *(UBYTE *)(dp->dp_Arg2 + (i << 1) + 1);
                }
                kprintf("%s", pa->outbuf);
                dp->dp_Res1 = len;
                dp->dp_Res2 = 0;
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_END:
            if ((pa = (struct debugArgs *)dp->dp_Arg1)) {
                debugClose(pa, SysBase);
                dp->dp_Res1 = DOSTRUE;
                dp->dp_Res2 = 0;
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
            }
            break;
        case ACTION_DIE:
            if (IsListEmpty(&pb.pb_Openers)) {
                dp->dp_Res1 = DOSTRUE;
                dp->dp_Res2 = 0;
                debugPipe = FALSE;
            } else {
                dp->dp_Res1 = DOSFALSE;
                dp->dp_Res2 = ERROR_OBJECT_IN_USE;
            }
            break;
        default:
            dp->dp_Res1 = DOSFALSE;
            dp->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
            break;
        }
    } while (debugPipe);

    /* ACTION_DIE ends up here... */
    replyPkt(dp, SysBase);

    CloseLibrary(pb.pb_UtilityBase);

    return RETURN_OK;
}
