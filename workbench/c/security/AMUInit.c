/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AMUInit - AROS Multi-User Init: enable or disable multi-user
          behaviour on a volume (security.library).
*/

/*****************************************************************************

    NAME
        AMUInit

    SYNOPSIS
        ENABLE/S,DISABLE/S,UPGRADE/S,VOLUME/A,FORCE/S

    LOCATION
        C:

    FUNCTION
        Enables, disables, or upgrades multi-user startup on a volume.

        ENABLE creates the security database directory <VOLUME>Security
        with a user database (passwd), group database (group) and
        configuration (Security.config), and installs
        <VOLUME>S/Security-Startup.

        The databases are taken from what the system has: on a multi-user
        system, the ones in use (run AMUInit as root); on a single-user
        system, the network database (AROSTCP's db/passwd and db/group,
        through usergroup.library) - accounts with a password are locked,
        as their hashes cannot be converted, and root has none. Without
        either, the standard accounts and groups of AROSTCP's db/passwd and
        db/group: the super user "root" without a password, the locked
        accounts "nobody", "services" and "guest", and the groups "admin"
        (root's), "nobody", "services", "guests" and "users".
        That script is run by dos.library before the Startup-Sequence when
        security.library is part of the ROM; it shows the login prompt, and
        the Startup-Sequence then runs as the user who logged in.

        DISABLE renames S/Security-Startup to
        S/Security-Startup.disabled. User and group databases are retained.
        The change takes effect when booting from that volume again.

        UPGRADE brings a volume enabled by an earlier version up to date:
        an installed Security-Startup older than this AMUInit's is saved
        as Security-Startup.old and replaced (FORCE replaces it whatever
        its version), and the directories pam.library uses are created.
        The databases are not touched; security.library itself corrects
        their owner and protection when it starts.

    INPUTS
        ENABLE   -- set the volume up for multi-user boots.
        DISABLE  -- boot single-user again.
        UPGRADE  -- update the startup script of an enabled volume.
        VOLUME   -- the volume (or any path on it), e.g. SYS: or DH0:.
        FORCE    -- ENABLE, UPGRADE: rewrite an existing Security-Startup.

    RESULT
        RETURN_OK if the requested operation succeeds.
        RETURN_WARN if DISABLE or UPGRADE finds no active startup script.
        RETURN_FAIL if the request is invalid or the operation fails.

    NOTES
        The volume must support file ownership. ENABLE preserves a disabled
        startup script by restoring it unless FORCE is specified.
        Set a password for root after the first multi-user boot.

    EXAMPLE
        AMUInit ENABLE SYS:
        AMUInit DISABLE SYS:
        AMUInit UPGRADE SYS:

    SEE ALSO
        Login, Logout, Passwd, SetOwner, Who

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>
#include <dos/dos.h>
#include <aros/libcall.h>
#include <libraries/security.h>
#include <aros/posixc/pwd.h>
#include <aros/posixc/grp.h>
#include <string.h>

struct Library *secBase;

const TEXT version[] = "$VER: AMUInit 45.9 (19.10.2026)";

#define TEMPLATE "ENABLE/S,DISABLE/S,UPGRADE/S,VOLUME/A,FORCE/S"
enum { ARG_ENABLE, ARG_DISABLE, ARG_UPGRADE, ARG_VOLUME, ARG_FORCE, ARG_COUNT };

/* The version of the script below (its $VER line) */
#define STARTUP_VERSION     secSTARTUP_VERSION
#define STARTUP_REVISION    secSTARTUP_REVISION

#define SEC_DIR         "Security"
#define STARTUP         "S/Security-Startup"
#define STARTUP_OFF     "S/Security-Startup.disabled"

