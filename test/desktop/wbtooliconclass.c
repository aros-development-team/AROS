#include <aros/asmcall.h>
#include <exec/types.h>
#include <intuition/classusr.h>

#include <proto/alib.h>
#include <proto/intuition.h>

#include "wbtooliconclass.h"

AROS_UFH3(IPTR,wbToolIconClassDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
	ULONG retval;

	switch(msg->MethodID)
	{
		default:
			retval=DoSuperMethodA(cl, obj, msg);
			break;
	}

	return retval;
}

