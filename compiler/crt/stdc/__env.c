/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Internal functions for environment variables handling.
*/

#include "__stdcio_intbase.h"

#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/var.h>

#include "__env.h"

#if AROS_AMIGAOS_COMPLIANCE && (AROS_AMIGAOS_COMPLIANCE >= 30)
/* ScanVars is a Morphos addition in dos.library v50, so we
 * have this stub here for posixc.library compatability with
 * dos.library v30..v39.
 */
/*****i**********************************************************************
    NAME
        LONG v30_ScanVars(

    SYNOPSIS
        struct Hook *hook, ULONG flags, APTR userdata)

    FUNCTION
        Invoke hook for each LV_VAR in the current process local
        variable list, stopping at its first nonzero result.

    INPUTS
        hook - Valid synchronous callback hook.
        flags - Copied into each message; does not filter the list.
        userdata - Opaque callback argument.
        Requires process context and stable local variables during
        the scan. Intended for GVF_LOCAL_ONLY callers.

    RESULT
        The first nonzero hook result, or zero after a complete
        scan. Failure to open utility.library also returns zero.

    NOTES
        Compatibility implementation selected when
        AROS_AMIGAOS_COMPLIANCE is at least 30. Opens and closes
        utility.library around the scan. Message storage is valid
        only during the call; names and values are borrowed from
        the process list. sv_GDir is an empty string.

*****************************************************************************/
static LONG v30_ScanVars(struct Hook *hook, ULONG flags, APTR userdata)
{
    /* We scan through the process->pr_LocalVars list */
    struct Process  *pr;
    struct LocalVar *var;
    struct ScanVarsMsg msg;
    LONG res = 0;
    APTR UtilityBase;
    
    UtilityBase = OpenLibrary("utility.library", 0);
    if (UtilityBase == NULL)
        return 0;

    msg.sv_SVMSize = sizeof(struct ScanVarsMsg);
    msg.sv_Flags = flags;
    pr  = (struct Process *)FindTask(NULL);

    /* We know that the callers of this routine
     * will be asking for GVF_LOCAL_ONLY
     */
    var = (struct LocalVar *)pr->pr_LocalVars.mlh_Head;
    
    ForeachNode(&pr->pr_LocalVars, var)
    {
        if (var->lv_Node.ln_Type == LV_VAR)
        {
            msg.sv_Name = var->lv_Node.ln_Name;
            msg.sv_Var = var->lv_Value;
            msg.sv_VarLen = var->lv_Len;
            msg.sv_GDir = "";
            res = CallHookPkt(hook, userdata, &msg);
            if(res != 0)
                break;
        }
    }

    CloseLibrary(UtilityBase);
    return res;
}
#undef ScanVars
#define ScanVars(a,b,c) v30_ScanVars(a,b,c)
#endif

/*****i**********************************************************************
    NAME
        __env_item * __env_newvar(

    SYNOPSIS
        const char *name, int valuesize)

    FUNCTION
        Allocate a detached environment cache entry with a copied
        name, uninitialized value storage and a NULL next link.

    INPUTS
        name - Readable, null-terminated name.
        valuesize - Positive allocation size in bytes; not checked.

    RESULT
        The new entry, owned by the caller, or NULL on allocation
        failure. Partial allocations are freed on failure.

    NOTES
        Initialize and terminate the value before exposing the
        entry to cache lookup. Release name, value and entry with
        free() when ownership is not transferred to the cache.

*****************************************************************************/
static __env_item *__env_newvar(const char *name, int valuesize)
{
    __env_item *item;

    item = malloc(sizeof(__env_item));
    if (!item) goto err1;

    item->name = strdup(name);
    if (!item->name) goto err2;

    item->value = malloc(valuesize);
    if (!item->value) goto err3;

    item->next = NULL;

    return item;

err3:
    free(item->name);
err2:
    free(item);
err1:
    return NULL;
}