static const char StartupScript[] =
    "; $VER: Security-Startup 45.12 (17.10.2026)\n"
    ";\n"
    "FailAt 21\n"
    "Assign LIBS: SYS:Classes ADD\n"
    "If NOT EXISTS \"RAM:T\"\n"
    "    MakeDir \"RAM:T\"\n"
    "EndIf\n"
    "Assign T:       \"RAM:T\"\n"
    "Assign IMAGES:  \"SYS:System/Images\" DEFER\n"
    "Assign THEMES: \"SYS:Prefs/Presets/Themes\" >NIL:\n"
    "Assign ENV: \"ENVARC:\"\n"
    "If EXISTS \"ENV:SYS/theme.var\"\n"
    "    Assign THEME: \"${SYS/theme.var}\"\n"
    "Else\n"
    "    Assign THEME: \"THEMES:AROSDefault\"\n"
    "EndIf\n"
    "If EXISTS \"THEME:Images\"\n"
    "    Assign IMAGES: \"THEME:Images\" PREPEND\n"
    "EndIf\n"
    "AddDataTypes REFRESH QUIET\n"
    "If EXISTS \"SYS:Classes/USB\"\n"
    "    Assign USBCLASSES: \"SYS:Classes/USB\"\n"
    "    Run <NIL: >NIL: QUIET SYS:C/AddUSBClasses\n"
    "EndIf\n"
    "If EXISTS \"SYS:Classes/Bluetooth\"\n"
    "    Assign BTCLASSES: \"SYS:Classes/Bluetooth\"\n"
    "    Run <NIL: >NIL: QUIET SYS:C/AddBTClasses\n"
    "EndIf\n"
    "If EXISTS \"C:BTStackLoader\"\n"
    "    SYS:C/BTStackLoader >NIL:\n"
    "EndIf\n"
    "\n"
    "If EXISTS \"SYS:C/Decoration\"\n"
    "    If EXISTS \"ENV:SYS/theme.var\"\n"
    "        SYS:C/Decoration <NIL: >NIL:\n"
    "    EndIf\n"
    "EndIf\n"
    "IPrefs\n"
    "\n"
    "Resident SYS:C/Wait\n"
    "Resident SYS:C/Skip\n"
    "Resident SYS:C/Login\n"
    "\n"
    ";\n"
    "; First run on this volume: secure the database files and create the\n"
    "; profiles area, while the boot context still has root's rights.\n"
    "If NOT EXISTS \"SYS:Security/Profiles\"\n"
    "    SetOwner \"SYS:Security\" UID=65535 GID=65535 >NIL:\n"
    "    Protect \"SYS:Security\" rwed GROUP=re OTHER=re QUIET\n"
    "    SetOwner \"SYS:Security/passwd\" UID=65535 GID=65535 >NIL:\n"
    "    Protect \"SYS:Security/passwd\" rwd QUIET\n"
    "    SetOwner \"SYS:Security/group\" UID=65535 GID=65535 >NIL:\n"
    "    Protect \"SYS:Security/group\" rwd GROUP=r OTHER=r QUIET\n"
    "    If EXISTS \"SYS:Security/Security.config\"\n"
    "        SetOwner \"SYS:Security/Security.config\" UID=65535 GID=65535 >NIL:\n"
    "        Protect \"SYS:Security/Security.config\" rwd QUIET\n"
    "    EndIf\n"
    "    If EXISTS \"SYS:Security/Security.log\"\n"
    "        SetOwner \"SYS:Security/Security.log\" UID=65535 GID=65535 >NIL:\n"
    "        Protect \"SYS:Security/Security.log\" rwd QUIET\n"
    "    EndIf\n"
    "    MakeDir \"SYS:Security/Profiles\" >NIL:\n"
    "    SetOwner \"SYS:Security/Profiles\" UID=65535 GID=65535 >NIL:\n"
    "    Protect \"SYS:Security/Profiles\" rwed GROUP=re OTHER=re QUIET\n"
    "EndIf\n"
    ";\n"
    "; pam.library's service files and modules: root's, and nobody else may\n"
    "; write there (security.library also checks this at every start).\n"
    "If NOT EXISTS \"SYS:Security/PAM\"\n"
    "    MakeDir \"SYS:Security/PAM\" >NIL:\n"
    "    SetOwner \"SYS:Security/PAM\" UID=65535 GID=65535 >NIL:\n"
    "    Protect \"SYS:Security/PAM\" rwed GROUP=re OTHER=re QUIET\n"
    "EndIf\n"
    "If NOT EXISTS \"SYS:Libs/PAM\"\n"
    "    MakeDir \"SYS:Libs/PAM\" >NIL:\n"
    "    SetOwner \"SYS:Libs/PAM\" UID=65535 GID=65535 >NIL:\n"
    "    Protect \"SYS:Libs/PAM\" rwed GROUP=re OTHER=re QUIET\n"
    "EndIf\n"
    ";\n"
    "; The users' security keys (root only) and security.library's plugins.\n"
    "If NOT EXISTS \"SYS:Security/Keys\"\n"
    "    MakeDir \"SYS:Security/Keys\" >NIL:\n"
    "EndIf\n"
    "SetOwner \"SYS:Security/Keys\" UID=65535 GID=65535 >NIL:\n"
    "Protect \"SYS:Security/Keys\" rwed QUIET\n"
    "If EXISTS \"SYS:Libs/Security\"\n"
    "    SetOwner \"SYS:Libs/Security\" UID=65535 GID=65535 >NIL:\n"
    "    Protect \"SYS:Libs/Security\" rwed GROUP=re OTHER=re QUIET\n"
    "EndIf\n"
    ";\n"
    "; T: and CLIPS: are per-user assigns: every user gets a directory of their\n"
    "; own in RAM:T and RAM:Clipboards, created for them on first use\n"
    "; (dos.library expands %u for the task that uses the assign). Anybody\n"
    "; may create their directory there; nobody may remove the parents.\n"
    "; The Startup-Sequence keeps these assigns.\n"
    "If NOT EXISTS \"RAM:Clipboards\"\n"
    "    MakeDir \"RAM:Clipboards\" >NIL:\n"
    "EndIf\n"
    "Protect \"RAM:T\" rwed GROUP=rwe OTHER=rwe QUIET\n"
    "Protect \"RAM:Clipboards\" rwed GROUP=rwe OTHER=rwe QUIET\n"
    "Assign T: \"RAM:T/%u\" USER >NIL:\n"
    "Assign CLIPS: \"RAM:Clipboards/%u\" USER >NIL:\n"
    ";\n"
    "; Keep asking until somebody logs in (Login returns WARN when the login\n"
    "; failed or was cancelled); the Startup-Sequence never runs as nobody.\n"
    "; SYSTEM offers Shutdown/Reboot instead of Cancel; without GRAPHICAL (or\n"
    "; without MUI) the prompt appears on this console.\n"
    "Lab login\n"
    "Login PARENT GRAPHICAL QUIET SYSTEM\n"
    "If WARN\n"
    "    Wait 1\n"
    "    Skip login BACK\n"
    "EndIf\n"
    "\n"
    ";\n"
    "; Cleanup\n"
    "If EXISTS \"BTCLASSES:\"\n"
    "    Assign BTCLASSES: REMOVE\n"
    "EndIf\n"
    "If EXISTS \"USBCLASSES:\"\n"
    "    Assign USBCLASSES: REMOVE\n"
    "EndIf\n"
    "Assign IMAGES: REMOVE\n"
    "Assign THEME: REMOVE\n"
    "Assign THEMES: REMOVE\n"
    "Assign ENV: REMOVE\n"
    "Assign LIBS: SYS:Classes REMOVE\n"
    "\n"
    ";\n"
    "; Per-user settings: Login sets $Home and $User from the user database.\n"
    "; HOME: is the home directory of whoever uses it (a per-user assign).\n"
    "If \"$Home\" NOT EQ \"\"\n"
    "    Assign HOME: \"$Home\"\n"
    "EndIf\n"
    "Assign HOME: \"%h\" USER >NIL:\n"
    ";\n"
    "If \"$User\" NOT EQ \"\"\n"
    "    If EXISTS \"SYS:Security/Profiles/$User\"\n"
    "        Assign PROFILE: \"SYS:Security/Profiles/$User\"\n"
    "        If NOT EXISTS \"PROFILE:Env-Archive\"\n"
    "            MakeDir \"PROFILE:Env-Archive\" >NIL:\n"
    "            Copy \"SYS:Prefs/Env-Archive\" \"PROFILE:Env-Archive\" ALL CLONE QUIET\n"
    "        EndIf\n"
    "        Assign ENVARC: \"PROFILE:Env-Archive\"\n"
    "        If EXISTS \"PROFILE:S\"\n"
    "            Assign S: \"PROFILE:S\"\n"
    "            Assign S: SYS:S ADD\n"
    "        EndIf\n"
    "    EndIf\n"
    "EndIf\n"
    "EndCLI >NIL:\n";

