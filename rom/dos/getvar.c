/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: GetVar - Return the value of a local or global variable.
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <dos/dos.h>
/*****************************************************************************

    NAME */
#include <dos/var.h>
#include <proto/dos.h>

        AROS_LH4(LONG, GetVar,

/*  SYNOPSIS */
        AROS_LHA(STRPTR, name, D1),
        AROS_LHA(STRPTR, buffer, D2),
        AROS_LHA(LONG  , size, D3),
        AROS_LHA(LONG  , flags, D4),

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

    HISTORY
        27-11-96    digulla automatically created from
                            dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

  if(name && buffer)
  {
    /* not global only? */
    if( 0 == (flags & GVF_GLOBAL_ONLY))
    {
      /* look for a local variable */
      struct LocalVar *lv;

      /* look for a variable of the given name */
      lv = FindVar(name, flags);
      if(lv)
      {
        int i;
        if(0 == size)
        {
          SetIoErr(ERROR_BAD_NUMBER);
          return 0;
        }

        /* which size is shorter: the buffer or the size of the value? */
        i = (size < lv->lv_Len) ? size : lv->lv_Len;
        CopyMem(lv->lv_Value, buffer, i);

        /* were we supposed to stop after the first "\n"?
           = No GVF_BINARY_VAR and no GVF_DONT_NULL_TERM
        */
        if (0 == (flags & (GVF_BINARY_VAR|GVF_DONT_NULL_TERM)))
        {
          int j = 0;
          while ( (buffer[j] != '\n') && (j < i) )
            j++;
            buffer[j]= 0x0; /* mark end of string */
            size = j;
        }
        else
        {
          size = lv->lv_Len;
        }

        SetIoErr(lv->lv_Len);
          return size;

      } /* Got lv */
    } /* !global only */

    /****** GLOBAL VARIABLE TREATMENT ******/

    /* global variable: GVF_GLOBAL_ONLY is set *OR*
                        GVF_GLOBAL_ONLY AND GVF_LOCAL_ONLY are NOT set
     */
    if (GVF_GLOBAL_ONLY == (flags & GVF_GLOBAL_ONLY) ||
        0               == (flags & (GVF_GLOBAL_ONLY|GVF_LOCAL_ONLY)) )
    {
      BPTR file;
      /* as standard: look for the file in ENV: if no path is
         given in the variable
      */
      UBYTE filebuf[256] = "ENV:";
      AddPart(filebuf, name, 256);

      file = Open(filebuf, MODE_OLDFILE);
      if(file) /* file could be opened */
      {
        ULONG fSize;
        struct FileInfoBlock fib;

        if(0 == size)
        {
          SetIoErr(ERROR_BAD_NUMBER);
          return -1;
        }

        if(ExamineFH(file, &fib))
        {
          /* fSize now contains the size of variable. */
          fSize = fib.fib_Size;
        }
        else
          return -1;

        /* We return the number of bytes actually read. */
        size = Read(file, buffer, size);
        Close(file);

        /* were we supposed to stop after the first "\n"?
           = No GVF_BINARY_VAR and no GVF_DONT_NULL_TERM
        */
        if (0 == (flags & (GVF_BINARY_VAR|GVF_DONT_NULL_TERM)))
        {
          int j = 0;
          while ( (buffer[j] != '\n') && (j < size) )
            j++;
          buffer[j]= 0x0; /* mark end of string */
          size = j;
        }
        SetIoErr(fSize);
        return size;
      } /* open(file) */
    } /* ! local file only */
  } /* name and buffer */

  SetIoErr(ERROR_OBJECT_NOT_FOUND);
  return -1;

  AROS_LIBFUNC_EXIT
} /* GetVar */
