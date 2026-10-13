/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: usergroup.library smoke test for the multi-user system. Prints the
          caller's credentials as seen through usergroup.library, which must
          match security.library when that is resident.

          Usage: UGTest [outfile]
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/usergroup.h>
#include <libraries/usergroup.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>

static BPTR out;

static void Out(CONST_STRPTR fmt, ...)
{
    char line[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    bug("[UGTest] %s", line);
    if (out)
        FPuts(out, line);
}

int main(int argc, char **argv)
{
    struct passwd *pw;
    struct group *gr;
    struct UserGroupCredentials *cr;
    gid_t groups[NGROUPS];
    int n, i;
    char *login;

    out = (argc > 1) ? Open(argv[1], MODE_NEWFILE) : BNULL;

    Out("--- usergroup.library as seen by this task ---\n");
    Out("uid %d euid %d gid %d egid %d umask %03o\n",
        (int)getuid(), (int)geteuid(), (int)getgid(), (int)getegid(), (unsigned)getumask());

    n = getgroups(NGROUPS, groups);
    Out("groups (%d):", n);
    for (i = 0; i < n; i++)
        Out(" %d", (int)groups[i]);
    Out("\n");

    login = getlogin();
    Out("getlogin: '%s'\n", login ? login : "(null)");

    pw = getpwuid(getuid());
    if (pw)
        Out("getpwuid(%d): name '%s' uid %d gid %d gecos '%s' dir '%s' shell '%s'\n",
            (int)getuid(), pw->pw_name, (int)pw->pw_uid, (int)pw->pw_gid,
            pw->pw_gecos ? pw->pw_gecos : "", pw->pw_dir ? pw->pw_dir : "",
            pw->pw_shell ? pw->pw_shell : "");
    else
        Out("getpwuid(%d): NULL (err %d)\n", (int)getuid(), ug_GetErr());

    pw = getpwnam("root");
    Out("getpwnam(root): %s uid %d\n", pw ? pw->pw_name : "NULL", pw ? (int)pw->pw_uid : -1);
    pw = getpwnam("nosuchuser");
    Out("getpwnam(nosuchuser): %s\n", pw ? "found?!" : "NULL (ok)");

    gr = getgrgid(getgid());
    if (gr)
    {
        Out("getgrgid(%d): name '%s' members:", (int)getgid(), gr->gr_name);
        for (i = 0; gr->gr_mem && gr->gr_mem[i]; i++)
            Out(" %s", gr->gr_mem[i]);
        Out("\n");
    }
    else
        Out("getgrgid(%d): NULL (err %d)\n", (int)getgid(), ug_GetErr());

    Out("passwd database:");
    setpwent();
    while ((pw = getpwent()) != NULL)
        Out(" %s(%d:%d)", pw->pw_name, (int)pw->pw_uid, (int)pw->pw_gid);
    endpwent();
    Out("\n");

    Out("group database:");
    setgrent();
    while ((gr = getgrent()) != NULL)
        Out(" %s(%d)", gr->gr_name, (int)gr->gr_gid);
    endgrent();
    Out("\n");

    cr = getcredentials(NULL);
    if (cr)
        Out("getcredentials: ruid %d euid %d rgid %d ngroups %d login '%s' umask %03o\n",
            (int)cr->cr_ruid, (int)cr->cr_euid, (int)cr->cr_rgid, (int)cr->cr_ngroups,
            cr->cr_login, (unsigned)cr->cr_umask);

    /* a non-root user may not become root; root may become anybody */
    Out("setuid(0): %s\n", setuid(0) == 0 ? "OK" : "denied (ok for non-root)");
    Out("now uid %d euid %d\n", (int)getuid(), (int)geteuid());

    if (out)
        Close(out);
    return RETURN_OK;
}
