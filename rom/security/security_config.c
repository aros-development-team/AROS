/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library configuration, key files and the user/group
          database. Derived from MultiUser Config.c (c) Geert Uytterhoeven.

          All functions here run in the context of the server process.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_auth.h"
#include "security_plugins.h"
#include "security_crypto.h"
#include "security_enforce.h"
#include "security_memory.h"
#include "security_support.h"
#include "security_protect.h"

const char KeyFileName[]    = secKey_FileName;
const char PasswdFileName[] = secPasswd_FileName;
const char ConfigFileName[] = secConfig_FileName;
const char GroupFileName[]  = secGroup_FileName;
const char LogFileName[]    = secLog_FileName;

/*
 * Convert a MuFS era format string ("%ld" with 32-bit longs) into one that
 * fetches IPTR sized values ("%id"), so that SIPTR argument arrays work on
 * 64-bit systems.
 */
void FixFormat(CONST_STRPTR src, STRPTR dst, ULONG dstsize)
{
    ULONG o = 0;

    while (*src && o < dstsize - 1)
    {
        if (src[0] == '%' && src[1] == 'l' && (src[2] == 'd' || src[2] == 'u' || src[2] == 'x' || src[2] == 'X' || src[2] == 'c'))
        {
            if (o + 3 >= dstsize - 1)
                break;
            dst[o++] = '%';
            dst[o++] = 'i';
            dst[o++] = src[2];
            src += 3;
        }
        else if (src[0] == '%' && src[1] == '%')
        {
            if (o + 2 >= dstsize - 1)
                break;
            dst[o++] = '%';
            dst[o++] = '%';
            src += 2;
        }
        else
            dst[o++] = *src++;
    }
    dst[o] = '\0';
}

/*
 * General purpose line buffer
 */
BOOL ClearBuffer(struct SecurityBase *secBase)
{
    if (!secBase->Buffer && !(secBase->Buffer = MAlloc(secGENBUFSIZE)))
    {
        Die(secBase, NULL, AN_Unknown | AG_NoMemory);
        return FALSE;
    }
    memset(secBase->Buffer, 0, secGENBUFSIZE);
    return TRUE;
}

void FreeBuffer(struct SecurityBase *secBase)
{
    if (secBase->Buffer)
    {
        Free(secBase->Buffer, secGENBUFSIZE);
        secBase->Buffer = NULL;
    }
}

void PurgeKeyBuffer(struct SecurityBase *secBase)
{
    memset(secBase->Key, 0, sizeof(secBase->Key));
}

/*
 * Parse a User Entry:  UserID|Password|uid|gid|UserName|HomeDir|Shell
 */
static struct secUserDef *ParseUserLine(struct SecurityBase *secBase, STRPTR line, ULONG linenum)
{
    int i, j, len;
    LONG uid = 0, gid = 0;
    struct secUserDef *def;
    STRPTR part[7];
    STRPTR ptr;
    ULONG pwdlen;

#define UPART_USERID            0
#define UPART_PASSWORD          1
#define UPART_UID               2
#define UPART_GID               3
#define UPART_USERNAME          4
#define UPART_HOMEDIR           5
#define UPART_SHELL             6

    i = 0;
    for (j = 0; j < 7; j++)
    {
        part[j] = &line[i];
        while ((line[i]) && (line[i] != '\n') && (line[i] != '|'))
            i++;
        if (j == 6)
        {
            if (line[i] && (line[i] != '\n'))
                goto Fail;
        }
        else if (line[i] != '|')
            goto Fail;
        line[i++] = '\0';
        len = strlen(part[j]);
        switch (j)
        {
        case UPART_USERID:
            if (!len || len >= secUSERIDSIZE)
                goto Fail;
            break;

        case UPART_PASSWORD:
            /* Any hash the crypto layer understands; empty = no password;
             * a leading '!' locks the account (secAuthControlA()) */
            if (part[j][0] == '!' && len > 1 && !IsValidPasswordHash(secBase, part[j] + 1))
                goto Fail;
            if (part[j][0] != '!' && len && !IsValidPasswordHash(secBase, part[j]))
                goto Fail;
            break;

        case UPART_UID:
            if (!len || (StrToLong(part[j], &uid) == -1) || (uid < 0) || (uid > 65535))
                goto Fail;
            break;

        case UPART_GID:
            if (!len || (StrToLong(part[j], &gid) == -1) || (gid < 0) || (gid > 65535))
                goto Fail;
            break;

        case UPART_USERNAME:
            if (len >= secUSERNAMESIZE)
                goto Fail;
            break;
        case UPART_HOMEDIR:
            if (len >= secHOMEDIRSIZE)
                goto Fail;
            break;
        case UPART_SHELL:
            if (len >= secSHELLSIZE)
                goto Fail;
            break;
        }
    }

