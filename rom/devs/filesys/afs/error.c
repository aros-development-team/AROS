/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      added disk check option for broken disks
 * 04-jan-2008 [Tomasz Wiszkowski]      corrected tabulation
 */


#include <proto/intuition.h>
#include <aros/debug.h>
#include <exec/rawfmt.h>

#include <intuition/intuition.h>

#include "error.h"
#include "errstrings.h"
#include "baseredef.h"


/*
 * displays requester on screen or puts text to the debug buffer
 */
LONG showPtrArgsText(struct AFSBase *afsbase, char *string, enum showReqType type, ULONG *args) 
{
	char* options[] =
	{
		"Cancel",
		"Retry|Cancel",
		"Check disk|Cancel",
		"Continue|Cancel",
		"Continue",
		NULL
	};
	struct EasyStruct es={sizeof (struct EasyStruct),0,"AFFS",0,options[type]};
	struct IntuitionBase *IntuitionBase;

	IntuitionBase = (APTR)OpenLibrary("intuition.library", 39);
	if (IntuitionBase != NULL)
	{
	    es.es_TextFormat=string;
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	    if (IntuitionBase->FirstScreen != NULL)
	    {
#endif
		return EasyRequestArgs(NULL,&es,NULL,args);
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	    }
#endif
	    CloseLibrary(IntuitionBase);
	}
	else
	{
	    /* We use serial for error printing when gfx.hidd is not initialized */
	    RawDoFmt(string, args, RAWFMTFUNC_SERIAL, NULL);
	    RawPutChar('\n');
	}
	return 0;
}

void showText(struct AFSBase *afsbase, char *string, ...) 
{
	showPtrArgsText(afsbase, string, Req_Cancel, (ULONG *)(&string+1));
}

LONG showRetriableError(struct AFSBase *afsbase, TEXT *string, ...) 
{
	return showPtrArgsText(afsbase, string, Req_RetryCancel, (ULONG*)(&string+1)); 
}

LONG showError(struct AFSBase *afsbase, ULONG error, ...) 
{
	if (error == ERR_ALREADY_PRINTED)
		return 0;
	if (error >= ERR_UNKNOWN)
	{
		return showPtrArgsText(afsbase, texts[ERR_UNKNOWN].text, texts[ERR_UNKNOWN].type, (ULONG *)&error);
	}

	return showPtrArgsText(afsbase, texts[error].text, texts[error].type, (ULONG *)(&error+1));
}


/* vim: set ts=3 noet fdm=marker fmr={,}: */
