/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Flush Caches
    Lang: english
*/

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH3(void, CacheClearE,

/*  SYNOPSIS */
	AROS_LHA(APTR,  address, A0),
	AROS_LHA(ULONG, length,  D0),
	AROS_LHA(ULONG, caches,  D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 107, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return;
} /* CacheClearE */

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, CacheClearU,

/*  LOCATION */
	struct ExecBase *, SysBase, 106, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return;
} /* CacheClearU */

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, CacheControl,

/*  LOCATION */
	struct ExecBase *, SysBase, 108, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return;
} /* CacheControl */

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, CachePostDMA,

/*  LOCATION */
	struct ExecBase *, SysBase, 128, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return;
} /* CachePostDMA */

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, CachePreDMA,

/*  LOCATION */
	struct ExecBase *, SysBase, 127, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    return;
} /* CachePreDMA */