/*
 * UserID|Password|uid|gid|UserName|HomeDir|Shell
 *
 * The standard accounts, as AROSTCP's db/passwd has them, in
 * security.library numbers (its root, 0, is 65535 and its nobody, -2, is
 * 0, as usergroup.library maps them): root without a password, and the
 * locked ('!') nobody, services and guest - nobody can log in as them
 * until root gives them a password. The same accounts, groups, names and
 * shells as the Accounts program's defaults (workbench/system/accounts,
 * DefUsers/DefGroups) in security.library's numbering: keep the two alike.
 */
static const char PasswdFile[] =
    "root||65535|65535|Super User|SYS:|shell\n"
    "nobody|!|0|0|Unprivileged|SYS:|cli\n"
    "services|!|1|1|Network Services|SYS:|cli\n"
    "guest|!|99|99|Guest|SYS:|cli\n";

/*
 * GroupID|gid|MgrUid|GroupName ... blank line ... uid:gid secondary memberships
 *
 * The standard groups, as AROSTCP's db/group has them: admin (root's
 * group), nobody, services, guests and users.
 */
static const char GroupFile[] =
    "admin|65535|65535|Administrators\n"
    "nobody|0|65535|Unprivileged\n"
    "services|1|65535|Network Services\n"
    "guests|99|65535|Guests\n"
    "users|100|65535|Users\n"
    "\n";

