/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>

#include "dos_intern.h"
#include <proto/utility.h>
#include <exec/lists.h>
#include <dos/var.h>

/*****************************************************************************

	NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, ScanVars,

/*  SYNOPSIS */
	AROS_LHA(struct Hook *, hook, D1),
	AROS_LHA(ULONG,          flags, D2),
	AROS_LHA(APTR,           userdata, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 169, Dos)

/*  FUNCTION
	Scan local and/or global variables accordingly to specified flags. For
	each scanned variable hook function is called. Scanning process will
	continue as long as hook returns 0. If hook returns a non-zero value,
	scanning will be aborted and ScanVars will return this value.

    INPUTS
	userdata - Any user-specific data passed to hook function.
	flags - Same as in GetVar().
	hook - Hook function that will be called for each scanned variable as:
	       result = hook_function(hook,userdata,message) with ScanVarsMsg
	       structure as message parameter containing information about given
	       variable.
	
    RESULT
	!=0 returned by hook function if scan process was aborted, 0 otherwise.

    NOTES
    	ScanVarsMsg structure content is valid only during hook call.
    	See <dos/var.h> for description of ScanVarsMsg structure.

	This function is compatible with AmigaOS v4.

    EXAMPLE

    BUGS
    Currently only local variables scanning is implemented.

    SEE ALSO
	DeleteVar(), FindVar(), GetVar(), SetVar(), <dos/var.h>

    INTERNALS

*****************************************************************************/
{
	AROS_LIBFUNC_INIT

	/* We scan through the process->pr_LocalVars list */
	struct Process  *pr;
	struct LocalVar *var;
	struct ScanVarsMsg msg;
	LONG res;

	msg.sv_SVMSize = sizeof(struct ScanVarsMsg);
	msg.sv_Flags = flags;
	pr  = (struct Process *)FindTask(NULL);

	ASSERT_VALID_PROCESS(pr);

	/* not global only? */
	if(0 == (flags & GVF_GLOBAL_ONLY))
	{
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
					return(res);
			}
		}
	}
	return 0;

    AROS_LIBFUNC_EXIT
}
