
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

/*
 *      Configuration Stuff
 */

const char KeyFileName[] = secKey_FileName;
const char PasswdFileName[] = secPasswd_FileName;
const char ConfigFileName[] = secConfig_FileName;
const char GroupFileName[] = secGroup_FileName;
const char LogFileName[] = secLog_FileName;

struct secUserDef *UserDef = NULL;
struct secGroupDef *GroupDef = NULL;

/*
 *      General Purpose Buffer
 */

#define GENBUFSIZE 1024

char *Buffer = NULL;

char Key[64];

/*
 *      First run indicator
 */
BOOL FirstStartup = TRUE;

/*
 *      Clear the General Purpose Buffer
 */

void ClearBuffer(void)
{
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (!Buffer && !(Buffer = MAlloc(GENBUFSIZE)))
        Die(NULL, AN_Unknown | AG_NoMemory);
    memset(Buffer, 0, GENBUFSIZE);
}

void FreeBuffer(void)
{
    if (Buffer) {
        Free(Buffer, GENBUFSIZE);
        Buffer = NULL;
    }
}

void PurgeKeyBuffer(void)
{
    memset(Key, 0, sizeof(Key));
}

/*
 *      Parse a User Entry.
 *      Note: Is is traditional to be able to specify the name of the nobody user,
 *      so allow for this.
 *      The Who support program will probably need tweaking to match.
 */

static struct secUserDef *ParseUserLine(struct SecurityBase *secBase, STRPTR line, ULONG linenum)
{
    int i, j, len;
    LONG uid, gid;
    struct secUserDef *def;
    STRPTR part[7];
    STRPTR ptr;

#define UPART_USERID            0
#define UPART_PASSWORD          1
#define UPART_UID               2
#define UPART_GID               3
#define UPART_USERNAME          4
#define UPART_HOMEDIR           5
#define UPART_SHELL             6

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    i = 0;
    for (j = 0; j < 7; j++) {
        part[j] = &line[i];
        while ((line[i]) && (line[i] != '\n') && (line[i] != '|'))
            i++;
        if (j == 6)
            if (line[i] && (line[i] != '\n'))
                goto Fail;
            else {}
        else if (line[i] != '|')
            goto Fail;
        line[i++] = '\0';
        len = strlen(part[j]);
        switch(j) {
            case UPART_USERID:
                if (!len)
                    goto Fail;
                break;

            case UPART_PASSWORD:
                if (len && (len != (MaxPwdLen(secBase))))
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
            case UPART_HOMEDIR:
            case UPART_SHELL:
                break;
        }
    }

    if ((def = (struct secUserDef *)MAllocV(sizeof(struct secUserDef)+strlen(part[UPART_USERID])+
                    strlen(part[UPART_USERNAME])+strlen(part[UPART_HOMEDIR])+
                    strlen(part[UPART_SHELL])+(MaxPwdLen(secBase)+1) +4))) {
        ptr = &((STRPTR)def)[sizeof(struct secUserDef)];
        def->UserID = ptr;
        strcpy(ptr, part[UPART_USERID]);
        ptr = &ptr[strlen(part[UPART_USERID])+1];
        def->Password = ptr;
        strcpy(ptr, part[UPART_PASSWORD]);
        ptr = &ptr[MaxPwdLen(secBase)+1];
        def->uid = (UWORD)uid;
        def->gid = (UWORD)gid;
        def->UserName = ptr;
        strcpy(ptr, part[UPART_USERNAME]);
        ptr = &ptr[strlen(part[UPART_USERNAME])+1];
        def->HomeDir = ptr;
        strcpy(ptr, part[UPART_HOMEDIR]);
        ptr = &ptr[strlen(part[UPART_HOMEDIR])+1];
        def->Shell = ptr;
        strcpy(ptr, part[UPART_SHELL]);
    } else
        Die(NULL, AN_Unknown | AG_NoMemory);
    return(def);

Fail:
    Warn(secBase, GetLocStr(secBase, MSG_BADENTRY_PASSWD), linenum);
    return(NULL);
}