static const char ConfigFile[] =
    "; security.library configuration, see <libraries/security.h>\n"
    "LOGLOGIN 1\n"
    "LOGLOGINFAIL 1\n"
    "LOGSTARTUP 1\n"
    "; hardware authentication tokens (AuthKey): one plugin per scheme\n"
    "LOADPLUGIN yubikey\n"
    "LOADPLUGIN fido\n"
    "; seconds a login waits for a plugged-in token and its touch\n"
    "KEYTIMEOUT 30\n";

static BOOL Exists(CONST_STRPTR path)
{
    BPTR lock = Lock(path, ACCESS_READ);

    if (lock)
        UnLock(lock);
    return lock != BNULL;
}

static BOOL MakePath(STRPTR buf, ULONG size, CONST_STRPTR vol, CONST_STRPTR rel)
{
    if (strlen(vol) + strlen(rel) + 1 > size)
        return FALSE;
    strcpy(buf, vol);
    strcat(buf, rel);
    return TRUE;
}

static BOOL WriteFile(CONST_STRPTR path, CONST_STRPTR text, CONST_STRPTR what)
{
    BPTR fh = Open(path, MODE_NEWFILE);
    BOOL ok = FALSE;

    if (fh)
    {
        LONG len = strlen(text);

        ok = (Write(fh, (APTR)text, len) == len);
        Close(fh);
    }
    if (!ok)
    {
        Printf("AMUInit: cannot write %s (%s): ", what, path);
        PrintFault(IoErr(), NULL);
    }
    else
        Printf("AMUInit: created %s\n", path);
    return ok;
}

/* Create the file if it does not exist yet; existing databases are kept */
static BOOL EnsureFile(CONST_STRPTR path, CONST_STRPTR text, CONST_STRPTR what)
{
    if (Exists(path))
    {
        Printf("AMUInit: keeping existing %s\n", path);
        return TRUE;
    }
    return WriteFile(path, text, what);
}

static BOOL EnsureDir(CONST_STRPTR path)
{
    BPTR lock;

    if (Exists(path))
        return TRUE;
    if ((lock = CreateDir(path)))
    {
        UnLock(lock);
        Printf("AMUInit: created %s\n", path);
        return TRUE;
    }
    Printf("AMUInit: cannot create %s: ", path);
    PrintFault(IoErr(), NULL);
    return FALSE;
}

/*
 * Multi-user needs a filesystem that stores file ownership: probe the volume
 * with a scratch file before touching anything. Fails on filesystems without
 * ACTION_SET_OWNER (e.g. FAT) and when security.library is not resident (the
 * handlers only accept owners with it - multi-user could not work anyway).
 */
static BOOL VolumeSupportsOwners(CONST_STRPTR vol)
{
    char p[256];
    BPTR fh;
    BOOL ok = FALSE;

    if (!MakePath(p, sizeof(p), vol, ".AMUInit-probe"))
        return FALSE;
    if ((fh = Open(p, MODE_NEWFILE)))
    {
        Close(fh);
        ok = SetOwner(p, ((ULONG)secROOT_UID << 16) | secROOT_GID) ? TRUE : FALSE;
        DeleteFile(p);
    }
    return ok;
}

/* ------------------------------------------------------------------------ */
/* Where the databases come from                                            */

/* A growing text buffer */
struct Text
{
    STRPTR  buf;
    ULONG   len, size;
};

static BOOL Add(struct Text *t, CONST_STRPTR s, ULONG max)
{
    ULONG n = strlen(s);

    if (n > max)
        n = max;
    if (t->len + n + 1 > t->size)
    {
        ULONG size = (t->size ? t->size * 2 : 1024) + n;
        STRPTR b = AllocVec(size, MEMF_ANY);

        if (!b)
            return FALSE;
        if (t->buf)
        {
            CopyMem(t->buf, b, t->len);
            FreeVec(t->buf);
        }
        t->buf = b;
        t->size = size;
    }
    CopyMem((APTR)s, t->buf + t->len, n);
    t->len += n;
    t->buf[t->len] = '\0';
    return TRUE;
}

static BOOL AddNum(struct Text *t, ULONG v)
{
    char d[12];
    int i = sizeof(d) - 1;

    d[i] = '\0';
    do
        d[--i] = '0' + (v % 10);
    while ((v /= 10) && i);
    return Add(t, &d[i], sizeof(d));
}

/* A field without '|' and control characters */
static BOOL AddField(struct Text *t, CONST_STRPTR s, ULONG size)
{
    char f[256];
    ULONG i;

    for (i = 0; s && s[i] && i < size - 1 && i < sizeof(f) - 1; i++)
        f[i] = ((UBYTE)s[i] < 0x20 || s[i] == '|') ? '_' : s[i];
    f[i] = '\0';
    return Add(t, f, sizeof(f));
}

