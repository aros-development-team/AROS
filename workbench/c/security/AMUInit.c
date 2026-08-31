/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AMUInit - AROS Multi-User Init: enable or disable multi-user
          behaviour on a volume (security.library).
*/

/*****************************************************************************

    NAME
        AMUInit

    SYNOPSIS
        ENABLE/S,DISABLE/S,VOLUME/A,FORCE/S

    LOCATION
        C:

    FUNCTION
        Prepares a (boot) volume for the multi-user system provided by
        security.library, or turns it off again.

        ENABLE creates the security database directory <VOLUME>Security
        with an initial user database (passwd: the super user "root" without
        a password), group database (group: "root" and "users") and
        configuration (Security.config), and installs <VOLUME>S/Security-Startup.
        That script is run by dos.library before the Startup-Sequence when
        security.library is part of the ROM; it shows the login prompt, and
        the Startup-Sequence then runs as the user who logged in.

        Existing database files are never touched (use FORCE to rewrite the
        startup script). Log in as root after the reboot and run Passwd to
        give root a password, and add users to the database.

        DISABLE renames <VOLUME>S/Security-Startup to Security-Startup.disabled
        so that the volume boots as a single-user system again; the databases
        are kept. ENABLE restores a disabled script.

    INPUTS
        ENABLE   -- set the volume up for multi-user boots.
        DISABLE  -- boot single-user again.
        VOLUME   -- the volume (or any path on it), e.g. SYS: or DH0:.
        FORCE    -- ENABLE: rewrite an existing Security-Startup.

    RESULT
        Standard DOS return codes.

    NOTES
        The change takes effect at the next boot of that volume. Only the
        booting volume's Security directory (SYS:Security) is used by the
        library.

    EXAMPLE
        AMUInit ENABLE SYS:
        AMUInit DISABLE SYS:

    SEE ALSO
        Login, Logout, Passwd, Who, SetOwner

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <libraries/security.h>
#include <string.h>

const TEXT version[] = "$VER: AMUInit 45.1 (29.08.2026)";

#define TEMPLATE "ENABLE/S,DISABLE/S,VOLUME/A,FORCE/S"
enum { ARG_ENABLE, ARG_DISABLE, ARG_VOLUME, ARG_FORCE, ARG_COUNT };

#define SEC_DIR         "Security"
#define STARTUP         "S/Security-Startup"
#define STARTUP_OFF     "S/Security-Startup.disabled"

static const char StartupScript[] =
    "; $VER: Security-Startup 45.6 (30.08.2026)\n"
    ";\n"
    "; Installed by AMUInit ENABLE. dos.library runs this script before\n"
    "; S:Startup-Sequence when security.library is part of the ROM: it shows\n"
    "; the login prompt, and the Startup-Sequence then runs as the user who\n"
    "; logged in. 'AMUInit DISABLE SYS:' boots single-user again.\n"
    ";\n"
    "; This runs in a shell on the boot console and MUST end with EndCLI.\n"
    ";\n"
    "; The Startup-Sequence has not made the assigns yet: the graphical login\n"
    "; (Zune) needs LIBS: for muimaster.library and the classes. It is removed\n"
    "; again below so that the Startup-Sequence starts as usual.\n"
    "\n"
    "Assign LIBS: SYS:Libs\n"
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
    "Assign T: REMOVE\n"
    "If EXISTS \"RAM:T\"\n"
    "    Delete \"RAM:T\" FORCE QUIET ALL\n"
    "EndIf\n"
    "Assign LIBS: REMOVE\n"
    "\n"
    ";\n"
    "; Per-user settings: Login sets $Home and $User from the user database.\n"
    "If \"$Home\" NOT EQ \"\"\n"
    "    Assign HOME: \"$Home\"\n"
    "EndIf\n"
    ";\n"
    "; A user profile in SYS:Security/Profiles/<user>/ provides its own ENVARC\n"
    "; and S directories; without them the system ones stay in use.\n"
    "If \"$User\" NOT EQ \"\"\n"
    "    If EXISTS \"SYS:Security/Profiles/$User/ENVARC\"\n"
    "        Assign ENVARC: \"SYS:Security/Profiles/$User/ENVARC\"\n"
    "    EndIf\n"
    "    If EXISTS \"SYS:Security/Profiles/$User/S\"\n"
    "        Assign S: \"SYS:Security/Profiles/$User/S\"\n"
    "        Assign S: SYS:S ADD\n"
    "    EndIf\n"
    "EndIf\n"
    "EndCLI >NIL:\n";

/* UserID|Password|uid|gid|UserName|HomeDir|Shell  (root: 65535, no password) */
static const char PasswdFile[] =
    "root||65535|65535|Super User|SYS:|\n";

/* GroupID|gid|MgrUid|GroupName ... blank line ... uid:gid secondary memberships */
static const char GroupFile[] =
    "root|65535|65535|Root Group\n"
    "users|100|65535|Users\n"
    "\n";

static const char ConfigFile[] =
    "; security.library configuration, see <libraries/security.h>\n"
    "LOGLOGIN 1\n"
    "LOGLOGINFAIL 1\n"
    "LOGSTARTUP 1\n";

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

static int MUEnable(CONST_STRPTR vol, BOOL force)
{
    char p[256], p2[256];

    /* the database directory and its files */
    if (!MakePath(p, sizeof(p), vol, SEC_DIR) || !EnsureDir(p))
        return RETURN_FAIL;
    if (!MakePath(p, sizeof(p), vol, SEC_DIR "/passwd") || !EnsureFile(p, PasswdFile, "user database"))
        return RETURN_FAIL;
    if (!MakePath(p, sizeof(p), vol, SEC_DIR "/group") || !EnsureFile(p, GroupFile, "group database"))
        return RETURN_FAIL;
    if (!MakePath(p, sizeof(p), vol, SEC_DIR "/" secConfig_FileName) || !EnsureFile(p, ConfigFile, "configuration"))
        return RETURN_FAIL;
    if (!MakePath(p, sizeof(p), vol, SEC_DIR "/Security.log") || !EnsureFile(p, "", "log file"))
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

    Printf("\nMulti-user support is enabled on %s: the next boot from it shows the login\n"
           "prompt. The super user is \"root\" and has no password yet: log in as root\n"
           "and run Passwd, then add users to %s%s/passwd.\n", vol, vol, SEC_DIR);
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

    if ((args[ARG_ENABLE] && args[ARG_DISABLE]) || (!args[ARG_ENABLE] && !args[ARG_DISABLE]))
        PutStr("AMUInit: give either ENABLE or DISABLE\n");
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
        else
            rc = MUDisable(vol);
    }

    FreeArgs(rda);
    return rc;
}