    pwdlen = MaxPwdLen(secBase);
    if (strlen(part[UPART_PASSWORD]) > pwdlen)
        pwdlen = strlen(part[UPART_PASSWORD]);

    if ((def = (struct secUserDef *)MAllocV(sizeof(struct secUserDef) + strlen(part[UPART_USERID]) +
                    strlen(part[UPART_USERNAME]) + strlen(part[UPART_HOMEDIR]) +
                    strlen(part[UPART_SHELL]) + (pwdlen + 2) + 4)))
    {
        ptr = &((STRPTR)def)[sizeof(struct secUserDef)];
        def->UserID = ptr;
        strcpy(ptr, part[UPART_USERID]);
        ptr = &ptr[strlen(part[UPART_USERID]) + 1];
        /* room for the hash and a '!' that locks the account */
        def->Password = ptr;
        strcpy(ptr, part[UPART_PASSWORD]);
        ptr = &ptr[pwdlen + 2];
        def->uid = (UWORD)uid;
        def->gid = (UWORD)gid;
        def->UserName = ptr;
        strcpy(ptr, part[UPART_USERNAME]);
        ptr = &ptr[strlen(part[UPART_USERNAME]) + 1];
        def->HomeDir = ptr;
        strcpy(ptr, part[UPART_HOMEDIR]);
        ptr = &ptr[strlen(part[UPART_HOMEDIR]) + 1];
        def->Shell = ptr;
        strcpy(ptr, part[UPART_SHELL]);
    }
    else
        Die(secBase, NULL, AN_Unknown | AG_NoMemory);
    return def;

Fail:
    Warn1(secBase, GetLocStr(secBase, MSG_BADENTRY_PASSWD), linenum);
    return NULL;
}

/*
 * Parse a Group Entry:  GroupID|gid|MgrUid|GroupName
 */
static struct secGroupDef *ParseGroupLine(struct SecurityBase *secBase, STRPTR line, ULONG linenum)
{
    int i, j, len;
    LONG gid = 0, mgruid = 0;
    struct secGroupDef *def;
    STRPTR part[4];
    STRPTR ptr;

#define GPART_GROUPID   0
#define GPART_GID       1
#define GPART_MGRUID    2
#define GPART_GROUPNAME 3

    i = 0;
    for (j = 0; j < 4; j++)
    {
        part[j] = &line[i];
        while ((line[i]) && (line[i] != '\n') && (line[i] != '|'))
            i++;
        if (j == 3)
        {
            if (line[i] && (line[i] != '\n'))
                goto Fail;
        }
        else if (line[i] != '|')
            goto Fail;
        line[i++] = '\0';
        len = strlen(part[j]);
        switch (j)
        {
        case GPART_GROUPID:
            if (!len || len >= secGROUPIDSIZE)
                goto Fail;
            break;
        case GPART_GID:
            if (!len || (StrToLong(part[j], &gid) == -1) || (gid < 0) || (gid > 65535))
                goto Fail;
            break;
        case GPART_MGRUID:
            if (!len || (StrToLong(part[j], &mgruid) == -1) || (mgruid < 0) || (mgruid > 65535))
                goto Fail;
            break;
        case GPART_GROUPNAME:
            if (len >= secGROUPNAMESIZE)
                goto Fail;
            break;
        }
    }

    if ((def = (struct secGroupDef *)MAllocV(sizeof(struct secGroupDef) + strlen(part[GPART_GROUPID]) +
                    strlen(part[GPART_GROUPNAME]) + 2)))
    {
        ptr = &((STRPTR)def)[sizeof(struct secGroupDef)];
        def->GroupID = ptr;
        strcpy(ptr, part[GPART_GROUPID]);
        ptr = &ptr[strlen(part[GPART_GROUPID]) + 1];
        def->gid = (UWORD)gid;
        def->MgrUid = (UWORD)mgruid;
        def->GroupName = ptr;
        strcpy(ptr, part[GPART_GROUPNAME]);
    }
    else
        Die(secBase, NULL, AN_Unknown | AG_NoMemory);
    return def;

Fail:
    Warn1(secBase, GetLocStr(secBase, MSG_BADENTRY_GROUP), linenum);
    return NULL;
}

