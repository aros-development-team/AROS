/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>

/******************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Exception,

/*  LOCATION */
	struct ExecBase *, SysBase, 11, Exec)

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
    struct Task * this = FindTask (NULL);
    BYTE nestCnt;
    ULONG flags;

    this->tc_Flags &= ~TF_EXCEPT;

    nestCnt = SysBase->IDNestCnt;
    SysBase->IDNestCnt = 0;

    while ((flags = (this->tc_SigExcept & this->tc_SigRecvd)))
    {
	flags ^= this->tc_SigExcept;
	flags ^= this->tc_SigRecvd;

	Enable ();

	this->tc_SigExcept = (*((ULONG_FUNC)(this->tc_ExceptCode))) (this->tc_ExceptData);

	Disable ();
    }

    SysBase->IDNestCnt = nestCnt;
} /* Exception */
