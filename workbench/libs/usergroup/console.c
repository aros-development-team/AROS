/*
 * Console handling
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
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <sys/time.h>

#include "base.h"

#include <proto/usergroup.h>

#include <string.h>

/*****************************************************************************

    NAME */

        AROS_LH0I(int, ug_OnConsole,

/*  SYNOPSIS */

/* LOCATION */
                  struct Library *, UserGroupBase, 22, Usergroup)

/*  FUNCTION
        Check if the user is logged on local console.

    RESULT
        result - 1 if the user is on console,
                 0 otherwise.

    BUGS

        Currently checking is done depending on the process window pointer.

    SEE ALSO

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);

    return
        me->pr_Task.tc_Node.ln_Type == NT_PROCESS &&
        me->pr_WindowPtr != (APTR)-1;

    AROS_LIBFUNC_EXIT
}

/******************************************************************************/

char *i_GetConsoleName(struct MsgPort *consoletask, char *buffer, ULONG size)
{
    struct Node *portowner = consoletask->mp_SigTask;
    static char hex_chars[] = "0123456789abcdef";
    IPTR ctx = (IPTR) consoletask;
    STRPTR poname;
    ULONG i;

    D(bug("[UserGroup] %s()\n", __func__));

    /* Fail if the there is no task for this port */
    if (portowner == NULL ||
            (consoletask->mp_Node.ln_Type != PA_SIGNAL &&
             consoletask->mp_Node.ln_Type != PA_SOFTINT) ||
            portowner->ln_Name == NULL) {
        poname = "XXX";
    } else {
        /* OK, port owner has got a name, use it */
        poname = portowner->ln_Name;
    }

    i = strlen(poname);
    if (i > 6)
        i = 6;
    if (i > size)
        i = size;
    memcpy(buffer, poname, size);

    /* Append with ":%06x" consoletask */
    if (i < size) {
        short n;

        for (n = 8, buffer[i++] = ':'; n > 0 && i < size; n--) {
            int hex_digit = ctx >> 28 & 0xf;
            if (n < 7 || hex_digit > 0)
                buffer[i++] = hex_chars[hex_digit];
            ctx <<= 4;
        }
    }

    /* NUL terminate */
    if (i < size) {
        buffer[i] = '\0';
        return buffer;
    } else {
        return NULL;
    }
}


/*****************************************************************************

    NAME */

        AROS_LH3I(char *, ug_ConsoleName,

/*  SYNOPSIS */
        AROS_LHA(BPTR, con, D0),
        AROS_LHA(char *, buffer, A1),
        AROS_LHA(ULONG, size, D1),

/* LOCATION */
        struct Library *, UserGroupBase, 23, Usergroup)

/*  FUNCTION
        Get a unique printable identifier for the interactive filehandle.
        This identifier is usually the task name of handler concatenated
        with message port address.

    INPUTS
        fh     - An interactive filehandle
        buffer - Buffer to hold console identifier
        size   - Number of bytes in buffer.

    RESULT
        name - If call is successful, pointer to buffer. NULL if
               error.

    BUGS
        May not get the proprer console name for all different console
        handlers.

    SEE ALSO
        dos.library/GetConsoleTask()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *confh = BADDR(con);

    D(bug("[UserGroup] %s()\n", __func__));

    if (con == 0)
        return NULL;

    if (!IsInteractive(con))
        return NULL;

    if (buffer == NULL || size < 1)
        return NULL;

    return i_GetConsoleName(confh->fh_Type, buffer, size);

    AROS_LIBFUNC_EXIT
}