/*
 * Parse a Relation Entry:  uid:gid[,gid...]
 */
static void ParseRelationLine(struct SecurityBase *secBase, STRPTR line, ULONG linenum)
{
    struct secUserDef *def;
    UWORD *groups;
    ULONG numgroups = 0;
    LONG uid, gid, len;
    ULONG i, j;

    if (((len = StrToLong(line, &uid)) == -1) || (uid < 0) || (uid > 65535))
        goto Fail;
    i = len;
    j = i + 1;
    if (line[i] != ':')
        goto Fail;
    for (def = secBase->UserDefs; def && (def->uid != uid); def = def->Next);
    if (!def)
        goto Fail;

    do
    {
        if (((len = StrToLong(&line[++i], &gid)) == -1) || (gid < 0) || (gid > 65535))
            goto Fail;
        i += len;
        numgroups++;
        if (line[i] && (line[i] != ',') && (line[i] != '\n'))
            goto Fail;
    } while (line[i] && (line[i] != '\n'));

    if (def->NumSecGroups + numgroups > 65535)
    {
        Warn2(secBase, GetLocStr(secBase, MSG_TOOMANYSECGROUPS), uid, linenum);
        return;
    }

    if (!(groups = MAlloc((numgroups + def->NumSecGroups) * sizeof(UWORD))))
    {
        Die(secBase, NULL, AN_Unknown | AG_NoMemory);
        return;
    }
    if (def->NumSecGroups)
    {
        CopyMem(def->SecGroups, groups, def->NumSecGroups * sizeof(UWORD));
        Free(def->SecGroups, def->NumSecGroups * sizeof(UWORD));
    }
    def->SecGroups = groups;
    groups += def->NumSecGroups;
    def->NumSecGroups += numgroups;
    for (i = 0; i < numgroups; i++)
    {
        j += StrToLong(&line[j], &gid) + 1;
        groups[i] = gid;
    }
    return;

Fail:
    Warn1(secBase, GetLocStr(secBase, MSG_BADENTRY_GROUP), linenum);
}

/*
 * Initialise the User and Group Definitions
 */
static void InitDefs(struct SecurityBase *secBase)
{
    BPTR file, olddir;
    ULONG linenum;
    char *Buffer;

    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    if (!secBase->_pwdLock || !ClearBuffer(secBase))
        return;
    Buffer = secBase->Buffer;

    olddir = CurrentDir(secBase->_pwdLock);
    RecoverPasswdFile(secBase);
    if ((file = Open(PasswdFileName, MODE_OLDFILE)))
    {
        struct secUserDef *def1, *def2 = NULL;

        for (linenum = 1; FGets(file, Buffer, secGENBUFSIZE - 1); linenum++)
            if (Buffer[0] && (Buffer[0] != '\n'))
                if ((def1 = ParseUserLine(secBase, Buffer, linenum)))
                {
                    if (def2)
                        def2->Next = def1;
                    else
                        secBase->UserDefs = def1;
                    def2 = def1;
                }
        Close(file);
    }

    if (secBase->_cfgLock)
        CurrentDir(secBase->_cfgLock);

    RecoverGroupFile(secBase);
    if (secBase->UserDefs && (file = Open(GroupFileName, MODE_OLDFILE)))
    {
        struct secGroupDef *def1, *def2 = NULL;

        for (linenum = 1; FGets(file, Buffer, secGENBUFSIZE - 1) && (Buffer[0] != '\n'); linenum++)
            if (Buffer[0])
                if ((def1 = ParseGroupLine(secBase, Buffer, linenum)))
                {
                    if (def2)
                        def2->Next = def1;
                    else
                        secBase->GroupDefs = def1;
                    def2 = def1;
                }
        if (Buffer[0] == '\n')
            for (linenum++; FGets(file, Buffer, secGENBUFSIZE - 1); linenum++)
                if (Buffer[0] && (Buffer[0] != '\n'))
                    ParseRelationLine(secBase, Buffer, linenum);
        Close(file);
    }
    CurrentDir(olddir);

    ClearBuffer(secBase);

    if (!secBase->UserDefs || !secBase->GroupDefs)
        FreeDefs(secBase);

    secBase->Configured = (secBase->UserDefs != NULL);
    D(bug(DEBUG_NAME_STR " %s: system is %sconfigured\n", __func__, secBase->Configured ? "" : "NOT ");)

    CheckDatabaseProtection(secBase);
}

