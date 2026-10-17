/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - registered hardware authentication tokens
          (Security.server context unless noted)

          Each user's tokens are kept in SYS:Security/Keys/<user>, a file
          only root may read or write, one token per line:

            <type> <scheme> <label> <public id> <secret, hex> <counter> <yyyy-mm-dd>

          The file is rewritten like the password file (new file, size
          check, rename). What a code looks like and whether it is right
          is the business of the scheme's plugin (ID_PLUGIN_HWAUTH, e.g.
          "LOADPLUGIN yubikey" for the "yubico" scheme); the library keeps
          the records and requires every accepted code's counter to grow.
          Without the scheme's plugin a token can be neither registered
          nor checked.

          Authorisation (secKeyAdminA()): root may do anything; users may
          list their own tokens, and add or remove them once Security.auth
          has redeemed a ticket of theirs (req->Verified: the request then
          comes from the authentication process).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "security_intern.h"
#include "security_server.h"
#include "security_config.h"
#include "security_keys.h"
#include "security_memory.h"
#include "security_task.h"
#include "security_plugins.h"
#include "security_protect.h"
#include "security_support.h"

#define KEYS_DIRNAME    "Keys"
#define KEYLINE_MAX     256

/* ------------------------------------------------------------------------ */
/* The plugins: one per scheme                                              */

#define MAXSCHEMES  8

static struct plugin_hwauth_ops *schemes[MAXSCHEMES];

static CONST_STRPTR TypeName(ULONG type)
{
    return (type == secKEY_OTP) ? "OTP" : NULL;
}

static ULONG TypeFromName(CONST_STRPTR name)
{
    return !strcmp(name, "OTP") ? secKEY_OTP : 0;
}

ULONG RegisterHwAuthHandler(struct SecurityBase *secBase, struct plugin_ops *ops)
{
    struct plugin_hwauth_ops *h = (struct plugin_hwauth_ops *)ops;
    int i, slot = -1;

    if (!h->Scheme || !h->Scheme[0] || strlen(h->Scheme) >= secKEYSCHEMESIZE || !TypeName(h->Type) ||
        !h->CheckRegistration || !h->MatchCode || !h->VerifyCode)
        return secpiFALSE;
    for (i = 0; i < MAXSCHEMES; i++)
    {
        if (schemes[i] && schemes[i]->Type == h->Type && !Stricmp(schemes[i]->Scheme, h->Scheme))
            return secpiFALSE;
        if (!schemes[i] && slot < 0)
            slot = i;
    }
    if (slot < 0)
        return secpiFALSE;
    schemes[slot] = h;
    return secpiTRUE;
}

ULONG UnRegisterHwAuthHandler(struct SecurityBase *secBase, struct plugin_ops *ops)
{
    int i;

    for (i = 0; i < MAXSCHEMES; i++)
    {
        if (schemes[i] == (struct plugin_hwauth_ops *)ops)
        {
            schemes[i] = NULL;
            return secpiTRUE;
        }
    }
    return secpiFALSE;
}

struct plugin_hwauth_ops *KeyScheme(struct SecurityBase *secBase, ULONG type, CONST_STRPTR scheme)
{
    int i;

    if (!scheme)
        return NULL;
    for (i = 0; i < MAXSCHEMES; i++)
        if (schemes[i] && schemes[i]->Type == type && !Stricmp(schemes[i]->Scheme, scheme))
            return schemes[i];
    return NULL;
}

ULONG KeyMatchCode(struct SecurityBase *secBase, const struct secKeyRec *rec, CONST_STRPTR typed, ULONG *codelen)
{
    struct plugin_hwauth_ops *h = KeyScheme(secBase, rec->Type, rec->Scheme);

    if (!h)
        return secpiNOTSUPP;
    UseModule(secBase, h->ops.module);
    *codelen = h->MatchCode(rec->PublicID, typed);
    ReleaseModule(secBase, h->ops.module);
    return *codelen ? secpiTRUE : secpiFALSE;
}