/*
 *      Parse a Group Entry
 */

static struct secGroupDef *ParseGroupLine(struct SecurityBase *secBase, STRPTR line, ULONG linenum)
{
    int i, j, len;
    LONG gid, mgruid;
    struct secGroupDef *def;
    STRPTR part[4];
    STRPTR ptr;

#define GPART_GROUPID	0
#define GPART_GID			1
#define GPART_MGRUID		2
#define GPART_GROUPNAME	3

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    i = 0;
    for (j = 0; j < 4; j++) {
        part[j] = &line[i];
        while ((line[i]) && (line[i] != '\n') && (line[i] != '|'))
                i++;
        if (j == 3)
            if (line[i] && (line[i] != '\n'))
                goto Fail;
            else {}
        else if (line[i] != '|')
            goto Fail;
        line[i++] = '\0';
        len = strlen(part[j]);
        switch(j) {
            case GPART_GROUPID:
                if (!len)
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
                break;
        }
    }

    if ((def = (struct secGroupDef *)MAllocV(sizeof(struct secGroupDef)+strlen(part[GPART_GROUPID])+
                    strlen(part[GPART_GROUPNAME])+2))) {
        ptr = &((STRPTR)def)[sizeof(struct secGroupDef)];
        def->GroupID = ptr;
        strcpy(ptr, part[GPART_GROUPID]);
        ptr = &ptr[strlen(part[GPART_GROUPID])+1];
        def->gid = (UWORD)gid;
        def->MgrUid = (UWORD)mgruid;
        def->GroupName = ptr;
        strcpy(ptr, part[GPART_GROUPNAME]);
    } else
        Die(NULL, AN_Unknown | AG_NoMemory);
    return(def);

Fail:
    Warn(secBase, GetLocStr(secBase, MSG_BADENTRY_GROUP), linenum);
    return(NULL);
}

/*
 *      Parse a Relation Entry
 */

static void ParseRelationLine(struct SecurityBase *secBase, STRPTR line, ULONG linenum)
{
    struct secUserDef *def;
    UWORD *groups;
    ULONG numgroups = 0;
    LONG uid, gid, len;
    ULONG i, j;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (((len = StrToLong(line, &uid)) == -1) || (uid < -2) || (uid > 65535))
            goto Fail;
    i = len;
    j = i+1;
    if (line[i] != ':')
            goto Fail;
    for (def = UserDef; def && (def->uid != uid); def = def->Next);
    if (def) {
            do {
                    if (((len = StrToLong(&line[++i], &gid)) == -1) || (gid < 0) || (gid > 65535))
                            goto Fail;
                    i += len;
                    numgroups++;
                    if (line[i] && (line[i] != ',') && (line[i] != '\n'))
                            goto Fail;
            } while (line[i] && (line[i] != '\n'));
            if (def->NumSecGroups+numgroups <= 65535) {
                if (def->NumSecGroups)
                    if ((groups = MAlloc((numgroups+def->NumSecGroups)*sizeof(UWORD)))) {
                        CopyMem(def->SecGroups, groups, def->NumSecGroups*sizeof(UWORD));
                        Free(def->SecGroups, def->NumSecGroups*sizeof(UWORD));
                        def->SecGroups = groups;
                        groups += def->NumSecGroups;
                        def->NumSecGroups += numgroups;
                    } else
                        Die(NULL, AN_Unknown | AG_NoMemory);
                    else if ((groups = MAlloc(numgroups*sizeof(UWORD)))) {
                        def->SecGroups = groups;
                        def->NumSecGroups = numgroups;
                    } else
                        Die(NULL, AN_Unknown | AG_NoMemory);
                    if (groups)
                        for (i = 0; i < numgroups; i++) {
                            j += StrToLong(&line[j], &gid) + 1;
                            groups[i] = gid;
                        }
            } else {
                Warn(secBase, GetLocStr(secBase, MSG_TOOMANYSECGROUPS), uid, linenum);
            }
    } else {
Fail:
        Warn(secBase, GetLocStr(secBase, MSG_BADENTRY_GROUP), linenum);
    }
}