/* A user definition from a password file line (modified in place); for
 * secUserAdminA(), which has checked the fields already */
struct secUserDef *MakeUserDef(struct SecurityBase *secBase, STRPTR line)
{
    return ParseUserLine(secBase, line, 0);
}

void FreeDefs(struct SecurityBase *secBase)
{
    struct secUserDef *udef = secBase->UserDefs;
    struct secGroupDef *gdef = secBase->GroupDefs;
    APTR p;

    while (udef)
    {
        p = udef->Next;
        if (udef->NumSecGroups)
            Free(udef->SecGroups, udef->NumSecGroups * sizeof(UWORD));
        FreeV(udef);
        udef = p;
    }
    secBase->UserDefs = NULL;
    while (gdef)
    {
        p = gdef->Next;
        FreeV(gdef);
        gdef = p;
    }
    secBase->GroupDefs = NULL;
}

struct secUserDef *GetUserDefs(struct SecurityBase *secBase)
{
    if (!secBase->UserDefs)
        InitDefs(secBase);
    return secBase->UserDefs;
}

struct secGroupDef *GetGroupDefs(struct SecurityBase *secBase)
{
    if (!secBase->GroupDefs)
        InitDefs(secBase);
    return secBase->GroupDefs;
}

/*
 * Key files
 */
static void RemTerminatingLF(char *buffer)
{
    int i = strlen(buffer);
    if (i && (buffer[i - 1] == '\n'))
        buffer[i - 1] = '\0';
}

/* Read a directory name from the key file and lock it on the given handler */
static BOOL ParseDirLockLine(struct SecurityBase *secBase, struct MsgPort *fs, BPTR file, BPTR *dir)
{
    char *Buffer = secBase->Buffer;
    BOOL res = FALSE;

    if (FGets(file, Buffer + 1, secGENBUFSIZE - 2))
    {
        res = TRUE;
        RemTerminatingLF(Buffer + 1);
        if (Buffer[1])
        {
            if (*dir)
                res = FALSE;    /* two volumes claim the directory */
            else
            {
                Buffer[0] = strlen(Buffer + 1);
                *dir = (BPTR)DoPkt(fs, ACTION_LOCATE_OBJECT, 0, (SIPTR)MKBADDR(Buffer), ACCESS_READ, 0, 0);
                res = (*dir != BNULL);
            }
        }
    }
    return res;
}

/* 'Safe' FGets: prevents synchronisation problems on startup */
static STRPTR SafeFGets(struct SecurityBase *secBase, BPTR fh, STRPTR buf, ULONG len)
{
    STRPTR res;
    int i;

    if (!(res = FGets(fh, buf, len)))
        for (i = 1; !res && (i < 10); i++)
        {
            Delay(25);
            res = FGets(fh, buf, len);
        }
    return res;
}

static LONG ReadKeyFile(struct SecurityBase *secBase, struct MsgPort *fs)
{
    BPTR dir, file, olddir;
    char buffer[secPASSWORDSIZE];
    LONG res = KEYFILE_NONE;
    char *Buffer = secBase->Buffer;
    /* BSTR ":" */
    UBYTE rootname[4] = { 1, ':', 0, 0 };

    dir = (BPTR)DoPkt(fs, ACTION_LOCATE_OBJECT, 0, (SIPTR)MKBADDR(rootname), ACCESS_READ, 0, 0);
    if (dir)
    {
        olddir = CurrentDir(dir);
        if ((file = Open(KeyFileName, MODE_OLDFILE)))
        {
            res = KEYFILE_BAD;
            if (SafeFGets(secBase, file, Buffer, secGENBUFSIZE - 1))
            {
                RemTerminatingLF(Buffer);
                if (Encrypt(buffer, Buffer, "Alpha, PowerPC or R4400?"))
                {
                    res = KEYFILE_OK;
                    if (secBase->Key[0])
                        res = strcmp(secBase->Key, buffer) ? KEYFILE_BAD : KEYFILE_OK;
                    else
                        strncpy(secBase->Key, buffer, sizeof(secBase->Key) - 1);
                    if (res == KEYFILE_OK &&
                        !(ParseDirLockLine(secBase, fs, file, &secBase->_pwdLock) &&
                          ParseDirLockLine(secBase, fs, file, &secBase->_cfgLock)))
                        res = KEYFILE_BAD;
                }
            }
            Close(file);
            ClearBuffer(secBase);
        }
        CurrentDir(olddir);
        UnLock(dir);
    }
    return res;
}

