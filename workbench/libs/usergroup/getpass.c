/*
 * getpass() --- password checking routine
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

#include <aros/libcall.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <sys/time.h>

#include "base.h"

#include <proto/usergroup.h>

#define EOF (-1)

/*****************************************************************************

    NAME */

        AROS_LH1I(char *, getpass,

/*  SYNOPSIS */
        AROS_LHA(const char *, prompt, A1),

/* LOCATION */
        struct Library *, UserGroupBase, 31, Usergroup)

/*  FUNCTION
        The getpass() function displays a prompt to, and reads in a password
        from "CONSOLE:".  If this device is not accessible, getpass()
        displays the prompt on the standard error output and reads from the
        standard input.

        The password may be up to _PASSWORD_LEN (currently 128) characters
        in length.  Any additional characters and the terminating newline
        character are discarded.

        Getpass turns off character echoing while reading the password.

    RESULT
        password -  a pointer to the null terminated password

    FILES
        Special device "CONSOLE:"

    SEE ALSO
        crypt()

    HISTORY
        A getpass function appeared in Version 7 AT&T UNIX.

    BUGS
        The getpass function leaves its result in an internal static object
        and returns a pointer to that object.  Subsequent calls to getpass
        will modify the same object.

        The calling program should zero the password as soon as possible to
        avoid leaving the cleartext password visible in the memory.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR conout, conin = Open("CONSOLE:", MODE_OLDFILE);
    static char ch, *p, buf[_PASSWORD_LEN];
    int useconsole = conin != NULL;

    D(bug("[UserGroup] %s()\n", __func__));

    /*
     * read and write to CONSOLE: if possible; else read from
     * stdin and write to stderr.
     */
    if (useconsole) {
        conout = conin;
    } else {
        conin = Input();
        conout = Output();
    }

    FPuts(conout, (UBYTE *)prompt);
    Flush(conout);

    SetMode(conin, 1);                                                                  /* Raw */
    for (p = buf; (ch = FGetC(conin)) != EOF && ch != '\n' && ch != '\r';)
        if (p < buf + _PASSWORD_LEN)
            *p++ = ch;

    *p = '\0';
    Write(conout, "\n", 1);

    SetMode(conin, 0);                                                                  /* Cooked */
    if (useconsole)
        Close(conin);

    return buf;

    AROS_LIBFUNC_EXIT
}
