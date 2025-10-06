/*
 * passwdunit.c --- unit for passwd entries
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "entries.h"

#include <string.h>
#if defined(DEBUG)
#include <assert.h>
#include "assert.h"
#endif

static void *CopyEntToPasswd(struct NetInfoReq *req, struct Ent *e);
static struct Ent *CopyPasswdToEnt(struct NetInfoDevice *nid, struct NetInfoReq *req);
static struct Ent *ParsePasswd(struct NetInfoDevice *nid, register UBYTE *p);
static void GetMembers(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);
static int PrintPasswd(struct NetInfoDevice *nid, BPTR file, struct Ent *e);

const static struct MapMethods passwd_methods[1] = {
    {
        ParsePasswd,
        PrintPasswd,
        CopyEntToPasswd,
        CopyPasswdToEnt,
        EntCleanup,
        NULL,
        EntHandleNotify,
    },
};

/*
 * Allocate and initialize passwd map
 */
struct NetInfoMap *InitPasswdMap(struct NetInfoDevice *nid)
{
    D(bug("[NetInfo] %s()\n", __func__));

    struct NetInfoMap *nim = NULL;
    if (nid->nid_dbuser) {
        nim = AllocVec(sizeof(*nim), MEMF_CLEAR|MEMF_PUBLIC);
        if (nim) {
            nim->nim_Methods = passwd_methods;
            nim->nim_Name = "passwd";
            nim->nim_Filename = nid->nid_dbuser;
        }
    }

    return nim;
}

/*
 * Copy an PasswdEnt structure to passwd structure
 */
static void *CopyEntToPasswd(struct NetInfoReq *req, struct Ent *e)
{
    D(bug("[NetInfo] %s()\n", __func__));

    struct PasswdEnt *pe = (struct PasswdEnt *)e;
    struct NetInfoPasswd *pw = req->io_Data;
    UBYTE  *to = (UBYTE *)(pw + 1);

    ULONG size = req->io_Length;
    ULONG actual = sizeof(*pw) + pe->pe_tlen;

    req->io_Actual = actual;

    if (size < actual) {
        return NULL;
    }

    /* copy strings */
    to = stpcopy(pw->pw_name   = to, pe->pe_passwd->pw_name);
    to = stpcopy(pw->pw_passwd = to, pe->pe_passwd->pw_passwd);
    to = stpcopy(pw->pw_gecos  = to, pe->pe_passwd->pw_gecos);
    to = stpcopy(pw->pw_dir    = to, pe->pe_passwd->pw_dir);
    to = stpcopy(pw->pw_shell  = to, pe->pe_passwd->pw_shell);

#if defined(DEBUG)
    assert(to == (UBYTE *)req->io_Data + actual);
#endif
    /* .. and ids */
    pw->pw_uid = pe->pe_passwd->pw_uid;
    pw->pw_gid = pe->pe_passwd->pw_gid;

    return pw;
}

/*
 * Copy an passwd structure into a internal entry
 */
static struct Ent *CopyPasswdToEnt(struct NetInfoDevice *nid, struct NetInfoReq *req)
{
    struct NetInfoPasswd *pw = (struct NetInfoPasswd*)req->io_Data;
    struct PasswdEnt *pe;
    UBYTE *to;
    ULONG txtlen;

    D(bug("[NetInfo] %s()\n", __func__));

    /*
    * These cause EFAULT
    */
    if (pw == NULL || (1 & (IPTR)pw) != 0 ||
            pw->pw_name == NULL ||
            pw->pw_passwd == NULL ||
            pw->pw_gecos == NULL ||
            pw->pw_dir == NULL ||
            pw->pw_shell == NULL) {
        req->io_Error = IOERR_BADADDRESS;
        return NULL;
    }

    txtlen = strlen(pw->pw_name) + strlen(pw->pw_passwd) + strlen(pw->pw_gecos) +
             strlen(pw->pw_dir) + strlen(pw->pw_shell) + 5;

    pe = AllocVec(sizeof(*pe) + txtlen, MEMF_CLEAR);
    if (pe == NULL) {
        req->io_Error = NIERR_NOMEM;
        return NULL;
    }

    pe->pe_node.ln_Type = ENT_PASSWD;
    pe->pe_node.ln_Pri  = ENT_CHANGED;
    pe->pe_tlen = txtlen;
    pe->pe_passwd->pw_uid = pw->pw_uid;
    pe->pe_passwd->pw_gid = pw->pw_gid;

    to = (UBYTE *)(pe + 1);
    to = stpcopy(pe->pe_passwd->pw_name   = to, pw->pw_name);
    to = stpcopy(pe->pe_passwd->pw_passwd = to, pw->pw_passwd);
    to = stpcopy(pe->pe_passwd->pw_gecos  = to, pw->pw_gecos);
    to = stpcopy(pe->pe_passwd->pw_dir    = to, pw->pw_dir);
    to = stpcopy(pe->pe_passwd->pw_shell  = to, pw->pw_shell);

#if defined(DEBUG)
    assert(to == pe->pe_passwd->pw_name + txtlen);
#endif
    return (struct Ent *)pe;
}