/*
 * Probe a volume for a key file: used to find filesystems that enforce
 * ownership natively (SFS, ...) but have no multi-user dostype. The key
 * must match the one of the other multi-user volumes.
 */
BOOL ProbeKeyFile(struct SecurityBase *secBase, struct MsgPort *fs)
{
    if (!ClearBuffer(secBase))
        return FALSE;
    return ReadKeyFile(secBase, fs) == KEYFILE_OK;
}

/*
 * Read the key files of the multi-user volumes. A volume without one is
 * simply not part of a key-file installation (a fresh AMUInit setup has
 * none anywhere: the configuration then comes from SYS:Security). A key
 * file that is present but unreadable, carries a different key, or
 * conflicts over the config directories marks its volume as tampered:
 * that volume is quarantined - treated as if it were not attached - and
 * the rest of the system boots on.
 *
 * Returns TRUE only when key files located both the password and the
 * config directory.
 */
BOOL ReadKeyFiles(struct SecurityBase *secBase)
{
    struct secVolume *vol;

    if (!secBase->Volumes || !ClearBuffer(secBase))
        return FALSE;

    for (vol = secBase->Volumes; vol; vol = vol->Next)
    {
        if (vol->FS_Flags)
            continue;
        if (ReadKeyFile(secBase, vol->Process) == KEYFILE_BAD)
        {
            bug(DEBUG_NAME_STR " %s: bad or inconsistent key file on '%s' - volume quarantined\n",
                __func__, vol->FS_Name ? vol->FS_Name : (STRPTR)"?");
            vol->Quarantined = TRUE;
        }
    }
    return (secBase->_pwdLock && secBase->_cfgLock) ? TRUE : FALSE;
}

/*
 * Load the configuration file
 */