/* usergroup.library by vector: its stubs need a per-task base */
static void ug_setpwent(struct Library *UserGroupBase) { AROS_LC0NR(void, setpwent, struct Library *, UserGroupBase, 21, Usergroup); }
static struct passwd *ug_getpwent(struct Library *UserGroupBase) { return AROS_LC0(struct passwd *, getpwent, struct Library *, UserGroupBase, 22, Usergroup); }
static void ug_endpwent(struct Library *UserGroupBase) { AROS_LC0NR(void, endpwent, struct Library *, UserGroupBase, 23, Usergroup); }
static void ug_setgrent(struct Library *UserGroupBase) { AROS_LC0NR(void, setgrent, struct Library *, UserGroupBase, 26, Usergroup); }
static struct group *ug_getgrent(struct Library *UserGroupBase) { return AROS_LC0(struct group *, getgrent, struct Library *, UserGroupBase, 27, Usergroup); }
static void ug_endgrent(struct Library *UserGroupBase) { AROS_LC0NR(void, endgrent, struct Library *, UserGroupBase, 28, Usergroup); }

/* The network database has no group descriptions: the standard groups get
 * the ones of the defaults above, any other its name */
static CONST_STRPTR GroupDescription(CONST_STRPTR name)
{
    static const struct { CONST_STRPTR name, desc; } std[] =
    {
        { "admin",    "Administrators"   },
        { "nobody",   "Unprivileged"     },
        { "services", "Network Services" },
        { "guests",   "Guests"           },
        { "users",    "Users"            },
    };
    ULONG i;

    for (i = 0; i < sizeof(std) / sizeof(std[0]); i++)
        if (!strcmp(name, std[i].name))
            return std[i].desc;
    return name;
}

/* network ids are POSIX ones: root is 0 and nobody -2 */
static LONG SecId(LONG id, LONG root, LONG nobody)
{
    if (id == 0)
        return root;
    if (id == -2)
        return nobody;
    return id;
}

#define MAXCONV     1024    /* accounts and groups converted */
#define MAXSEC      32      /* secondary groups per account */

struct ConvUser
{
    char    name[secUSERIDSIZE];
    UWORD   uid, gid;
    UWORD   ngroups;
    UWORD   groups[MAXSEC];
};

/*
 * A single-user system: the network database of usergroup.library
 * (netinfo.device: AROSTCP's db/passwd and db/group) becomes the multi-user
 * one. Its crypt() hashes mean nothing to security.library, so an account
 * that has a password is locked ('!'), as are those that cannot log in
 * there ('*'); root gets none, so that it can log in and set them with
 * Passwd. Ids become security.library's (root 65535, nobody 0); an id
 * that is taken (AROSTCP's guest shares uid 1 with daemon) becomes the
 * account's group id if that is free. FALSE if there is nothing to convert.
 */
