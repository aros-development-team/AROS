/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    envoyfs_imports.c - Envoy export discovery and login hashing, lifted from
    envoy/prefs/imports/imports.c (re/spec/efs-protocol.md §1.1, §1.6).

    Every entry point degrades gracefully when the Envoy libraries are not
    installed (the module registers its handler regardless - "show but
    degrade"): Imp_ListExports fails with ENVOYERR_NORESOURCES and
    Imp_MakeLogin returns FALSE.
*/

#include <proto/exec.h>
#include <proto/nipc.h>
#include <proto/services.h>
#include <proto/accounts.h>
#include <exec/memory.h>
#include <envoy/nipc.h>
#include <envoy/services.h>
#include <envoy/errors.h>
#include <stdio.h>
#include <string.h>

#include "envoyfs_intern.h"
#include "envoyfs_imports.h"
#include "locale.h"

#define CMD_LISTEXPORTS     4
#define LIST_REQSIZE        128
#define LIST_RESPSIZE       1024
#define LIST_TIMEOUT        5

CONST_STRPTR Imp_ErrorText(ULONG err, STRPTR buf, ULONG size)
{
    switch (err)
    {
    case ENVOYERR_UNKNOWNHOST:      return _(MSG_ENVOY_ERR_UNKNOWNHOST);
    case ENVOYERR_UNKNOWNENTITY:
    case ENVOYERR_UNKNOWNSERVICE:   return _(MSG_ENVOY_ERR_NOSERVICE);
    case ENVOYERR_OPENSERVICEFAIL:
    case ENVOYERR_BADSTARTSERVICE:  return _(MSG_ENVOY_ERR_NOSERVICELIB);
    case ENVOYERR_TIMEOUT:          return _(MSG_ENVOY_ERR_TIMEOUT);
    case ENVOYERR_NORESOLVER:
    case ENVOYERR_CANTDELIVER:
    case ENVOYERR_ABORTED:          return _(MSG_ENVOY_ERR_UNREACHABLE);
    case ENVOYERR_NORESOURCES:      return _(MSG_ENVOY_ERR_RESOURCES);
    case 0x8000:
    case 0x8001:                    return _(MSG_ENVOY_ERR_REFUSED);
    }
    snprintf(buf, size, "%s (%ld)", _(MSG_ENVOY_ERR_OTHER), (long)err);
    return buf;
}

void Imp_FreeList(struct List *list)
{
    struct Node *n;

    while ((n = RemHead(list)))
        FreeVec(n);
}

static void CopyField(STRPTR dst, ULONG size, CONST_STRPTR src)
{
    ULONG n = src ? strlen(src) : 0;

    if (n > size - 1)
        n = size - 1;
    if (n)
        CopyMem((APTR)src, dst, n);
    dst[n] = '\0';
}

ULONG Imp_ListExports(CONST_STRPTR host, CONST_STRPTR user, CONST_STRPTR password, struct List *list)
{
    struct Entity *me, *svc;
    struct Transaction *t;
    ULONG err = 0;
    UBYTE *req;

    if (!NIPCBase || !ServicesBase)
        return ENVOYERR_NORESOURCES;

    if (!(me = CreateEntity(ENT_AllocSignal, 0, TAG_DONE)))
        return ENVOYERR_NORESOURCES;
    svc = FindService((STRPTR)host, (STRPTR)"Filesystem", me, FSVC_Error, (IPTR)&err, TAG_DONE);
    if (!svc)
    {
        DeleteEntity(me);
        return err ? err : ENVOYERR_UNKNOWNSERVICE;
    }
    if (!(t = AllocTransaction(TRN_AllocReqBuffer, LIST_REQSIZE, TRN_AllocRespBuffer, LIST_RESPSIZE, TAG_DONE)))
        err = ENVOYERR_NORESOURCES;
    else
    {
        /* user name at 0, clear password at 64, each a C string in a 64-byte field */
        req = t->trans_RequestData;
        memset(req, 0, LIST_REQSIZE);
        CopyField((STRPTR)req, 64, user);
        CopyField((STRPTR)req + 64, 64, password);
        t->trans_Command = CMD_LISTEXPORTS;
        t->trans_ReqDataActual = LIST_REQSIZE;
        t->trans_Timeout = LIST_TIMEOUT;
        err = DoTransaction(svc, me, t);
        memset(req, 0, LIST_REQSIZE);           /* do not keep the password */
        if (!err)
        {
            ULONG off;
            for (off = 0; off + IMP_NAMELEN <= t->trans_RespDataActual; off += IMP_NAMELEN)
            {
                CONST_STRPTR name = (CONST_STRPTR)t->trans_ResponseData + off;
                ULONG len = strnlen(name, IMP_NAMELEN - 1);
                struct Node *n;

                if (!len || !(n = AllocVec(sizeof(struct Node) + len + 1, MEMF_CLEAR)))
                    continue;
                n->ln_Name = (char *)(n + 1);
                CopyMem((APTR)name, n->ln_Name, len);
                AddTail(list, n);
            }
        }
        FreeTransaction(t);
    }
    LoseService(svc);
    DeleteEntity(me);
    return err;
}

BOOL Imp_MakeLogin(struct ImpLogin *login, CONST_STRPTR user, CONST_STRPTR password)
{
    char hash[16];

    memset(login, 0, sizeof(*login));
    CopyField(login->User, sizeof(login->User), user);
    if (!AccountsBase)
        return FALSE;
    /* ECrypt lower-cases the user name itself (re/spec/services-accounts.md §7) */
    ECrypt((STRPTR)hash, (STRPTR)(password ? password : (CONST_STRPTR)""), (STRPTR)login->User);
    login->Hash[0] = '$';
    CopyField(login->Hash + 1, sizeof(login->Hash) - 1, hash);
    memset(hash, 0, sizeof(hash));
    return TRUE;
}

void Imp_DeviceName(STRPTR buf, ULONG size, CONST_STRPTR host, CONST_STRPTR export)
{
    CONST_STRPTR h = strrchr(host, ':');
    ULONG n = 0, len;

    h = h ? h + 1 : host;                       /* "Realm:host" -> "host" */
    while (*h && n < size - 2)
        buf[n++] = *h++;
    buf[n++] = '-';
    len = strlen(export);
    if (len && export[len - 1] == ':')
        len--;                                  /* one trailing ':' is removed */
    for (; len && n < size - 1; export++, len--)
        buf[n++] = (*export == ':') ? '_' : *export;
    buf[n] = '\0';
}