void LoadConfig(struct SecurityBase *secBase)
{
    BPTR file, olddir;
    SIPTR *argarray[19];

#define argLIMITDOSSETPROTECTION    0
#define argPROFILE                  1
#define argLASTLOGINREQ             2
#define argLOGSTARTUP               3
#define argLOGLOGIN                 4
#define argLOGLOGINFAIL             5
#define argLOGPASSWD                6
#define argLOGPASSWDFAIL            7
#define argLOGCHECKPASSWD           8
#define argLOGCHECKPASSWDFAIL       9
#define argPASSWDUIDLEVEL           10
#define argPASSWDGIDLEVEL           11
#define argFSTAB                    12
#define argRESOURCETRACKING         13
#define argLOADPLUGIN               14
#define argHASHEDLOGIN              15
#define argMAXTRIES                 16
#define argLOCKTIME                 17
#define argLOGINDELAYMAX            18

    struct RDArgs *rdargs;
    ULONG line;
    struct secConfig config;
    char *Buffer;

    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    config.Flags = secCFGF_LimitDOSSetProtection | secCFGF_Profile | secCFGF_LastLoginReq;
    config.LogFlags = 0;
    config.PasswduidLevel = secNOBODY_UID;
    config.PasswdgidLevel = secNOBODY_UID;
    config.MaxTries = secAUTH_DEFMAXTRIES;
    config.LockTime = secAUTH_DEFLOCKTIME;
    config.LoginDelayMax = secAUTH_DEFDELAYMAX;

    if (secBase->_cfgLock && ClearBuffer(secBase) && (rdargs = AllocDosObject(DOS_RDARGS, NULL)))
    {
        Buffer = secBase->Buffer;
        olddir = CurrentDir(secBase->_cfgLock);
        if ((file = Open(ConfigFileName, MODE_OLDFILE)))
        {
            D(bug(DEBUG_NAME_STR " %s: '%s' opened\n", __func__, ConfigFileName);)
            for (line = 1; FGets(file, Buffer, secGENBUFSIZE - 1); line++)
            {
                if (!Buffer[0] || Buffer[0] == '\n' || Buffer[0] == ';' || Buffer[0] == '#')
                    continue;
                rdargs->RDA_Source.CS_Buffer = Buffer;
                rdargs->RDA_Source.CS_Length = strlen(Buffer);
                rdargs->RDA_Source.CS_CurChr = 0;
                rdargs->RDA_DAList = 0;
                rdargs->RDA_Buffer = NULL;
                rdargs->RDA_BufSiz = 0;
                rdargs->RDA_ExtHelp = NULL;
                rdargs->RDA_Flags = RDAF_NOPROMPT;
                memset(argarray, 0, sizeof(argarray));
                if (ReadArgs("LIMITDOSSETPROTECTION/K/N,PROFILE/K/N,LASTLOGINREQ/K/N,LOGSTARTUP/K/N,"
                             "LOGLOGIN/K/N,LOGLOGINFAIL/K/N,LOGPASSWD/K/N,LOGPASSWDFAIL/K/N,"
                             "LOGCHECKPASSWD/K/N,LOGCHECKPASSWDFAIL/K/N,PASSWDUIDLEVEL/K/N,"
                             "PASSWDGIDLEVEL/K/N,FSTAB/K/N,RESOURCETRACKING/K/N,LOADPLUGIN/K,"
                             "HASHEDLOGIN/K/N,MAXTRIES/K/N,LOCKTIME/K/N,LOGINDELAYMAX/K/N",
                             (SIPTR *)argarray, rdargs))
                {
#define BOOLOPT(idx, var, flag) \
                    if (argarray[idx]) { if (*argarray[idx]) var |= (flag); else var &= ~(flag); }

                    BOOLOPT(argLIMITDOSSETPROTECTION, config.Flags, secCFGF_LimitDOSSetProtection);
                    BOOLOPT(argPROFILE, config.Flags, secCFGF_Profile);
                    BOOLOPT(argLASTLOGINREQ, config.Flags, secCFGF_LastLoginReq);
                    BOOLOPT(argFSTAB, config.Flags, secCFGF_UseFSTab);
                    BOOLOPT(argRESOURCETRACKING, config.Flags, secCFGF_RT);
                    BOOLOPT(argHASHEDLOGIN, config.Flags, secCFGF_HashedLogin);
                    BOOLOPT(argLOGSTARTUP, config.LogFlags, secLogF_Startup);
                    BOOLOPT(argLOGLOGIN, config.LogFlags, secLogF_Login);
                    BOOLOPT(argLOGLOGINFAIL, config.LogFlags, secLogF_LoginFail);
                    BOOLOPT(argLOGPASSWD, config.LogFlags, secLogF_Passwd);
                    BOOLOPT(argLOGPASSWDFAIL, config.LogFlags, secLogF_PasswdFail);
                    BOOLOPT(argLOGCHECKPASSWD, config.LogFlags, secLogF_CheckPasswd);
                    BOOLOPT(argLOGCHECKPASSWDFAIL, config.LogFlags, secLogF_CheckPasswdFail);
#undef BOOLOPT
                    if (argarray[argPASSWDUIDLEVEL])
                    {
                        if (*argarray[argPASSWDUIDLEVEL] > 65535 || *argarray[argPASSWDUIDLEVEL] < 0)
                            Warn1(secBase, GetLocStr(secBase, MSG_BADVALUE_CONFIG), line);
                        else
                            config.PasswduidLevel = *argarray[argPASSWDUIDLEVEL];
                    }
                    if (argarray[argPASSWDGIDLEVEL])
                    {
                        if (*argarray[argPASSWDGIDLEVEL] > 65535 || *argarray[argPASSWDGIDLEVEL] < 0)
                            Warn1(secBase, GetLocStr(secBase, MSG_BADVALUE_CONFIG), line);
                        else
                            config.PasswdgidLevel = *argarray[argPASSWDGIDLEVEL];
                    }
                    if (argarray[argMAXTRIES])
                        config.MaxTries = (*argarray[argMAXTRIES] < 0 || *argarray[argMAXTRIES] > 65535) ? secAUTH_DEFMAXTRIES : *argarray[argMAXTRIES];
                    if (argarray[argLOCKTIME])
                        config.LockTime = (*argarray[argLOCKTIME] < 0 || *argarray[argLOCKTIME] > 65535) ? secAUTH_DEFLOCKTIME : *argarray[argLOCKTIME];
                    if (argarray[argLOGINDELAYMAX])
                        config.LoginDelayMax = (*argarray[argLOGINDELAYMAX] < 0 || *argarray[argLOGINDELAYMAX] > 3600) ? secAUTH_DEFDELAYMAX : *argarray[argLOGINDELAYMAX];
                    if (argarray[argLOADPLUGIN])
                    {
                        /* LOADPLUGIN name: the plugin file, WITHOUT the suffix */
                        if (!loadPlugin(secBase, (STRPTR)argarray[argLOADPLUGIN]))
                            Warn1(secBase, "Failed to load plugin \"%s\"", argarray[argLOADPLUGIN]);
                    }
                }
                else
                    Warn1(secBase, GetLocStr(secBase, MSG_BADOPTION_CONFIG), line);
                FreeArgs(rdargs);
            }
            Close(file);
            ClearBuffer(secBase);
        }
        else
        {
            D(bug(DEBUG_NAME_STR " %s: no configuration file, using defaults\n", __func__);)
        }
        CurrentDir(olddir);
        FreeDosObject(DOS_RDARGS, rdargs);
    }

    secBase->Config = config;
    secBase->LimitDOSSetProtection = (config.Flags & secCFGF_LimitDOSSetProtection) ? TRUE : FALSE;

    if (secBase->FirstStartup)
    {
        secBase->FirstStartup = FALSE;
        if (secBase->Config.LogFlags & secLogF_Startup)
            VLogF(secBase, LOGF_Flag_Type_Information, SECLOG_SUB_SERVER, GetLogStr(secBase, MSG_LOG_STARTUP), NULL);
        CheckStartupScript(secBase);
    }
}