/*****i**********************************************************************
    NAME
        __env_item ** internal_findvar(

    SYNOPSIS
        register const char *name)

    FUNCTION
        Locate the link holding a named entry in the current
        stdcio cache, or the NULL link where it can be appended.

    INPUTS
        name - Readable, null-terminated key. Requires initialized
               stdcio base state and a well-formed cache list.

    RESULT
        A borrowed pointer to the matching entry link or terminal
        NULL link. Comparison is case-sensitive. No allocation
        or list modification occurs.

    NOTES
        Use the link only while the cache remains unchanged.
        The function does not synchronize access.

*****************************************************************************/
static __env_item **internal_findvar(register const char *name)
{
    struct StdCIOIntBase *StdCIOBase =
        (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();

   __env_item **curr;

   for (
       curr = &StdCIOBase->env_list;
       *curr && strcmp((*curr)->name, name);
       curr = &((*curr)->next)
   );

  return curr;
}

/*
  Allocates space for a variable with name 'name' returning a pointer to it.
  If a variable with this name already exists then returns a pointer to that
  variable.

  Returns NULL on error.
*/
/*****i**********************************************************************
    NAME
        __env_item * __stdcio_env_getvar(

    SYNOPSIS
        const char *name, int valuesize)

    FUNCTION
        Find or create an entry in the current stdcio environment
        cache. This function does not read a DOS variable.

    INPUTS
        name - Readable, null-terminated, case-sensitive cache key.
        valuesize - Positive number of bytes required for the value,
                    including space for its terminator.
        Existing cached values must be valid null-terminated strings.
        Serialize access to a shared cache.

    RESULT
        A borrowed cache entry, or NULL when creation fails or a
        failed replacement removes the last entry. Newly allocated
        value storage is uninitialized; fill and terminate it before
        another lookup. Do not free the returned entry directly.

    NOTES
        Existing storage is replaced when strlen(value) is less
        than valuesize. Its old contents and address are then lost.
        The comparison uses string length, not allocated capacity.
        No arguments or size arithmetic are validated.

    BUGS
        If replacement allocation fails, the old value and entry
        are freed, the name leaks, and the entry is unlinked. The
        return value is then the next entry, if any, which can belong
        to a different name. A non-NULL result does not reliably
        identify the requested variable after allocation failure.

*****************************************************************************/
__env_item *__stdcio_env_getvar(const char *name, int valuesize)
{
    register __env_item **curr;

    curr = internal_findvar(name);

    if (*curr)
    {
        if (strlen((*curr)->value) < valuesize)
        {
            free((*curr)->value);
            (*curr)->value = malloc(valuesize);

            if (!(*curr)->value)
            {
                __env_item *tmp = (*curr)->next;

                free(*curr);
                *curr = tmp;
            }
        }
    }
    else
    {
        *curr = __env_newvar(name, valuesize);
    }

    return *curr;
}

/*****i**********************************************************************
    NAME
        void __env_delvar(

    SYNOPSIS
        const char *name)

    FUNCTION
        Remove the first matching entry from the current stdcio
        environment cache and free its name, value and node.

    INPUTS
        name - Readable, null-terminated, case-sensitive key.
               Serialize access to a shared cache.

    RESULT
        No return value. A missing key leaves the list unchanged.

    NOTES
        Invalidates borrowed pointers into the removed entry.
        Does not delete or change the corresponding DOS variable.

*****************************************************************************/
void __env_delvar(const char *name)
{
    register __env_item **curr;

    curr = internal_findvar(name);

    if (*curr)
    {
        register __env_item *tmp = *curr;

        *curr = (*curr)->next;

        free(tmp->name);
        free(tmp->value);
        free(tmp);
    }
}

struct EnvData
{
   int envcount;
   int varbufsize;
   char * varbufptr;
};

/* Hook function counting the number of variables and computing the buffer size
   needed for name=value character buffer. */
/*****i**********************************************************************
    NAME
        LONG get_var_len(

    SYNOPSIS
        struct Hook *hook, APTR userdata, struct ScanVarsMsg *message)

    FUNCTION
        Count one variable and add its name=value string storage
        requirement to an EnvData accumulator.

    INPUTS
        hook - Unused.
        userdata - Writable, initialized EnvData accumulator.
        message - Valid scan message with a terminated name and
                  a nonnegative value length.

    RESULT
        Always zero, allowing the scan to continue.

    NOTES
        Adds one to envcount and name length plus sv_VarLen plus
        two to varbufsize. Counts use int without overflow checks.
        No value bytes are read or retained.

*****************************************************************************/
LONG get_var_len(struct Hook *hook, APTR userdata, struct ScanVarsMsg *message)
{
    struct EnvData *data = (struct EnvData *) userdata;
    data->envcount++;
    data->varbufsize +=
        (ptrdiff_t) strlen((char*)message->sv_Name) +
        message->sv_VarLen + 2;
    return 0;
}

/* Hook function storing name=value strings in buffer */
/*****i**********************************************************************
    NAME
        LONG get_var(

    SYNOPSIS
        struct Hook *hook, APTR userdata, struct ScanVarsMsg *message)

    FUNCTION
        Append one null-terminated name=value string at the
        current EnvData buffer cursor and advance the cursor.

    INPUTS
        hook - Unused.
        userdata - EnvData with a writable varbufptr and enough
                   remaining storage for the complete string.
        message - Valid name and value data from the scan.

    RESULT
        Always zero. The cursor points just past the stored NUL.

    NOTES
        Copies at most sv_VarLen value bytes, stopping at an
        embedded NUL. Binary values are therefore truncated.
        No destination bound is checked; message storage is not
        retained.

*****************************************************************************/
LONG get_var(struct Hook *hook, APTR userdata, struct ScanVarsMsg *message)
{
    struct EnvData *data = userdata;
    data->varbufptr[0] = '\0';
    strcat(data->varbufptr, (char*) message->sv_Name);
    strcat(data->varbufptr, "=");
    strncat(data->varbufptr, (char*) message->sv_Var, message->sv_VarLen);
    data->varbufptr = data->varbufptr + strlen(data->varbufptr) + 1;
    return 0;
}

/* Function storing name=value strings in given environ buffer. When buffer is
   null, it returns minimal necessary size of the buffer needed to store all
   name=value strings. */
/*****i**********************************************************************
    NAME
        int __env_get_environ(

    SYNOPSIS
        char **environ, int size)

    FUNCTION
        Measure or write a snapshot of DOS local variables as a
        NULL-terminated pointer array followed by name=value
        strings in the same allocation.

    INPUTS
        environ - NULL to measure, or suitably aligned writable
                  storage for the pointer array and string data.
        size - Available bytes when environ is non-NULL.
        Requires process context and an unchanged local-variable
        list throughout both scans.

    RESULT
        With NULL environ, the calculated size in bytes. Otherwise
        returns -1 before writing when size is too small, or zero
        after constructing the snapshot. No storage is allocated.

    NOTES
        Array entries point inside the supplied block. The final
        entry is NULL. The caller owns the block and must keep it
        alive while using those pointers. Scan results and integer
        size overflow are not checked.

    BUGS
        For an empty environment, the calculated size covers only
        one pointer, but the write path also writes a NUL byte
        immediately after it. The advertised minimum is too small.
        Changes between scans can invalidate both capacity and
        entry counts. Embedded NULs truncate variable values.

*****************************************************************************/
int __env_get_environ(char **environ, int size)
{
    int i;
    struct Hook hook;
    struct EnvData u;
    char *varbuf;

    u.envcount = 0;
    u.varbufsize = 0;

    memset(&hook, 0, sizeof(struct Hook));
    hook.h_Entry = (HOOKFUNC) get_var_len;
    ScanVars(&hook, GVF_LOCAL_ONLY, &u);

    if(environ == NULL)
        return sizeof(char*) * (u.envcount + 1) + u.varbufsize;
    else if(size < sizeof(char*) *(u.envcount + 1) + u.varbufsize)
        return -1;

    /* store name=value strings after table of pointers */
    varbuf = (char*) (environ + (u.envcount + 1));
            
    /* time to fill in the buffers */
    u.varbufptr = varbuf;
    u.varbufptr[0] = '\0';
    hook.h_Entry = (HOOKFUNC) get_var;
    ScanVars(&hook, GVF_LOCAL_ONLY, &u);

    for(i = 0; i < u.envcount; i++)
    {
        environ[i] = varbuf;
        varbuf = strchr(varbuf, '\0') + 1;
    }
    environ[u.envcount] = NULL;

    return 0;
}