ULONG KeyVerifyCode(struct SecurityBase *secBase, const struct secKeyRec *rec, CONST_STRPTR code, ULONG *counter)
{
    struct plugin_hwauth_ops *h = KeyScheme(secBase, rec->Type, rec->Scheme);
    ULONG res;

    if (!h)
        return secpiNOTSUPP;
    UseModule(secBase, h->ops.module);
    res = h->VerifyCode(rec->PublicID, rec->Secret, rec->SecretLen, code, counter);
    ReleaseModule(secBase, h->ops.module);
    return res;
}

/* ------------------------------------------------------------------------ */
/* Helpers                                                                  */

static int HexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* exactly 2n hex digits -> n bytes */
static BOOL HexDecode(CONST_STRPTR s, UBYTE *out, ULONG n)
{
    ULONG i;

    if (!s || strlen(s) != 2 * n)
        return FALSE;
    for (i = 0; i < n; i++)
    {
        int hi = HexValue(s[2 * i]), lo = HexValue(s[2 * i + 1]);

        if (hi < 0 || lo < 0)
            return FALSE;
        out[i] = (UBYTE)((hi << 4) | lo);
    }
    return TRUE;
}

static void HexEncode(const UBYTE *in, ULONG n, STRPTR out)
{
    static const char digits[] = "0123456789abcdef";
    ULONG i;

    for (i = 0; i < n; i++)
    {
        out[2 * i] = digits[in[i] >> 4];
        out[2 * i + 1] = digits[in[i] & 15];
    }
    out[2 * n] = '\0';
}

/* A public id: 1..35 printable characters, no spaces (the scheme checks the rest) */
static BOOL PublicIDOK(CONST_STRPTR s)
{
    ULONG n, i;

    if (!s)
        return FALSE;
    n = strlen(s);
    if (n < 1 || n >= secKEYPUBIDSIZE)
        return FALSE;
    for (i = 0; i < n; i++)
        if ((UBYTE)s[i] <= ' ' || s[i] == 0x7f)
            return FALSE;
    return TRUE;
}

/* A secret: 2..2*secKEYSECRETMAX hex digits, an even number; its length in *len */
static BOOL SecretDecode(CONST_STRPTR s, UBYTE *out, ULONG *len)
{
    ULONG n;

    if (!s)
        return FALSE;
    n = strlen(s);
    if (n < 2 || n > 2 * secKEYSECRETMAX || (n & 1))
        return FALSE;
    *len = n / 2;
    return HexDecode(s, out, *len);
}

/* A label: 1..31 printable characters, no spaces */
static BOOL LabelOK(CONST_STRPTR s)
{
    ULONG n, i;

    if (!s)
        return FALSE;
    n = strlen(s);
    if (n < 1 || n >= secKEYLABELSIZE)
        return FALSE;
    for (i = 0; i < n; i++)
        if ((UBYTE)s[i] <= ' ' || s[i] == 0x7f)
            return FALSE;
    return TRUE;
}

static void PutNum(STRPTR out, ULONG v, int width)
{
    int i;

    for (i = width - 1; i >= 0; i--)
    {
        out[i] = '0' + (v % 10);
        v /= 10;
    }
}

/* Today as yyyy-mm-dd (from DateStamp(): days since 1978-01-01) */
static void TodayISO(struct SecurityBase *secBase, STRPTR out)
{
    struct DateStamp ds;
    ULONG z, era, doe, yoe, doy, mp, y, m, d;

    DateStamp(&ds);
    /* days since 1970-01-01 is ds_Days + 2922; then Howard Hinnant's civil_from_days */
    z = ds.ds_Days + 2922 + 719468;
    era = z / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    if (m <= 2)
        y++;
    PutNum(out, y, 4);
    out[4] = '-';
    PutNum(out + 5, m, 2);
    out[7] = '-';
    PutNum(out + 8, d, 2);
    out[10] = '\0';
}

/* ------------------------------------------------------------------------ */
/* The files                                                                */

/* The Keys directory under the configuration directory, created (root
 * only) when missing; a lock on it, or BNULL. Server context. */
