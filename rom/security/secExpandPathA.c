/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"

/* Append s to buffer; FALSE if it does not fit */
static BOOL Put(STRPTR buffer, ULONG size, ULONG *len, CONST_STRPTR s, ULONG n)
{
    if (*len + n >= size)
        return FALSE;
    CopyMem((APTR)s, buffer + *len, n);
    *len += n;
    buffer[*len] = '\0';
    return TRUE;
}

/*****************************************************************************

    NAME */
        AROS_LH3(BOOL, secExpandPathA,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, template, A0),
        AROS_LHA(STRPTR, buffer, A1),
        AROS_LHA(struct TagItem *, taglist, A2),

/*  LOCATION */
        struct SecurityBase *, secBase, 65, Security)

/*  FUNCTION
        Expand the user tokens of a path for the owner of a task: the
        per-user assigns of dos.library ("Assign T: RAM:T/%u USER") are
        resolved with it, each time and in the task that uses them.

          %u  the user ID ("root" for the super user, "nobody" for a task
              that no user owns)
          %h  the user's home directory
          %g  the ID of the user's primary group
          %%  a '%'

    INPUTS
        template - the path with tokens
        buffer   - receives the expanded path
        taglist  - secT_Task (struct Task *): the task whose owner counts
                   (default: the caller);
                   secT_BufferSize (ULONG): the size of buffer (required)

    RESULT
        TRUE if the path was expanded. FALSE with IoErr() set otherwise:
        ERROR_NOT_IMPLEMENTED on a system that is not multi-user (the
        template is not expanded there), ERROR_BAD_TEMPLATE for an unknown
        token, ERROR_OBJECT_NOT_FOUND when the user has no home directory
        or group, ERROR_LINE_TOO_LONG.

    SEE ALSO
        dos.library/GetDeviceProc(), C:Assign

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *task = (struct Task *)GetTagData(secT_Task, 0, taglist);
    ULONG size = GetTagData(secT_BufferSize, 0, taglist);
    struct secUserInfo *ui = NULL;
    struct secGroupInfo *gi = NULL;
    ULONG owner, len = 0, i;
    LONG err = ERROR_LINE_TOO_LONG;   /* what a failed Put() means */
    BOOL ok = TRUE, user = FALSE;

    if (!template || !buffer || size == 0)
    {
        SetIoErr(ERROR_REQUIRED_ARG_MISSING);
        return FALSE;
    }
    buffer[0] = '\0';
    if (!secBase->sec_AfterDOSDone || !secBase->Configured)
    {
        SetIoErr(ERROR_NOT_IMPLEMENTED);
        return FALSE;
    }
    owner = GetTaskOwner(secBase, task ? task : FindTask(NULL));

    for (i = 0; ok && template[i]; i++)
    {
        if (template[i] != '%')
        {
            ok = Put(buffer, size, &len, &template[i], 1);
            continue;
        }
        switch (template[++i])
        {
        case '%':
            ok = Put(buffer, size, &len, "%", 1);
            break;

        case 'u':
        case 'h':
        case 'g':
            if (!user)
            {
                /* the owner's record, once */
                if ((ui = secAllocUserInfo()))
                {
                    ui->uid = owner >> 16;
                    if (!secGetUserInfo(ui, secKeyType_uid))
                    {
                        secFreeUserInfo(ui);
                        ui = NULL;
                    }
                }
                user = TRUE;
            }
            if (template[i] == 'u')
            {
                if (ui)
                    ok = Put(buffer, size, &len, ui->UserID, strlen(ui->UserID));
                else if ((owner >> 16) == secNOBODY_UID)
                    ok = Put(buffer, size, &len, "nobody", 6);
                else
                {
                    err = ERROR_OBJECT_NOT_FOUND;
                    ok = FALSE;
                }
            }
            else if (template[i] == 'h')
            {
                if (ui && ui->HomeDir[0])
                    ok = Put(buffer, size, &len, ui->HomeDir, strlen(ui->HomeDir));
                else
                {
                    err = ERROR_OBJECT_NOT_FOUND;
                    ok = FALSE;
                }
            }
            else
            {
                if (!gi && (gi = secAllocGroupInfo()))
                {
                    gi->gid = owner & secMASK_GID;
                    if (!secGetGroupInfo(gi, secKeyType_gid))
                    {
                        secFreeGroupInfo(gi);
                        gi = NULL;
                    }
                }
                if (gi)
                    ok = Put(buffer, size, &len, gi->GroupID, strlen(gi->GroupID));
                else
                {
                    err = ERROR_OBJECT_NOT_FOUND;
                    ok = FALSE;
                }
            }
            break;

        default:
            err = ERROR_BAD_TEMPLATE;
            ok = FALSE;
            break;
        }
    }

    if (ui)
        secFreeUserInfo(ui);
    if (gi)
        secFreeGroupInfo(gi);
    if (!ok)
    {
        buffer[0] = '\0';
        SetIoErr(err);
    }
    return ok;

    AROS_LIBFUNC_EXIT
} /* secExpandPathA */