/*
 * Rewrite a database file in 'dir': written to a new file, which is
 * protected as soon as it exists, and renamed into place only once it is
 * known to be complete; an interrupted rename is completed by
 * RecoverDbFile(). The old file is never touched if anything goes wrong
 * before that. 'write' returns the number of bytes it wrote, or -1.
 */
typedef LONG (*DbWriter)(struct SecurityBase *secBase, BPTR file);

static BOOL RewriteDbFile(struct SecurityBase *secBase, BPTR dir, CONST_STRPTR name, CONST_STRPTR newname,
                          CONST_STRPTR oldname, BOOL readable, DbWriter write)
{
    BPTR file, olddir;
    BOOL res = FALSE;

    if (!dir)
        return FALSE;

    olddir = CurrentDir(dir);
    DeleteFile(newname);
    if ((file = Open(newname, MODE_NEWFILE)))
    {
        LONG written;
        BPTR lock;
        struct FileInfoBlock *fib;

        if (readable)
            ProtectRootWritable(secBase, newname);
        else
            ProtectRootOnly(secBase, newname);
        written = write(secBase, file);
        res = Close(file) && (written > 0);

        /* the file on disk must hold all of it */
        if (res)
        {
            res = FALSE;
            if ((lock = Lock(newname, SHARED_LOCK)))
            {
                if ((fib = AllocDosObject(DOS_FIB, NULL)))
                {
                    if (Examine(lock, fib))
                        res = (fib->fib_Size == written);
                    FreeDosObject(DOS_FIB, fib);
                }
                UnLock(lock);
            }
        }
        if (res)
        {
            DeleteFile(oldname);
            res = Rename(name, oldname) && Rename(newname, name);
            if (res)
                DeleteFile(oldname);
            else
                RecoverDbFile(secBase, name, newname, oldname);
            /* some filesystems recreate on rename; make sure */
            if (readable)
                ProtectRootWritable(secBase, name);
            else
                ProtectRootOnly(secBase, name);
        }
        else
        {
            SIPTR args[1] = { (SIPTR)name };

            DeleteFile(newname);
            VLogF(secBase, LOGF_Flag_Type_Error, SECLOG_SUB_SERVER, "'%s' could not be rewritten; it is unchanged", args);
        }
    }
    CurrentDir(olddir);

    return res;
}

static LONG WritePasswd(struct SecurityBase *secBase, BPTR file)
{
    struct secUserDef *def;
    LONG written = 0, len;
    SIPTR args[7];

    for (def = secBase->UserDefs; def; def = def->Next)
    {
        args[0] = (SIPTR)def->UserID;
        args[1] = (SIPTR)def->Password;
        args[2] = (SIPTR)def->uid;
        args[3] = (SIPTR)def->gid;
        args[4] = (SIPTR)def->UserName;
        args[5] = (SIPTR)def->HomeDir;
        args[6] = (SIPTR)def->Shell;
        if ((len = VFPrintf(file, "%s|%s|%iu|%iu|%s|%s|%s\n", (RAWARG)args)) < 0)
            return -1;
        written += len;
    }
    return written;
}