/*
 *      Initialise the User and Group Definitions
 */

static void InitDefs(struct SecurityBase *secBase)
{
    BPTR file;
    ULONG linenum;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    CurrentDir(secBase->_pwdLock);
    if ((file = Open(PasswdFileName, MODE_OLDFILE))) {
        struct secUserDef *def1, *def2 = NULL;

        for (linenum = 1; FGets(file, Buffer, GENBUFSIZE-1); linenum++)
            /* V36/37: use GENBUFSIZE-1 */
            /* V39: use GENBUFSIZE */
            if (Buffer[0] && (Buffer[0] != '\n'))
                    if ((def1 = ParseUserLine(secBase, Buffer, linenum))) {
                        if (def2)
                            def2->Next = def1;
                        else
                            UserDef = def1;
                        def2 = def1;
                    }
        Close(file);
    }
    CurrentDir(secBase->_cfgLock);

    if (UserDef && (file = Open(GroupFileName, MODE_OLDFILE))) {
        struct secGroupDef *def1, *def2 = NULL;

        for (linenum = 1; FGets(file, Buffer, GENBUFSIZE-1) && (Buffer[0] != '\n'); linenum++)
            /* V36/37: use GENBUFSIZE-1 */
            /* V39: use GENBUFSIZE */
            if (Buffer[0])
                if ((def1 = ParseGroupLine(secBase, Buffer, linenum))) {
                    if (def2)
                        def2->Next = def1;
                    else
                        GroupDef = def1;
                    def2 = def1;
                }
        if (Buffer[0] == '\n')
            for (linenum++; FGets(file, Buffer, GENBUFSIZE-1); linenum++)
                /* V36/37: use GENBUFSIZE-1 */
                /* V39: use GENBUFSIZE */
                if (Buffer[0] && (Buffer[0] != '\n'))
                    ParseRelationLine(secBase, Buffer, linenum);
        Close(file);
    }

    ClearBuffer();

    if (!UserDef || !GroupDef)
        FreeDefs();
}

/*
 *      Free the User and Group Definitions
 */

void FreeDefs(void)
{
    struct secUserDef *udef = UserDef;
    struct secGroupDef *gdef = GroupDef;
    APTR p;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    while(udef) {
        p = udef->Next;
        if (udef->NumSecGroups)
            Free(udef->SecGroups, udef->NumSecGroups*sizeof(UWORD));
        
        FreeV(udef);
        udef = p;
    }
    UserDef = NULL;
    while(gdef) {
        p = gdef->Next;
        FreeV(gdef);
        gdef = p;
    }
    GroupDef = NULL;
}

/*
 *      Get a pointer to the User Definitions
 */

struct secUserDef *GetUserDefs(struct Library *_Base)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (!UserDef)
        InitDefs(secBase);
    return(UserDef);
}

/*
 *      Get a pointer to the Group Definitions
 */

struct secGroupDef *GetGroupDefs(struct Library *_Base)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (!GroupDef)
        InitDefs(secBase);
    return(GroupDef);
}

/*
 *      Parse a Key File Line and Lock the appropriate Directory
 */
static void RemTerminatingLF(char *buffer)
{
    int i;

    for (i = 0; buffer[i]; i++);
    if (i && (buffer[i-1] == '\n'))
        buffer[i-1] = '\0';
}

static BOOL ParseDirLockLine(struct MsgPort *fs, BPTR file, BPTR *dir)
{
    BOOL res = FALSE;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if ((res = (FGets(file, Buffer+1, GENBUFSIZE-2)==NULL?FALSE:TRUE)  )) {
        /* V36/37: use GENBUFSIZE-2 */
        /* V39: use GENBUFSIZE-1 */
        RemTerminatingLF(Buffer);
        if (Buffer[1]) {
            if (*dir)	{
                res = FALSE;
            } else {
                Buffer[0] = strlen(Buffer+1);
                if ((*dir = (BPTR)DoPkt(fs, ACTION_LOCATE_OBJECT, 0, (SIPTR)MKBADDR(Buffer), ACCESS_READ, 0, 0)) != BNULL)
                    res = TRUE;
            }
        }
    }
    return(res);
}

