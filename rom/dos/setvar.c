/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/lists.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <dos/var.h>
#include <proto/dos.h>

        AROS_LH4(BOOL, SetVar,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(CONST_STRPTR, buffer, D2),
        AROS_LHA(LONG        , size, D3),
        AROS_LHA(LONG        , flags, D4),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 150, Dos)

/*  FUNCTION
        This function will set a local of environmental variable. Although
        it is recommended that you only use ASCII strings within variables,
        this is not actually required.

	Variable names are not case sensitive.

	SetVar() for an already existing variable changes the variable's
	value to "buffer".

    INPUTS
        name        -   The name of the variable to set.
        buffer      -   The actual data of the variable.
        size        -   The size of the data in the buffer.
        flags       -   Combination of the type of variable to set (lower
                        8 bits of the value), and various flags which control
                        this function. Flags defined are:

                        GVF_LOCAL_ONLY  -   set a local variable only,
                        GVF_GLOBAL_ONLY -   set a global environmental
                                            variable only.
                        GVF_SAVE_VAR    -   If GVF_GLOBAL_ONLY is set, then
                                            this flag will cause SetVar() to
                                            save the variable to ENVARC: as well
                                            as to ENV:.
			GVF_BINARY_VAR and GVF_DONT_NULL_TERM are stored in
			the lv_Flags field for local variables, but not
			used otherwise by SetVar().

                        Note the default is to set a local environmental
                        variable.

			The following variable types are defined:
			LV_VAR		- local environment variable
			LV_ALIAS	- shell alias
			LVF_IGNORE	- internal shell use

			LV_VAR and LV_ALIAS should be treated as
			"exclusive or".


    RESULT
        Zero if this function failed, non-zero otherwise.

    NOTES
    	It is possible to have two variables with the same name as
	long as they have different types.

    EXAMPLE

    BUGS
        Only type LV_VAR can be made global.

        If you set GVF_SAVE_VAR, and this function returns failure, the
        variable may have still been set in ENV:.

    SEE ALSO
        DeleteVar(), FindVar(), GetVar(),

    INTERNALS
    	See FindVar() for a description of the inner workings of local
	variables.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* valid input? */
    if(name && buffer && size)
    {
	/* Local variable is default. */
	if((flags & GVF_GLOBAL_ONLY) == 0)
	{
	    ULONG nameLen = strlen(name);
	    struct LocalVar *lv;
      
	    /* does a Variable with that name already exist? */
	    if (NULL != (lv = FindVar(name, flags)))
	    {
		/* delete old value of that existing variable */
		FreeMem(lv->lv_Value,lv->lv_Len);
	    }
	    else 
	    {
		/* 
		** create a LocalVar-structure and insert it into the list
		*/
		if (NULL != (lv = AllocVec(sizeof(struct LocalVar) + nameLen + 1,
					   MEMF_CLEAR|MEMF_PUBLIC) ) )
		{
		    struct Process  *pr = (struct Process *)FindTask(NULL);
		    struct LocalVar *n  = (struct LocalVar *)pr->pr_LocalVars.mlh_Head;

		    /* init the newly created structure */

		    lv->lv_Node.ln_Type = flags;  /* ln_Type is UBYTE! */
		    lv->lv_Node.ln_Name = (UBYTE *)lv + sizeof(struct LocalVar);
		    CopyMem(name, lv->lv_Node.ln_Name, nameLen);
		    lv->lv_Flags = flags & (GVF_BINARY_VAR|GVF_DONT_NULL_TERM);
          
		    /* 
		    ** First let's see whether we have to add it at the head
		    ** of the list as the list is still empty   OR
		    ** the very first element is already greater than the one
		    ** we want to insert 
		    */
          
		    if (n == (struct LocalVar *)&(pr->pr_LocalVars.mlh_Tail) ||
			Stricmp(name, n->lv_Node.ln_Name) < 0)
		    {
			AddHead((struct List *)&pr->pr_LocalVars,
				(struct Node *)lv);
		    }
		    else
		    {
			/*
			** Now we can be sure that we will have to insert
			** somewhere behind the first element in the list 
			*/
			ForeachNode(&pr->pr_LocalVars, n)
			{
			    if (Stricmp(name, n->lv_Node.ln_Name) < 0)
			    {
				break;
			    }
			}
            
			if (NULL != n->lv_Node.ln_Succ)
			{
			    Insert((struct List *)&pr->pr_LocalVars ,
				   (struct Node *) lv ,
				   (struct Node *) n->lv_Node.ln_Pred);
			}
			else
			{
			    AddTail((struct List *)&pr->pr_LocalVars,
				    (struct Node *) lv);
			}
		    }
		}
	    }

	    /* -1 as size means: buffer contains a null-terminated string*/
	    if (-1 == size)
	    {
	        /* Do NOT add 1 byte to account for the NUL char, AmigaOS(R) doesn't
		   do it that way.  */
		lv->lv_Len = strlen(buffer);
	    }
	    else
	    {
		lv->lv_Len = size;
	    }
	    
	    /* now get some memory for the value*/
	    lv->lv_Value = AllocMem(lv->lv_Len, MEMF_PUBLIC);
	    
	    if (lv->lv_Value)
	    {
		CopyMem(buffer, lv->lv_Value, lv->lv_Len);
		
		return DOSTRUE; 
	    } /* memory for actual value */
	} /* set a local variable */
	
	/* Ok, try and set a global variable. */
	if ((flags & GVF_LOCAL_ONLY) == 0) 
	{
	    BPTR file;
	    /* as a standard: look for the file in ENV: if no path is
	       given in the variable */
	    UBYTE nameBuffer[384]= "ENV:";

	    AddPart(nameBuffer, name, 384);
	    
	    /* Just try and open the file */
	    file = Open(nameBuffer, MODE_NEWFILE);

	    if (file != NULL)
	    {
		/* Write the data to the file */
		/* size = -1 means that the value is a null-terminated
		   string */
		if (-1 == size)
		{
		    Write(file, buffer, strlen(buffer));
		}
		else
		{
		    Write(file, buffer, size);
		}
		
		Close(file);
	    }
	    else
	    {
		return DOSFALSE;
	    }

	    /* Let's see whether we're supposed to make a copy of this to
	     * envarc also... 
	     */
	    if (0 != (flags & GVF_SAVE_VAR))
	    {
		CopyMem("ENVARC:", nameBuffer, 8);
		AddPart(nameBuffer, name, 384);

		file = Open(nameBuffer, MODE_NEWFILE);

		if (file != NULL)
		{
		    /* Write the data to the file */
		    /* size = -1 means that the value is a null-terminated
		       string */
		    if (-1 == size)
		    {
			Write(file, buffer, strlen(buffer));
		    }
		    else
		    {
			Write(file, buffer, size);
		    }

		    Close(file);
		}
	    }
	    
	    /* We created both, bye bye */
	    return DOSTRUE;
	} /* try a global variable */
    } /* input was valid */

    return DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* SetVar */
