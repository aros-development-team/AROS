/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/intuition.h>
#include <aros/debug.h>

#include <intuition/intuition.h>

#include "error.h"
#include "baseredef.h"

void showPtrArgsText(struct AFSBase *afsbase, char *string, ULONG *args) {
struct EasyStruct es={sizeof (struct EasyStruct),0,"AFFS",0,"Cancel"};

	es.es_TextFormat=string;
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	if (IntuitionBase->FirstScreen != NULL)
	{
#endif
		EasyRequestArgs(NULL,&es,NULL,args);
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	}
	else
	{
#warning "kprintf for error printing when gfx.hidd is not initialized"
		vkprintf(string, (va_list)args);
		kprintf("\n");
	}
#endif
}

void showText(struct AFSBase *afsbase, char *string, ...) {

	showPtrArgsText(afsbase, string,(ULONG *)(&string+1));
}

void showError(struct AFSBase *afsbase, ULONG error, ...) {
char *texts[] =
{
	NULL,
	"No ioport",
	"Couldn't open device %s",
	"Couldn't add disk as dosentry",
	"Disk is not validated!\n",
	"Wrong data block %lu",
	"Wrong checksum on block %lu",
	"Missing some more bitmap blocks",
	"Wrong blocktype on block %lu",
	"Read/Write Error %ld accessing block %lu",
	NULL,
	"Unknown error"
};
	if (error == ERR_ALREADY_PRINTED)
		return;
	if (error >= ERR_UNKNOWN)
	{
		showPtrArgsText(afsbase, texts[ERR_UNKNOWN], (ULONG *)&error);
	}
	else
		showPtrArgsText(afsbase, texts[error], (ULONG *)(&error+1));
}

LONG showRetriableError(struct AFSBase *afsbase, TEXT *string, ...) {
struct EasyStruct es = {sizeof (struct EasyStruct), 0, "AFFS", string, "Retry|Cancel"};
LONG result = 0;

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	if (IntuitionBase->FirstScreen != NULL)
	{
#endif
		result = EasyRequestArgs(NULL, &es, NULL, (ULONG *)(&string+1));
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	}
	else
	{
		vkprintf(string, (va_list)(&string+1));
		kprintf("\n");
	}
#endif
	return result;
}