/*
 *      'Safe' FGets
 *
 *      Prevents synchronization problems on startup
 */

static STRPTR SafeFGets(BPTR fh, STRPTR buf, ULONG len)
{
    STRPTR res;
    int i;

    if (!(res = FGets(fh, buf, len)))
        for (i = 1; !res && (i < 10); i++) {
            Delay(25);
            res = FGets(fh, buf, len);
        }
    return(res);
}

/*
 *      Read and parse the Key Files
 */

static BOOL ReadKeyFile(struct Library *_Base, struct MsgPort *fs)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;
    BPTR dir, file;
    char buffer[12];
    BOOL res = FALSE;
    static char name[] = "\1:";

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    dir = (BPTR)DoPkt(fs, ACTION_LOCATE_OBJECT, 0, (SIPTR)MKBADDR(name), ACCESS_READ, 0, 0);
    if (dir) {
        CurrentDir(dir);
        file = Open(KeyFileName, MODE_OLDFILE);
        if (file) {
            if (SafeFGets(file, Buffer, GENBUFSIZE-1)) {
                res = TRUE;
                /* V36/37: use GENBUFSIZE-1 */
                /* V39: use GENBUFSIZE */
                RemTerminatingLF(Buffer);
                if (Encrypt(buffer, Buffer, "Alpha, PowerPC or R4400?")) {
                    if (Key[0])	{
                        res = !strcmp(Key, buffer);
                    } else {
                        strcpy(Key, buffer);
                    }
                    if (res)	{
                        res = (ParseDirLockLine(fs, file, &secBase->_pwdLock) &&
                                ParseDirLockLine(fs, file, &secBase->_cfgLock));
                    }
                }
            }
            Close(file);
            ClearBuffer();
        }
        UnLock(dir);
    }
    return(res);
}

BOOL ReadKeyFiles(struct Library *_Base)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;
    struct secVolume *vol;
    BOOL res = FALSE;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (secBase->Volumes)
    {
        for (vol = secBase->Volumes; vol && (res = (vol->FS_Flags?TRUE:ReadKeyFile(_Base, vol->Process))); vol = vol->Next);
        return((BOOL)(res && secBase->_pwdLock && secBase->_cfgLock));
    }
    return FALSE;
}

/*
 *      Attempt to load the Configuration file
 */
void LoadConfig(struct Library *_Base)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;
    BPTR file;
    SIPTR *argarray[15] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#define argLIMITDOSSETPROTECTION	0