static BOOL ConvertNetwork(struct Text *pw, struct Text *gr)
{
    struct Library *UserGroupBase;
    struct ConvUser *users;
    struct passwd *p;
    struct group *g;
    ULONG nusers = 0, ngroups = 0, i, j;
    BOOL ok = FALSE, haveroot = FALSE, haverootgroup = FALSE, locked = FALSE;

    if (!(UserGroupBase = OpenLibrary("usergroup.library", 4)))
        return FALSE;
    if (!(users = AllocVec(MAXCONV * sizeof(struct ConvUser), MEMF_CLEAR)))
    {
        CloseLibrary(UserGroupBase);
        return FALSE;
    }

    ug_setpwent(UserGroupBase);
    while ((p = ug_getpwent(UserGroupBase)) && nusers < MAXCONV)
    {
        LONG uid, gid = SecId((LONG)p->pw_gid, secROOT_GID, secNOBODY_GID);
        BOOL taken;

        /* uid -1: AROSTCP's version string */
        if (!p->pw_name || !p->pw_name[0] || (LONG)p->pw_uid == -1 || strchr(p->pw_name, '|') ||
            strchr(p->pw_name, ':') || strlen(p->pw_name) >= secUSERIDSIZE)
            continue;
        uid = SecId((LONG)p->pw_uid, secROOT_UID, secNOBODY_UID);
        for (taken = FALSE, j = 0; j < nusers; j++)
            if (users[j].uid == uid)
                taken = TRUE;
        if (taken)
        {
            for (taken = FALSE, j = 0; j < nusers; j++)
                if (users[j].uid == gid)
                    taken = TRUE;
            if (taken || gid == secROOT_UID)
            {
                Printf("AMUInit: '%s' shares its user id with another account: left out\n", p->pw_name);
                continue;
            }
            uid = gid;
        }
        if (uid < 0 || uid > 65535 || gid < 0 || gid > 65535)
            continue;

        strcpy(users[nusers].name, p->pw_name);
        users[nusers].uid = uid;
        users[nusers].gid = gid;
        if (uid == secROOT_UID)
            haveroot = TRUE;

        ok = AddField(pw, p->pw_name, secUSERIDSIZE) && Add(pw, "|", 1);
        if (uid != secROOT_UID)
        {
            /* no password stays none; anything else is locked */
            if (p->pw_passwd && p->pw_passwd[0])
            {
                ok = ok && Add(pw, "!", 1);
                if (p->pw_passwd[0] != '*' && p->pw_passwd[0] != '!')
                {
                    Printf("AMUInit: '%s' has a password, which cannot be converted: the account is locked\n", p->pw_name);
                    locked = TRUE;
                }
            }
        }
        ok = ok && Add(pw, "|", 1) && AddNum(pw, uid) && Add(pw, "|", 1) && AddNum(pw, gid) && Add(pw, "|", 1) &&
             AddField(pw, p->pw_gecos, secUSERNAMESIZE) && Add(pw, "|", 1) &&
             AddField(pw, (p->pw_dir && p->pw_dir[0]) ? p->pw_dir : "SYS:", secHOMEDIRSIZE) && Add(pw, "|", 1) &&
             AddField(pw, p->pw_shell, secSHELLSIZE) && Add(pw, "\n", 1);
        if (!ok)
            break;
        nusers++;
    }
    ug_endpwent(UserGroupBase);

    if (ok && nusers)
    {
        if (!haveroot)
            ok = Add(pw, "root||65535|65535|Super User|SYS:|shell\n", 64);

        ug_setgrent(UserGroupBase);
        while (ok && (g = ug_getgrent(UserGroupBase)))
        {
            LONG gid;
            char **m;

            if (!g->gr_name || !g->gr_name[0] || (LONG)g->gr_gid == -1 || strchr(g->gr_name, '|') ||
                strchr(g->gr_name, ':') || strlen(g->gr_name) >= secGROUPIDSIZE)
                continue;
            gid = SecId((LONG)g->gr_gid, secROOT_GID, secNOBODY_GID);
            if (gid < 0 || gid > 65535)
                continue;
            if (gid == secROOT_GID)
                haverootgroup = TRUE;
            ok = AddField(gr, g->gr_name, secGROUPIDSIZE) && Add(gr, "|", 1) && AddNum(gr, gid) &&
                 Add(gr, "|65535|", 7) && AddField(gr, GroupDescription(g->gr_name), secGROUPNAMESIZE) && Add(gr, "\n", 1);
            ngroups++;
            /* the members: secondary groups of the accounts */
            for (m = g->gr_mem; ok && m && *m; m++)
                for (j = 0; j < nusers; j++)
                    if (!strcmp(users[j].name, *m) && users[j].gid != gid && users[j].ngroups < MAXSEC)
                        users[j].groups[users[j].ngroups++] = gid;
        }
        ug_endgrent(UserGroupBase);
        if (ok && !haverootgroup)
            ok = Add(gr, "admin|65535|65535|Administrators\n", 64);
        if (ok)
            ok = Add(gr, "\n", 1);
        for (i = 0; ok && i < nusers; i++)
        {
            if (!users[i].ngroups)
                continue;
            ok = AddNum(gr, users[i].uid) && Add(gr, ":", 1);
            for (j = 0; ok && j < users[i].ngroups; j++)
                ok = (j == 0 || Add(gr, ",", 1)) && AddNum(gr, users[i].groups[j]);
            ok = ok && Add(gr, "\n", 1);
        }
        if (ok)
        {
            Printf("AMUInit: the databases are made from the network database (%lu accounts, %lu groups)\n",
                   (unsigned long)nusers, (unsigned long)ngroups);
            if (locked)
                PutStr("AMUInit: log in as root after the reboot and give the locked accounts passwords with Passwd\n");
        }
    }
    else
        ok = FALSE;

    FreeVec(users);
    CloseLibrary(UserGroupBase);
    return ok;
}

/* Read a whole file into t; FALSE if it cannot be read */
static BOOL ReadAll(CONST_STRPTR path, struct Text *t)
{
    char buf[512];
    BPTR fh;
    LONG n;
    BOOL ok = TRUE;

    if (!(fh = Open(path, MODE_OLDFILE)))
        return FALSE;
    while (ok && (n = Read(fh, buf, sizeof(buf) - 1)) > 0)
    {
        buf[n] = '\0';
        ok = Add(t, buf, n);
    }
    if (n < 0)
        ok = FALSE;
    Close(fh);
    return ok && t->len > 0;
}

/* A multi-user system: the databases in use (only root can read the
 * password file). FALSE if they cannot be read. */