static BPTR KeysDirLock(struct SecurityBase *secBase, BOOL create)
{
    BPTR olddir, lock;

    if (!secBase->_cfgLock)
        return BNULL;
    olddir = CurrentDir(secBase->_cfgLock);
    if (!(lock = Lock(KEYS_DIRNAME, SHARED_LOCK)) && create)
    {
        if ((lock = CreateDir(KEYS_DIRNAME)))
        {
            UnLock(lock);
            ProtectRootOnly(secBase, KEYS_DIRNAME);
            lock = Lock(KEYS_DIRNAME, SHARED_LOCK);
        }
    }
    CurrentDir(olddir);
    return lock;
}

void FreeKeys(struct secKeyRec *list)
{
    while (list)
    {
        struct secKeyRec *next = list->Next;

        memset(list, 0, sizeof(*list));
        Free(list, sizeof(*list));
        list = next;
    }
}

/* Split a line into words; FALSE if there are not exactly n */
static BOOL Words(STRPTR line, STRPTR *w, ULONG n)
{
    ULONG i = 0;

    while (*line)
    {
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0';
        if (!*line)
            break;
        if (i == n)
            return FALSE;
        w[i++] = line;
        while (*line && *line != ' ' && *line != '\t' && *line != '\n')
            line++;
    }
    return i == n;
}

static struct secKeyRec *ParseKeyLine(struct SecurityBase *secBase, STRPTR line)
{
    STRPTR w[7];
    struct secKeyRec *rec;
    LONG counter;
    ULONG type;

    if (!Words(line, w, 7) || !(type = TypeFromName(w[0])))
        return NULL;
    if (strlen(w[1]) >= secKEYSCHEMESIZE || !LabelOK(w[2]) || !PublicIDOK(w[3]) || StrToLong(w[5], &counter) < 0 ||
        counter < 0 || strlen(w[6]) >= sizeof(rec->Registered))
        return NULL;
    if (!(rec = MAlloc(sizeof(*rec))))
        return NULL;
    memset(rec, 0, sizeof(*rec));
    rec->Type = type;
    strcpy(rec->Scheme, w[1]);
    strcpy(rec->Label, w[2]);
    strcpy(rec->PublicID, w[3]);
    if (!SecretDecode(w[4], rec->Secret, &rec->SecretLen))
    {
        FreeKeys(rec);
        return NULL;
    }
    rec->Counter = (ULONG)counter;
    strcpy(rec->Registered, w[6]);
    return rec;
}

struct secKeyRec *LoadKeys(struct SecurityBase *secBase, CONST_STRPTR userid)
{
    struct secKeyRec *list = NULL, *last = NULL, *rec;
    char line[KEYLINE_MAX];
    BPTR dir, olddir, file;

    if (!userid || !userid[0] || strchr(userid, '/') || strchr(userid, ':'))
        return NULL;
    if (!(dir = KeysDirLock(secBase, FALSE)))
        return NULL;
    olddir = CurrentDir(dir);
    if ((file = Open(userid, MODE_OLDFILE)))
    {
        while (FGets(file, line, sizeof(line) - 1))
        {
            if (!line[0] || line[0] == '\n' || line[0] == ';' || line[0] == '#')
                continue;
            if ((rec = ParseKeyLine(secBase, line)))
            {
                if (last)
                    last->Next = rec;
                else
                    list = rec;
                last = rec;
            }
            else
            {
                SIPTR args[1] = { (SIPTR)userid };
                VLogF(secBase, LOGF_Flag_Type_Warn, SECLOG_SUB_SERVER, "a key of '%s' is malformed and ignored", args);
            }
        }
        Close(file);
    }
    memset(line, 0, sizeof(line));
    CurrentDir(olddir);
    UnLock(dir);
    return list;
}

BOOL UserHasKeys(struct SecurityBase *secBase, CONST_STRPTR userid)
{
    struct secKeyRec *list = LoadKeys(secBase, userid);

    FreeKeys(list);
    return list != NULL;
}

