/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Find a local variable.
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>
#include <dos/var.h>

        AROS_LH2(struct LocalVar *, FindVar,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(ULONG       , type, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 153, Dos)

/*  FUNCTION
        Finds a local variable structure.

    INPUTS
        name    -   the name of the variable you wish to find. Note that
                    variable names follow the same syntax and semantics
                    as filesystem names.
        type    -   The type of variable to be found (see <dos/var.h>).

    RESULT
        A pointer to the LocalVar structure for that variable if it was
        found. If the variable wasn't found, or was of the wrong type
        NULL will be returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        DeleteVar(), GetVar(), SetVar()

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

  /* only the lowest 8 Bits are valid here */
  type &= 0xFF;

  if(name)
  {
    /* We scan through the process->pr_LocalVars list */
    struct Process  *pr;
    struct LocalVar *var;
    
    pr  = (struct Process *)FindTask(NULL);
    var = (struct LocalVar *)pr->pr_LocalVars.mlh_Head;

    while(var != (struct LocalVar *)&(pr->pr_LocalVars.mlh_Tail) )
    { 
      LONG res;
      if(var->lv_Node.ln_Type == type)
      {
        /* The list is alphabetically sorted. */
        res = Stricmp(name, var->lv_Node.ln_Name);
        
        /* Found it */
        if(res == 0)
          return var;

        /* We have gone too far through the sorted list. */
        else if(res < 0)
          return NULL;
      }
      var = (struct LocalVar *)var->lv_Node.ln_Succ;
    }
  }
  return NULL;

  AROS_LIBFUNC_EXIT
} /* FindVar */