/* The group lines, then - after an empty line - the secondary groups of
 * the users ("uid:gid,gid...") */
static LONG WriteGroup(struct SecurityBase *secBase, BPTR file)
{
    struct secGroupDef *gdef;
    struct secUserDef *udef;
    LONG written = 0, len;
    BOOL relations = FALSE;
    SIPTR args[4];
    ULONG i;

    for (gdef = secBase->GroupDefs; gdef; gdef = gdef->Next)
    {
        args[0] = (SIPTR)gdef->GroupID;
        args[1] = (SIPTR)gdef->gid;
        args[2] = (SIPTR)gdef->MgrUid;
        args[3] = (SIPTR)gdef->GroupName;
        if ((len = VFPrintf(file, "%s|%iu|%iu|%s\n", (RAWARG)args)) < 0)
            return -1;
        written += len;
    }
    for (udef = secBase->UserDefs; udef; udef = udef->Next)
    {
        if (!udef->NumSecGroups)
            continue;
        if (!relations)
        {
            if (FPutC(file, '\n') < 0)
                return -1;
            written++;
            relations = TRUE;
        }
        args[0] = (SIPTR)udef->uid;
        if ((len = VFPrintf(file, "%iu:", (RAWARG)args)) < 0)
            return -1;
        written += len;
        for (i = 0; i < udef->NumSecGroups; i++)
        {
            args[0] = (SIPTR)udef->SecGroups[i];
            if ((len = VFPrintf(file, i ? ",%iu" : "%iu", (RAWARG)args)) < 0)
                return -1;
            written += len;
        }
        if (FPutC(file, '\n') < 0)
            return -1;
        written++;
    }
    return written;
}

/*
 * Write the user definitions back to the password file
 */
BOOL UpdateUserDefs(struct SecurityBase *secBase)
{
    return RewriteDbFile(secBase, secBase->_pwdLock, PasswdFileName, secPasswd_NewName, secPasswd_OldName,
                         FALSE, WritePasswd);
}

/*
 * Write the group definitions and the users' secondary groups back to the
 * group file
 */
BOOL UpdateGroupDefs(struct SecurityBase *secBase)
{
    return RewriteDbFile(secBase, secBase->_cfgLock, GroupFileName, secGroup_NewName, secGroup_OldName,
                         TRUE, WriteGroup);
}

/*
 * Format a log line and record it: in log.resource (if present) and
 * appended to the log file in the configuration directory (if that is
 * known yet). User IDs, service and host names come from callers, so
 * control characters are replaced: a name cannot forge further lines.
 */
void VLogF(struct SecurityBase *secBase, ULONG type, CONST_STRPTR sub, CONST_STRPTR fmt, SIPTR *argv)
{
    BPTR file, olddir;
    char date[LEN_DATSTRING];
    char time[LEN_DATSTRING];
    char fixed[256];
    char text[512];
    struct DateTime dt;
    SIPTR args[3];
    UBYTE *p;

    FixFormat(fmt, fixed, sizeof(fixed));
    FormatString(fixed, argv, text, sizeof(text));
    for (p = (UBYTE *)text; *p; p++)
    {
        if (*p < 0x20 || (*p >= 0x7f && *p < 0xa0))
            *p = '?';
    }
    args[0] = (SIPTR)text;
    VSecLog(secBase, type, sub, "%s", args);

    ObtainSemaphore(&secBase->LogSem);
    /* checked under LogSem: the server drops the lock under it too */
    if (secBase->_cfgLock && secBase->sec_AfterDOSDone)
    {
        olddir = CurrentDir(secBase->_cfgLock);
        if ((file = Open(LogFileName, MODE_READWRITE)))
        {
            if (Seek(file, 0, OFFSET_END) != -1)
            {
                DateStamp(&dt.dat_Stamp);
                dt.dat_Format = FORMAT_DOS;
                dt.dat_Flags = 0;
                dt.dat_StrDay = NULL;
                dt.dat_StrDate = date;
                dt.dat_StrTime = time;
                DateToStr(&dt);
                args[0] = (SIPTR)date;
                args[1] = (SIPTR)time;
                args[2] = (SIPTR)text;
                VFPrintf(file, "%s, %s: %s\n", (RAWARG)args);
            }
            Close(file);
        }
        CurrentDir(olddir);
    }
    ReleaseSemaphore(&secBase->LogSem);
}