static LONG WriteKeys(struct SecurityBase *secBase, BPTR file, APTR ctx)
{
    struct secKeyRec *rec;
    char secret[2 * secKEYSECRETMAX + 1];
    LONG written = 0, len;
    SIPTR args[7];

    for (rec = (struct secKeyRec *)ctx; rec; rec = rec->Next)
    {
        HexEncode(rec->Secret, rec->SecretLen, secret);
        args[0] = (SIPTR)TypeName(rec->Type);
        args[1] = (SIPTR)rec->Scheme;
        args[2] = (SIPTR)rec->Label;
        args[3] = (SIPTR)rec->PublicID;
        args[4] = (SIPTR)secret;
        args[5] = (SIPTR)rec->Counter;
        args[6] = (SIPTR)rec->Registered;
        len = VFPrintf(file, "%s %s %s %s %s %iu %s\n", (RAWARG)args);
        memset(secret, 0, sizeof(secret));
        if (len < 0)
            return -1;
        written += len;
    }
    return written;
}

/* The user's key file from the list; an empty list removes the file */
static BOOL SaveKeys(struct SecurityBase *secBase, CONST_STRPTR userid, struct secKeyRec *list)
{
    char newname[secUSERIDSIZE + 8], oldname[secUSERIDSIZE + 8];
    BPTR dir, olddir;
    BOOL res;

    if (!(dir = KeysDirLock(secBase, TRUE)))
        return FALSE;
    if (!list)
    {
        olddir = CurrentDir(dir);
        res = DeleteFile(userid) || IoErr() == ERROR_OBJECT_NOT_FOUND;
        CurrentDir(olddir);
        UnLock(dir);
        return res;
    }
    strcpy(newname, userid);
    strcat(newname, ".new");
    strcpy(oldname, userid);
    strcat(oldname, ".old");
    res = RewriteDbFile(secBase, dir, userid, newname, oldname, FALSE, WriteKeys, list);
    UnLock(dir);
    return res;
}

/* Security.auth accepted an OTP: remember its counter */
BOOL UpdateKeyCounter(struct SecurityBase *secBase, CONST_STRPTR userid, CONST_STRPTR publicid, ULONG counter)
{
    struct secKeyRec *list = LoadKeys(secBase, userid), *rec;
    BOOL res = FALSE;

    for (rec = list; rec; rec = rec->Next)
    {
        if (!strcmp(rec->PublicID, publicid))
        {
            rec->Counter = counter;
            res = SaveKeys(secBase, userid, list);
            break;
        }
    }
    FreeKeys(list);
    return res;
}

/* ------------------------------------------------------------------------ */
/* secKeyAdminA()                                                           */

