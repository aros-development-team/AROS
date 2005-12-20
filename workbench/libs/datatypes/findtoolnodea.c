/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "datatypes_intern.h"
#include <proto/utility.h>

/*****************************************************************************

    NAME */

        AROS_LH2(struct ToolNode *, FindToolNodeA,

/*  SYNOPSIS */
	AROS_LHA(struct List    *, toollist, A0),
	AROS_LHA(struct TagItem *, attrs   , A1),

/*  LOCATION */
	struct Library *, DataTypesBase, 41, DataTypes)

/*  FUNCTION

    Search for a specific tool in a list of given tool nodes.

    INPUTS

    toollist  --  a list or a struct ToolNode * (which will be skipped) to
                  search in; may be NULL.

    attrs     --  search tags; if NULL, the result of the function will
                  simply be the following node.

    TAGS

    TOOLA_Program     --  name of the program to search for

    TOOLA_Which       --  one of the TW_#? types

    TOOLA_LaunchType  --  launch mode: TF_SHELL, TF_WORKBENCH or TF_RX

    RESULT

    A pointer to a ToolNode describing the search result (NULL for failure).

    NOTES

    The entries in dt->dtn_ToolList are valid as long as a lock is kept on
    the data type 'dt' (ObtainDataTypeA() or LockDataType()).

    EXAMPLE

    BUGS

    SEE ALSO

    LaunchToolA()

    INTERNALS

    HISTORY

    7.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR program;
    WORD   which;
    WORD   ltype;

    struct ToolNode *tNode;

    if(toollist == NULL)
	return NULL;

    program = (STRPTR)GetTagData(TOOLA_Program, NULL, attrs);
    which   =   (WORD)GetTagData(TOOLA_Which, -1, attrs);
    ltype   =   (WORD)GetTagData(TOOLA_LaunchType, -1, attrs);

    ForeachNode(toollist, tNode)
    {
	/* Match program name */
	if(program != NULL)
	{
	    if(Stricmp(tNode->tn_Tool.tn_Program, program) != 0)
		continue;
	}

	/* Check type */
	if(which != -1)
	{
	    if(which != tNode->tn_Tool.tn_Which)
		continue;
	}
	
	/* Check launch type */
	if(ltype != -1)
	{
	    if(ltype != (tNode->tn_Tool.tn_Flags & TF_LAUNCH_MASK))
		continue;
	}
	
	return tNode;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* FindToolNodeA */