#define argPROFILE					1
#define argLASTLOGINREQ				2
#define argLOGSTARTUP				3
#define argLOGLOGIN					4
#define argLOGLOGINFAIL				5
#define argLOGPASSWD					6
#define argLOGPASSWDFAIL			7
#define argLOGCHECKPASSWD			8
#define argLOGCHECKPASSWDFAIL		9
#define argPASSWDUIDLEVEL			10
#define argPASSWDGIDLEVEL			11
#define argFSTAB						12
/*#define argRESOURCETRACKING		13*/
#define argLOADPLUGIN				14

    struct RDArgs *rdargs;
    ULONG line;
    struct secConfig config;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    config.Flags = muCFGF_LimitDOSSetProtection | muCFGF_Profile | muCFGF_LastLoginReq;
    config.LogFlags = 0;
    config.PasswduidLevel = secNOBODY_UID;
    config.PasswdgidLevel = secNOBODY_UID;

    if ((rdargs = AllocDosObject(DOS_RDARGS, NULL))) {
        CurrentDir(secBase->_cfgLock);
        if ((file = Open(ConfigFileName, MODE_OLDFILE))) {
            D(bug( DEBUG_NAME_STR " %s: '%s' opened @ %p\n", __func__, ConfigFileName, file);)
            for (line = 1; FGets(file, Buffer, GENBUFSIZE-1); line++) {
                D(bug( DEBUG_NAME_STR " %s:        Config Line '%s'\n", __func__, Buffer);)
                /* V36/37: use GENBUFSIZE-1 */
                /* V39: use GENBUFSIZE */
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
                                "PASSWDGIDLEVEL/K/N,FSTAB/K/N,RESOURCETRACKING/K/N,LOADPLUGIN/K"
                                , (SIPTR *)argarray, rdargs)) {
                    if (argarray[argLIMITDOSSETPROTECTION])
                        if (*argarray[argLIMITDOSSETPROTECTION])
                            config.Flags |= muCFGF_LimitDOSSetProtection;
                        else
                            config.Flags &= ~muCFGF_LimitDOSSetProtection;
                    if (argarray[argPROFILE])
                        if (*argarray[argPROFILE])
                            config.Flags |= muCFGF_Profile;
                        else
                            config.Flags &= ~muCFGF_Profile;
                    if (argarray[argLASTLOGINREQ])
                        if (*argarray[argLASTLOGINREQ])
                            config.Flags |= muCFGF_LastLoginReq;
                        else
                            config.Flags &= ~muCFGF_LastLoginReq;
                    if (argarray[argLOGSTARTUP])
                        if (*argarray[argLOGSTARTUP])
                            config.LogFlags |= muLogF_Startup;
                        else
                            config.LogFlags &= ~muLogF_Startup;
                    if (argarray[argLOGLOGIN])
                        if (*argarray[argLOGLOGIN])
                            config.LogFlags |= muLogF_Login;
                        else
                            config.LogFlags &= ~muLogF_Login;
                    if (argarray[argLOGLOGINFAIL])
                        if (*argarray[argLOGLOGINFAIL])
                            config.LogFlags |= muLogF_LoginFail;
                        else
                            config.LogFlags &= ~muLogF_LoginFail;
                    if (argarray[argLOGPASSWD])
                        if (*argarray[argLOGPASSWD])
                            config.LogFlags |= muLogF_Passwd;
                        else
                            config.LogFlags &= ~muLogF_Passwd;
                    if (argarray[argLOGPASSWDFAIL])
                        if (*argarray[argLOGPASSWDFAIL])
                            config.LogFlags |= muLogF_PasswdFail;
                        else
                            config.LogFlags &= ~muLogF_PasswdFail;
                    if (argarray[argLOGCHECKPASSWD])
                        if (*argarray[argLOGCHECKPASSWD])
                            config.LogFlags |= muLogF_CheckPasswd;
                        else
                            config.LogFlags &= ~muLogF_CheckPasswd;
                    if (argarray[argLOGCHECKPASSWDFAIL])
                        if (*argarray[argLOGCHECKPASSWDFAIL])
                            config.LogFlags |= muLogF_CheckPasswdFail;
                        else
                            config.LogFlags &= ~muLogF_CheckPasswdFail;
                    if (argarray[argPASSWDUIDLEVEL])
                        if (*argarray[argPASSWDUIDLEVEL] > 65535)
                            Warn(secBase, GetLocStr(secBase, MSG_BADVALUE_CONFIG), line);
                        else
                            config.PasswduidLevel = *argarray[argPASSWDUIDLEVEL];
                    if (argarray[argPASSWDGIDLEVEL])
                        if (*argarray[argPASSWDGIDLEVEL] > 65535)
                            Warn(secBase, GetLocStr(secBase, MSG_BADVALUE_CONFIG), line);
                        else
                            config.PasswdgidLevel = *argarray[argPASSWDGIDLEVEL];
                    
                    if (argarray[argFSTAB])
                        if (*argarray[argFSTAB])	{
                            config.Flags |= muCFGF_UseFSTab;
                        } else	{
                            config.Flags &= ~muCFGF_UseFSTab;
                        }
#if 0 /* Resource Tracking */
                    if (argarray[argRESOURCETRACKING])
                        if (*argarray[argRESOURCETRACKING])
                            config.Flags |= muCFGF_RT;
                        else
                            config.Flags &= ~muCFGF_RT;
#endif 
                    if (argarray[argLOADPLUGIN])	{
                        /* LOADPLUGIN/K where the keyword is the name of the plugin
                         * file, WITHOUT the .secfsplugin extension */
                        /* FIXME: What should we do if a plugin fails to load? */
                        if (!loadPlugin(secBase, (STRPTR)argarray[argLOADPLUGIN]))
                            Warn(secBase, "Failed to load plugin \"%s\"", argarray[argLOADPLUGIN]);
                    }
                } else
                    Warn(secBase, GetLocStr(secBase, MSG_BADOPTION_CONFIG), line);
                FreeArgs(rdargs);
            }
            Close(file);
            ClearBuffer();
        } else
            Warn(secBase, GetLocStr(secBase, MSG_NOCONFIGFILE));
        D(bug( DEBUG_NAME_STR " %s: Freeing Args...\n", __func__);)
        FreeDosObject(DOS_RDARGS, rdargs);
    } else
        Die(NULL, AN_Unknown | AG_NoMemory);

    D(bug( DEBUG_NAME_STR " %s: Done\n", __func__);)

    secBase->Config.Flags = config.Flags;
    secBase->Config.LogFlags = config.LogFlags;
    secBase->Config.PasswduidLevel = config.PasswduidLevel;
    secBase->Config.PasswdgidLevel = config.PasswdgidLevel;

    if (FirstStartup) {
        FirstStartup = FALSE;
        if (secBase->Config.LogFlags & muLogF_Startup)
            VLogF(_Base, "Startup", NULL);
    }
}


