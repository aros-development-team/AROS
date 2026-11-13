/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc:
*/
#include <stdio.h>
#include <string.h>
#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

        AROS_LH3(BOOL, FillRexxMsg,

/*  SYNOPSIS */
        AROS_LHA(struct RexxMsg *, msgptr, A0),
        AROS_LHA(ULONG           , count , D0),
        AROS_LHA(ULONG           , mask  , D1),

/*  LOCATION */
        struct RxsLib *, RexxSysBase, 27, RexxSys)

/*  FUNCTION
        Converts the first count entries of msgptr->rm_Args into allocated
        Rexx argument strings. For entry i, bit i of mask selects the
        input type: a set bit treats the field as a signed long integer;
        a clear bit treats it as a pointer to a NUL-terminated C string.

    INPUTS
        msgptr - RexxMsg whose rm_Args entries contain input values.
        count  - Number of entries to convert, from 0 through MAXRMARG.
        mask   - Type-selection bits for the first count entries. Bit i
                 selects integer conversion when set and string copying
                 when clear.

    RESULT
        TRUE if every selected entry is converted. FALSE if memory
        allocation fails; newly allocated strings are then released and
        the original rm_Args entries remain unchanged.

    NOTES
        On success, the converted entries contain pointers to newly
        allocated Rexx argument strings. A NULL string input remains
        NULL. Original C strings are copied, not freed. Use
        ClearRexxMsg() to release the converted argument strings.
        Do not call this function again on already converted entries:
        replacing their pointers does not release the previous strings.
        Only the characters preceding the first NUL byte of a source C
        string are copied.

    BUGS
        count is not checked against the rm_Args array or the local
        conversion array. Values beyond their capacity cause
        out-of-bounds accesses.

    SEE ALSO
        ClearRexxMsg(), CreateRexxMsg(), CreateArgstring()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    STRPTR args[16];
    char number[20];
    ULONG i, j;
    
    for (i = 0; i < count; i++)
    {
        /* Is argument i an integer ? */
        if (mask & (1<<i))
        {
            /* Convert int to string */
            sprintf(number, "%ld", (long)msgptr->rm_Args[i]);
            args[i] = (STRPTR)CreateArgstring(number, strlen(number));
            
            /* Clean up if error in CreateArgstring */
            if (args[i] == NULL)
            {
                for (j = 0; j < i; j++)
                    if (args[j] != NULL) DeleteArgstring((UBYTE *)args[j]);
                ReturnBool("FillRexxMsg", FALSE);
            }
        }
        else
        {
            /* CreateArgstring with null terminated string if pointer is not null */
            if (msgptr->rm_Args[i] == 0) args[i] = NULL;
            else
            {
                args[i] = (STRPTR)CreateArgstring(RXARG(msgptr,i), strlen(RXARG(msgptr,i)));
            
                /* Clean up if error in CreateArgstring */
                if (args[i] == NULL)
                {
                    for (j = 0; j < i; j++)
                        if (args[j] != NULL) DeleteArgstring((UBYTE *)args[j]);
                    ReturnBool("FillRexxMsg", FALSE);
                }
            }
        }
    }
    
    CopyMem(args, msgptr->rm_Args, count * sizeof(STRPTR));
    ReturnBool("FillRexxMsg", TRUE);
    AROS_LIBFUNC_EXIT
} /* FillRexxMsg */