static BOOL CopyCurrent(struct Text *pw, struct Text *gr)
{
    char p[256];
    BPTR lock;
    BOOL ok = FALSE;

    if ((lock = secGetPasswdDirLock()))
    {
        ok = NameFromLock(lock, p, sizeof(p)) && AddPart(p, secPasswd_FileName, sizeof(p)) && ReadAll(p, pw);
        UnLock(lock);
    }
    if (ok && (lock = secGetConfigDirLock()))
    {
        ok = NameFromLock(lock, p, sizeof(p)) && AddPart(p, secGroup_FileName, sizeof(p)) && ReadAll(p, gr);
        UnLock(lock);
    }
    else
        ok = FALSE;
    if (ok)
        PutStr("AMUInit: the databases are copied from the ones in use\n");
    else
        PutStr("AMUInit: the databases in use cannot be read (log in as root): the defaults are used\n");
    return ok;
}

static int MUEnable(CONST_STRPTR vol, BOOL force)
{
    char p[256], p2[256];
    BOOL copied = FALSE;

    if (!VolumeSupportsOwners(vol))
    {
        Printf("AMUInit: %s does not support file ownership (SetOwner) - multi-user cannot be enabled on it\n", vol);
        return RETURN_FAIL;
    }

    /* the database directory and its files */
    if (!MakePath(p, sizeof(p), vol, SEC_DIR) || !EnsureDir(p))
        return RETURN_FAIL;
    /* The databases: those in use on a multi-user system, the network
     * database's on a single-user one, the defaults when there is none */
    {
        struct Text pw = { NULL, 0, 0 }, gr = { NULL, 0, 0 };
        CONST_STRPTR pwtext = PasswdFile, grtext = GroupFile;
        char p2[256];
        BOOL ok;

        MakePath(p, sizeof(p), vol, SEC_DIR "/passwd");
        MakePath(p2, sizeof(p2), vol, SEC_DIR "/group");
        if (!Exists(p) || !Exists(p2))
        {
            BOOL multi = FALSE;

            if (FindResident(SECURITYNAME) && (secBase = OpenLibrary(SECURITYNAME, 45)))
                multi = secIsConfigured();
            if (multi ? CopyCurrent(&pw, &gr) : ConvertNetwork(&pw, &gr))
            {
                pwtext = pw.buf;
                grtext = gr.buf;
                copied = multi;
            }
            else if (!multi)
                PutStr("AMUInit: there is no network database to take the accounts from: the defaults are used\n");
            if (secBase)
                CloseLibrary(secBase);
            secBase = NULL;
        }
        ok = EnsureFile(p, pwtext, "user database") && EnsureFile(p2, grtext, "group database");
        if (pw.buf)
            FreeVec(pw.buf);
        if (gr.buf)
            FreeVec(gr.buf);
        if (!ok)
            return RETURN_FAIL;
    }
    if (!MakePath(p, sizeof(p), vol, SEC_DIR "/" secConfig_FileName) || !EnsureFile(p, ConfigFile, "configuration"))
        return RETURN_FAIL;
    if (!MakePath(p, sizeof(p), vol, SEC_DIR "/Security.log") || !EnsureFile(p, "", "log file"))
        return RETURN_FAIL;
    if (!MakePath(p, sizeof(p), vol, SEC_DIR "/PAM") || !EnsureDir(p))
        return RETURN_FAIL;
    if (!MakePath(p, sizeof(p), vol, SEC_DIR "/Keys") || !EnsureDir(p))
        return RETURN_FAIL;

    /* the startup script */
    if (!MakePath(p, sizeof(p), vol, "S") || !EnsureDir(p))
        return RETURN_FAIL;
    MakePath(p, sizeof(p), vol, STARTUP);
    MakePath(p2, sizeof(p2), vol, STARTUP_OFF);
    if (Exists(p) && !force)
        Printf("AMUInit: keeping existing %s (FORCE rewrites it)\n", p);
    else if (!force && Exists(p2) && Rename(p2, p))
        Printf("AMUInit: re-enabled %s\n", p);
    else
    {
        if (Exists(p2))
            DeleteFile(p2);
        if (!WriteFile(p, StartupScript, "startup script"))
            return RETURN_FAIL;
    }

    if (copied)
        Printf("\nMulti-user support is enabled on %s: the next boot from it shows the login\n"
               "prompt, for the accounts of this system.\n", vol);
    else
        Printf("\nMulti-user support is enabled on %s: the next boot from it shows the login\n"
               "prompt. The super user is \"root\" and has no password yet: log in as root\n"
               "and run Passwd, then add users with SYS:System/Accounts.\n", vol);
    return RETURN_OK;
}

