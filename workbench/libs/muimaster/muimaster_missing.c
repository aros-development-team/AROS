#include <exec/types.h>
#include <intuition/classes.h>
#include <utility/tagitem.h>

/***************************************************************************

    NAME */
	AROS_LH2(APTR, MUI_AllocAslRequest,

/*  SYNOPSIS */
	AROS_LHA(ULONG           , reqType, D0),
	AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 8, MUIMaster)

/*  FUNCTION
	.

******/

{
	return 0;
}

/***************************************************************************

    NAME */
	AROS_LH2(BOOL, MUI_AslRequest,

/*  SYNOPSIS */
	AROS_LHA(APTR            , requester, A0),
	AROS_LHA(struct TagItem *, tagList,   A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 9, MUIMaster)

/*  FUNCTION
	.

******/

{
	return 0;
}

/***************************************************************************

    NAME */
	AROS_LH1(VOID, MUI_FreeAslRequest,

/*  SYNOPSIS */
	AROS_LHA(APTR, requester, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 10, MUIMaster)

/*  FUNCTION
	.

******/

{
	return;
}

/***************************************************************************

    NAME */
	AROS_LH0(LONG, MUI_Error,

/*  SYNOPSIS */

/*  LOCATION */
	struct Library *, MUIMasterBase, 11, MUIMaster)

/*  FUNCTION
	.

******/

{
	return 0;
}

/***************************************************************************

    NAME */
	AROS_LH1(LONG, MUI_SetError,

/*  SYNOPSIS */
	AROS_LHA(LONG, num, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 12, MUIMaster)

/*  FUNCTION
	.

******/

{
	return 0;
}

/*** EOF ***/
