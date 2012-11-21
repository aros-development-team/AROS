/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetVar - Return the value of a local or global variable.
    Lang: English
*/

#include <aros/debug.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include "dos_intern.h"

static LONG getvar_from(const char *name, const char *volume, STRPTR buffer, LONG size, LONG flags, struct DosLibrary *DOSBase);

/*****************************************************************************

    NAME */
#include <dos/var.h>
#include <proto/dos.h>

        AROS_LH4(LONG, GetVar,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name,   D1),
        AROS_LHA(STRPTR,       buffer, D2),
        AROS_LHA(LONG,         size,   D3),
        AROS_LHA(LONG,         flags,  D4),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 151, Dos)

/*  FUNCTION
        This function will return the value of a local or environmental
        variable in the supplied buffer.

        It is advised to only use ASCII characters with a variable, but
        this is not required.

        If GVF_BINARY_VAR is not specified, this function will stop putting
        characters into the destination buffer when a '\n' is hit, or the
        end of the buffer is reached. Otherwise it will complete fill the
        buffer.

    INPUTS
        name    -   the name of the variable you want.
        buffer  -   Space to store the returned variable.
        size    -   Length of the buffer in bytes.
        flags   -   A combination of the type of variable to get (lower
                    8 bits) and flags that control the value of this
                    function. Current flags are:

                    GVF_GLOBAL_ONLY    - only tries to get a global variable.
                    GVF_LOCAL_ONLY     - only tries to get a local variable.
                    GVF_BINARY_VAR     - do not stop at a '\n' character.
                    GVF_DONT_NULL_TERM - no NULL termination. This only
                                         applies to GVF_BINARY_VAR.

    RESULT
        Will return the number of characters put in the buffer, or -1
        if the variable is not defined. The '\n' character if it exists
        will not be placed in the buffer.

        If the value would overflow the user buffer, then the number of
        characters copied into the buffer will be returned and the buffer
        truncated.The buffer will be NULL terminated unless
        GVF_DONT_NULL_TERM is set.

        IoErr() will contain either:
          ERROR_OBJECT_NOT_FOUND
              if the variable is not defined.
          ERROR_BAD_NUMBER
              if the size of the buffer is 0.
          the total length of the variable
              otherwise.

    NOTES

    EXAMPLE

    BUGS
        LV_VAR is the only type that can be global.

    SEE ALSO
        DeleteVar(), FindVar(), SetVar()

    INTERNALS
        Redo the RESULT documentation.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("GetVar: name = \"%s\", buffer = $%lx, size = %ld, flags = $%lx\n",
          name, buffer, size, flags));

    if (0 == size)
    {
        D(bug("GetVar: bad size\n"));

        SetIoErr(ERROR_BAD_NUMBER);

        return 0;
    }

    if (name && buffer)
    {
        /* not global only? */
        if(0 == (flags & GVF_GLOBAL_ONLY))
        {
            /* look for a local variable */
            struct LocalVar *lv;
            
            D(bug("GetVar: Local variable\n"));
            /* look for a variable of the given name */
            lv = FindVar(name, flags);

            if (lv)
            {
                int i;
                /* which size is shorter: the buffer or the size of
                   the value? */
                i = (size < lv->lv_Len) ? size : lv->lv_Len;
                CopyMem(lv->lv_Value, buffer, i);
                
                /* were we supposed to stop after the first "\n"?
                   = No GVF_BINARY_VAR and no GVF_DONT_NULL_TERM
                */
                if (0 == (flags & GVF_BINARY_VAR))
                {
                    int j = 0;

                    while ((buffer[j] != '\n') && (j < i))
                    {
                        j++;
                    }
                    
                    if (j == size)
                    {
                        j = size - 1;
                    }

                    buffer[j]= 0x0; /* mark end of string */
                    size = j;
                }
                else if (0 == (flags & GVF_DONT_NULL_TERM))
                {
                    if (i == size)
                    {
                        i = size - 1;
                    }
                    
                    buffer[i] = 0x0; /* mark end of string */
                    size = i;
                }
                else
                {
                    size = i;
                }
                
                SetIoErr(lv->lv_Len);
                D(bug("GetVar: return %d\n", size));

                return size;
            } /* Got lv */
        } /* !global only */
        
        /****** GLOBAL VARIABLE TREATMENT ******/
        
        if ((flags & 0xff) == LV_VAR && !(flags & GVF_LOCAL_ONLY))
        {
            LONG ret;

            /* as standard: look for the file in ENV: if no path is
             * given in the variable
             */
            ret = getvar_from(name, "ENV:", buffer, size, flags, DOSBase);

            if (ret >= 0)
                return ret;

            /* If not found in ENV:, look in ENVARC: */
            ret = getvar_from(name, "ENVARC:", buffer, size, flags, DOSBase);

            if (ret >= 0)
                return ret;

        } /* ! local file only */
    } /* name and buffer */
    
    D(bug("GetVar: not found\n"));

    SetIoErr(ERROR_OBJECT_NOT_FOUND);

    return -1;

    AROS_LIBFUNC_EXIT
} /* GetVar */


static LONG getvar_from(const char *name, const char *volume, STRPTR buffer, LONG size, LONG flags, struct DosLibrary *DOSBase)
{
    BPTR file;
    LONG i;
    
    UBYTE filebuf[256];

    strncpy(filebuf, volume, sizeof(filebuf));
    /* Just being paranoid here */
    filebuf[sizeof(filebuf)-1]=0;
    
    AddPart(filebuf, name, 256);
    D(bug("GetVar: Global variable: %s\n", filebuf));
    file = Open(filebuf, MODE_OLDFILE);

    if (file) /* file could be opened */
    {
        ULONG fSize;
        struct FileInfoBlock fib;

        if (ExamineFH(file, &fib))
        {
            /* fSize now contains the size of variable. */
            fSize = fib.fib_Size;
        }
        else
        {
            D(bug("GetVar: can't find size\n"));

            return -1;
        }

        /* We return the number of bytes actually read. */
        i = Read(file, buffer, size);
        Close(file);

        /* were we supposed to stop after the first "\n"?
           = No GVF_BINARY_VAR and no GVF_DONT_NULL_TERM */
        if (0 == (flags & GVF_BINARY_VAR))
        {
            int j = 0;
            /* lets search for the first '\n' (if any) in the
             * string and replace it by '\0'. */
            while ((buffer[j] != '\n') && (j < i))
            {
                j++;
            }
    
            if (j == size)
            {
                j = size - 1;
            }

            buffer[j]= '\0'; /* mark end of string */
            size = j;
        }
        else if (0 == (flags & GVF_DONT_NULL_TERM))
        {
            if (i == size)
            {
                i = size - 1;
            }

            buffer[i] = 0x0; /* mark end of string */
            size = i;
        }
        else
        {
            size = i;
        }

        SetIoErr(fSize);
        D(bug("GetVar: return %d\n", size));

        return size;
    } /* open(file) */

    return -1;
}