/* The version in a script's "$VER: Security-Startup v.r" line; 0 if none */
static ULONG ScriptVersion(CONST_STRPTR path)
{
    char line[160];
    BPTR fh;
    ULONG v = 0, r = 0;

    if (!(fh = Open(path, MODE_OLDFILE)))
        return 0;
    while (FGets(fh, line, sizeof(line) - 1))
    {
        char *p = strstr(line, "$VER: Security-Startup ");
        if (p)
        {
            p += strlen("$VER: Security-Startup ");
            while (*p >= '0' && *p <= '9')
                v = v * 10 + (*p++ - '0');
            if (*p == '.')
                for (p++; *p >= '0' && *p <= '9'; p++)
                    r = r * 10 + (*p - '0');
            break;
        }
    }
    Close(fh);
    return (v << 16) | (r & 0xffff);
}

static int MUUpgrade(CONST_STRPTR vol, BOOL force)
{
    char p[256], p2[256];
    ULONG have, want = ((ULONG)STARTUP_VERSION << 16) | STARTUP_REVISION;

    MakePath(p, sizeof(p), vol, STARTUP);
    if (!Exists(p))
    {
        Printf("AMUInit: %s is not enabled for multi-user boots (no %s)\n", vol, p);
        return RETURN_WARN;
    }
    if (MakePath(p2, sizeof(p2), vol, SEC_DIR "/PAM"))
        EnsureDir(p2);
    if (MakePath(p2, sizeof(p2), vol, "Libs/PAM"))
        EnsureDir(p2);
    if (MakePath(p2, sizeof(p2), vol, SEC_DIR "/Keys"))
        EnsureDir(p2);

    have = ScriptVersion(p);
    if (have >= want && !force)
    {
        Printf("AMUInit: %s is up to date (%lu.%lu)\n", p, (unsigned long)(have >> 16), (unsigned long)(have & 0xffff));
        return RETURN_OK;
    }
    MakePath(p2, sizeof(p2), vol, STARTUP ".old");
    if (Exists(p2))
        DeleteFile(p2);
    if (!Rename(p, p2))
    {
        Printf("AMUInit: cannot rename %s: ", p);
        PrintFault(IoErr(), NULL);
        return RETURN_FAIL;
    }
    if (!WriteFile(p, StartupScript, "startup script"))
    {
        Rename(p2, p);
        return RETURN_FAIL;
    }
    Printf("AMUInit: the previous script is kept as %s; carry any changes of your own over to\n"
           "the new one.\n", p2);
    return RETURN_OK;
}

static int MUDisable(CONST_STRPTR vol)
{
    char p[256], p2[256];

    MakePath(p, sizeof(p), vol, STARTUP);
    MakePath(p2, sizeof(p2), vol, STARTUP_OFF);
    if (!Exists(p))
    {
        Printf("AMUInit: %s is not enabled for multi-user boots (no %s)\n", vol, p);
        return RETURN_WARN;
    }
    if (Exists(p2))
        DeleteFile(p2);
    if (!Rename(p, p2))
    {
        Printf("AMUInit: cannot rename %s: ", p);
        PrintFault(IoErr(), NULL);
        return RETURN_FAIL;
    }
    Printf("Multi-user support is disabled on %s: the next boot from it is single-user.\n"
           "The databases in %s%s are kept; AMUInit ENABLE turns it on again.\n", vol, vol, SEC_DIR);
    return RETURN_OK;
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    char vol[128];
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "AMUInit");
        return RETURN_FAIL;
    }

    if ((args[ARG_ENABLE] ? 1 : 0) + (args[ARG_DISABLE] ? 1 : 0) + (args[ARG_UPGRADE] ? 1 : 0) != 1)
        PutStr("AMUInit: give one of ENABLE, DISABLE or UPGRADE\n");
    else
    {
        /* normalise the volume: "SYS", "SYS:" or "SYS:anything" -> "SYS:" */
        CONST_STRPTR v = (CONST_STRPTR)args[ARG_VOLUME];
        ULONG n = 0;

        while (v[n] && v[n] != ':' && n < sizeof(vol) - 2)
            n++;
        memcpy(vol, v, n);
        vol[n] = ':';
        vol[n + 1] = '\0';

        if (!Exists(vol))
        {
            Printf("AMUInit: cannot access %s: ", vol);
            PrintFault(IoErr(), NULL);
        }
        else if (args[ARG_ENABLE])
            rc = MUEnable(vol, args[ARG_FORCE] ? TRUE : FALSE);
        else if (args[ARG_UPGRADE])
            rc = MUUpgrade(vol, args[ARG_FORCE] ? TRUE : FALSE);
        else
            rc = MUDisable(vol);
    }

    FreeArgs(rda);
    return rc;
}