/****** netinfo.device/passwd **********************************************

   NAME
        passwd - format of the password file

   DESCRIPTION

        The passwd files are files consisting of newline separated records,
        one per user, containing seven bar (`|') separated fields.  These
        fields are as follows:

            name      User's login name.

            password  User's encrypted password.

            uid       User's id.

            gid       User's login group id.

            gecos     General information about the user.

            home_dir  User's home directory.

            shell     User's login shell.

        The name field is the login used to access the computer account, and
        the uid field is the number associated with it.  They should both be
        unique across the system (and often across a group of systems) since
        they control file access.

        While it is possible to have multiple entries with identical login
        names and/or identical user id's, it is usually a mistake to do so.
        Routines that manipulate these files will often return only one of
        the multiple entries, and that one by random selection.

        The login name must never begin with a hyphen (`-'); also, it is
        strongly suggested that neither upper-case characters or dots (`.')
        be part of the name, as this tends to confuse mailers. No field may
        contain a bar (`|') as this has been used to separate the fields in
        the user database.

        The password field is the encrypted form of the password. The actual
        format is not restricted by netinfo.device, but usergroup.library
        uses same format as Version 7 UNIX (POSIX) or 4.4BSD.  The format
        used by MultiUser 1.5 or AS225r2ß is also possible, but its use is
        strongly discouraged.

        If the password field is empty, no password will be required to gain
        access to the machine.  This is almost invariably a mistake. Because
        these files contain the encrypted user passwords, they should not be
        readable by anyone without appropriate privileges.

        The group field is the group that the user will be placed in upon
        login.

        The gecos field normally contains comma (`,') separated subfields as
        follows:

            name           user's full name
            office         user's office number
            wphone         user's work phone number
            hphone         user's home phone number

        This information is used by the finger program.

        The user's home directory is the full DOS path name where the user
        will be placed on login.

        The shell field is the command interpreter the user prefers. If
        there is nothing in the shell field, the CLI is assumed.

   FILES
        AmiTCP:db/passwd

   SEE ALSO
        group

   COPYRIGHT
        Copyright 1980, 1991 The Regents of the University of California.
        Copyright 1993, 1994 AmiTCP/IP-Group, <AmiTCP-Group@hut.fi>,
        Helsinki University of Technology, Helsinki

   HISTORY
        A passwd file format appeared in Version 6 AT&T UNIX.

        The used format is basically the same as in the Multiuser Library
        1.3 and AS225r2 use, except the password field.

****************************************************************************
*/

/*
 * Parse a passwd database entry
 */
static struct Ent *ParsePasswd(struct NetInfoDevice *nid, register UBYTE *p)
{
    int i;
    UBYTE *to, *np = p, *field[PASSWDFIELDS];
    ULONG txtlen;
    LONG uid, gid;
    struct PasswdEnt *pe;

    D(bug("[NetInfo] %s()\n", __func__));

    for (i = 0;
            i < PASSWDFIELDS && (field[i++] = strsep((char **)&np, "|\n")) && np && *np;)
        ;

#if defined(DEBUG)
    assert(np != NULL);
#endif
    if (i < PASSWDFIELDS || !np) {
        if (i < PASSWDFIELDS) {
            bug("[NetInfo] %s: ERROR: malformed enry\n", __func__);
        } else {
            bug("[NetInfo] %s: ERROR: no line-end?\n", __func__);
        }
        return NULL;
    }

    if ((i = StrToLong(field[2], &uid)) <= 0 || field[2][i] != '\0') {
        bug("[NetInfo] %s: ERROR: failed to parse uid\n", __func__);
        return NULL;
    }
    if ((i = StrToLong(field[3], &gid)) <= 0 || field[3][i] != '\0') {
        bug("[NetInfo] %s: ERROR: failed to parse gid\n", __func__);
        return NULL;
    }

    txtlen = np - field[0] - (field[4] - field[2]);
    pe = AllocVec(sizeof(*pe) + txtlen, MEMF_CLEAR);
    if (!pe) {
        bug("[NetInfo] %s: Failed to allocate entry storage\n", __func__);
        return NULL;
    }

    pe->pe_node.ln_Type = ENT_PASSWD;
    pe->pe_tlen = txtlen;
    pe->pe_passwd->pw_uid = uid;
    pe->pe_passwd->pw_gid = gid;

    to = (UBYTE *)(pe + 1);
    to = stpcopy(pe->pe_passwd->pw_name   = to, field[0]);
    to = stpcopy(pe->pe_passwd->pw_passwd = to, field[1]);
    to = stpcopy(pe->pe_passwd->pw_gecos  = to, field[4]);
    to = stpcopy(pe->pe_passwd->pw_dir    = to, field[5]);
    to = stpcopy(pe->pe_passwd->pw_shell  = to, field[6]);

    D(bug("[NetInfo] %s: <%ld:%ld> '%s'\n", __func__, pe->pe_passwd->pw_uid, pe->pe_passwd->pw_gid, pe->pe_passwd->pw_name));

#if defined(DEBUG)
    assert(to == pe->pe_passwd->pw_name + txtlen);
#endif
    return (struct Ent *)pe;
}

/*
 * Print out an passwd entry
 */
static int PrintPasswd(struct NetInfoDevice *nid, BPTR file, struct Ent *e)
{
    struct PasswdEnt *pe = (struct PasswdEnt *)e;

    D(bug("[NetInfo] %s()\n", __func__));

    if (VFPrintf(file, "%s|%s|%ld|%ld|%s|%s|%s\n", (RAWARG)pe->pe_passwd) == -1)
        return NIERR_ACCESS;
    else
        return 0;
}

/*
 * Find all UIDs in the named group
 */
static void GetMembers(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    D(bug("[NetInfo] %s()\n", __func__));

    req->io_Error = NIERR_NOTFOUND;
    TermIO(req);
}