ULONG KeyAdmin(struct SecurityBase *secBase, struct Task *client, struct secKARequest *req)
{
    struct secUserDef *def;
    struct secKeyRec *list, *rec, *prev, *nd;
    UWORD caller;
    BOOL isroot, self;
    ULONG res;

    if (!secBase->Configured || !GetUserDefs(secBase))
        return secKAERR_UNAVAILABLE;
    if (!req || !req->UserID || !req->UserID[0] || !client)
        return secKAERR_BADARGS;

    for (def = GetUserDefs(secBase); def; def = def->Next)
        if (!strcmp(def->UserID, req->UserID))
            break;
    if (!def)
        return secKAERR_NOTFOUND;

    /* root, or the user in person: listing freely, changing only through
     * Security.auth, which has seen their ticket */
    if (client == (struct Task *)secBase->Auth)
    {
        caller = req->CallerUid;
        isroot = (caller == secROOT_UID);
        self = req->Verified && (caller == def->uid);
    }
    else
    {
        caller = (UWORD)(GetTaskOwner(secBase, client) >> 16);
        isroot = (caller == secROOT_UID);
        self = FALSE;
    }
    if (!isroot && !self && !(req->Operation == secKA_LIST && caller == def->uid))
        return secKAERR_NOTALLOWED;

    list = LoadKeys(secBase, req->UserID);

    switch (req->Operation)
    {
    case secKA_LIST:
    {
        ULONG n = 0;

        for (rec = list; rec; rec = rec->Next)
        {
            if (req->List && n < req->ListSize)
            {
                struct secKeyInfo *ki = &req->List[n];

                memset(ki, 0, sizeof(*ki));
                ki->Type = rec->Type;
                strcpy(ki->Scheme, rec->Scheme);
                strcpy(ki->Label, rec->Label);
                strcpy(ki->PublicID, rec->PublicID);
                ki->Counter = rec->Counter;
                strcpy(ki->Registered, rec->Registered);
            }
            n++;
        }
        req->Count = n;
        res = secKAERR_OK;
        break;
    }

    case secKA_ADD:
    {
        struct plugin_hwauth_ops *h;
        UBYTE secret[secKEYSECRETMAX];
        ULONG secretlen;

        if (!TypeName(req->Type) || !req->Scheme || strlen(req->Scheme) >= secKEYSCHEMESIZE || !LabelOK(req->Label) ||
            !PublicIDOK(req->PublicID) || !SecretDecode(req->SecretHex, secret, &secretlen))
        {
            res = secKAERR_BADARGS;
            break;
        }
        if (!(h = KeyScheme(secBase, req->Type, req->Scheme)))
        {
            res = secKAERR_UNAVAILABLE;
            break;
        }
        UseModule(secBase, h->ops.module);
        res = h->CheckRegistration(req->PublicID, secret, secretlen);
        ReleaseModule(secBase, h->ops.module);
        if (res != secpiTRUE)
        {
            res = secKAERR_BADARGS;
            break;
        }
        for (rec = list; rec; rec = rec->Next)
        {
            if (!Stricmp(rec->Label, req->Label) || !strcmp(rec->PublicID, req->PublicID))
                break;
        }
        if (rec)
        {
            res = secKAERR_EXISTS;
            break;
        }
        if (!(nd = MAlloc(sizeof(*nd))))
        {
            res = secKAERR_WRITE;
            break;
        }
        memset(nd, 0, sizeof(*nd));
        nd->Type = req->Type;
        strcpy(nd->Scheme, h->Scheme);
        strcpy(nd->Label, req->Label);
        strcpy(nd->PublicID, req->PublicID);
        CopyMem(secret, nd->Secret, secretlen);
        nd->SecretLen = secretlen;
        memset(secret, 0, sizeof(secret));
        TodayISO(secBase, nd->Registered);
        for (prev = list; prev && prev->Next; prev = prev->Next)
            ;
        if (prev)
            prev->Next = nd;
        else
            list = nd;
        res = SaveKeys(secBase, req->UserID, list) ? secKAERR_OK : secKAERR_WRITE;
        if (res == secKAERR_OK)
        {
            SIPTR args[3] = { (SIPTR)req->Label, (SIPTR)req->UserID, (SIPTR)(isroot ? "root" : "the user") };
            VLogF(secBase, LOGF_Flag_Type_Information, SECLOG_SUB_SERVER, "security key '%s' registered for '%s' by %s", args);
        }
        break;
    }

    case secKA_REMOVE:
        if (!LabelOK(req->Label))
        {
            res = secKAERR_BADARGS;
            break;
        }
        for (prev = NULL, rec = list; rec && Stricmp(rec->Label, req->Label); prev = rec, rec = rec->Next)
            ;
        if (!rec)
        {
            res = secKAERR_NOTFOUND;
            break;
        }
        if (prev)
            prev->Next = rec->Next;
        else
            list = rec->Next;
        rec->Next = NULL;
        FreeKeys(rec);
        res = SaveKeys(secBase, req->UserID, list) ? secKAERR_OK : secKAERR_WRITE;
        if (res == secKAERR_OK)
        {
            SIPTR args[3] = { (SIPTR)req->Label, (SIPTR)req->UserID, (SIPTR)(isroot ? "root" : "the user") };
            VLogF(secBase, LOGF_Flag_Type_Information, SECLOG_SUB_SERVER, "security key '%s' of '%s' removed by %s", args);
        }
        break;

    default:
        res = secKAERR_BADARGS;
        break;
    }

    FreeKeys(list);
    return res;
}