/*
 *      Update the User Definitions
 *
 *
 *      OUT:	BOOL	Success
 */

BOOL UpdateUserDefs(struct Library *_Base)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;
    BPTR file;
    BOOL res = FALSE;
    struct secUserDef *def = UserDef;
    SIPTR args[7];

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    CurrentDir(secBase->_pwdLock);
    if ((file = Open(PasswdFileName, MODE_NEWFILE))) {
        res = TRUE;
        while(def && res) {
            args[0] = (SIPTR)def->UserID;
            args[1] = (SIPTR)def->Password;
            args[2] = (SIPTR)def->uid;
            args[3] = (SIPTR)def->gid;
            args[4] = (SIPTR)def->UserName;
            args[5] = (SIPTR)def->HomeDir;
            args[6] = (SIPTR)def->Shell;
            res = (VFPrintf(file, "%s|%s|%ld|%ld|%s|%s|%s\n", (RAWARG)args) != -1);
            def = def->Next;
        }
        res = Close(file) && res;
    }

    return(res);
}

/*
 *      Format and dump a string to the Log File
 */

void VLogF(struct Library *_Base, STRPTR fmt, SIPTR *argv)
{
    struct SecurityBase *secBase = (struct SecurityBase *)_Base;
    BPTR file;
    char date[LEN_DATSTRING];
    char time[LEN_DATSTRING];
    struct DateTime dt;
    SIPTR args[2];

    CurrentDir(secBase->_cfgLock);
    if ((file = Open(LogFileName, MODE_READWRITE))) {
        if (Seek(file, 0, OFFSET_END) != -1) {
            DateStamp(&dt.dat_Stamp);
            dt.dat_Format = FORMAT_DOS;
            dt.dat_Flags = 0;
            dt.dat_StrDay = NULL;
            dt.dat_StrDate = date;
            dt.dat_StrTime = time;
            DateToStr(&dt);
            args[0] = (SIPTR)date;
            args[1] = (SIPTR)time;
            VFPrintf(file, "%s, %s: ", (RAWARG)args);
            VFPrintf(file, fmt, (RAWARG)argv);
            FPutC(file, '\n');
        }
        Close(file);
    }
}
