/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DeleteVar() - Deletes a local or environmental variable.
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <dos/var.h>
#include <proto/dos.h>

        AROS_LH2(LONG, DeleteVar,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(ULONG , flags, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 152, Dos)

/*  FUNCTION
        Deletes a local or environment variable.

        The default is to delete a local variable if one was found,
        or to delete a global environment variable otherwise.

        A global environment variable will only be deleted for the
        type LV_VAR.

    INPUTS
        name  - the name of the variable to delete. Note that variable
                names follow the same syntax and semantics as filesystem
                names.

        flags - A combination of the type of variable (low 8 bits), and
                flags to control the behaviour of this routine.
                Currently defined flags:

                GVF_LOCAL_ONLY  - delete a local variable.
                GVF_GLOBAL_ONLY - delete a global environment variable.

    RESULT
        If non-zero, the variable was deleted successfully,
        DOSFALSE otherwise.

    NOTES
        When the GVF_SAVE_VAR flag is set, and only one of the global
        variable pair could be deleted (either the in memory or on disk
        variable), DOSFALSE will be returned.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        XXX: Find out whether GVF_SAVE_VAR does actually affect this function.

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  if(name)
  {  
    if((flags & GVF_GLOBAL_ONLY) == 0)
    {
      struct LocalVar *lv = NULL;
      lv = FindVar(name, flags & 0xFF);
      if(lv)
      {
        /* free allocated memory for value of variable */
        FreeMem(lv -> lv_Value, lv -> lv_Len);

        Remove((struct Node *)lv);
        FreeVec(lv);
        return DOSTRUE;
      }
    } /* !global only => local variable */

    /* If we are allowed to delete globals, and not deleting an alias */
    if( ((flags & GVF_LOCAL_ONLY) == 0) && ((flags & 0x7F) == 0) )
    {
      /* Variable names should be less than 256 characters. */
      /* as a standard: look for the file in ENV: if no path is
         given in the variable
      */
      UBYTE filebuffer[256] = "ENV:";
      BPTR filelock;
      BOOL delMemory = FALSE, delDisk = FALSE;

      AddPart(filebuffer, name, 256);

      if((filelock = Lock(filebuffer, EXCLUSIVE_LOCK)))
      {
        UnLock(filelock);
        delMemory = DeleteFile(filebuffer);
      }

      if(flags & GVF_SAVE_VAR)
      {
        filebuffer[0] = 0;
        AddPart(filebuffer, "ENVARC:", 256);
        AddPart(filebuffer, name, 256);

        if((filelock = Lock(filebuffer, EXCLUSIVE_LOCK)))
        {
          UnLock(filelock);
          delDisk = DeleteFile(filebuffer);
        }
      }
      else delDisk = TRUE;
      
      if( (delDisk != FALSE) && (delMemory != FALSE))
        return TRUE;
    } /* !local only => Global variable */
  } /* if(name) */

  return DOSFALSE;

  AROS_LIBFUNC_EXIT
} /* DeleteVar */
