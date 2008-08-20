/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id: error.c 27651 2008-01-05 00:27:23Z error $
*/

/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      added disk check option for broken disks
 * 04-jan-2008 [Tomasz Wiszkowski]      corrected tabulation
 */


#include <proto/intuition.h>
#include <aros/debug.h>

#include <intuition/intuition.h>

#include "error.h"
#include "baseredef.h"

/*
 * showReqType matches adequate option[] in showPtrArgsText.
 */
enum showReqType 
{
	Req_Cancel = 0,
	Req_RetryCancel,
	Req_CheckCancel,
	Req_ContinueCancel,
	Req_Continue
};

/*
 * showReqStruct: a group describing particular requester: 
 * - text - content to be displayed
 * - type - set of buttons for the requester
 */
struct showReqStruct 
{
	char* text;
	enum showReqType type;
};

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

	es.es_TextFormat=string;
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	if (IntuitionBase->FirstScreen != NULL)
	{
#endif
		return EasyRequestArgs(NULL,&es,NULL,args);
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	}
	else
	{
#warning "kprintf for error printing when gfx.hidd is not initialized"
		vkprintf(string, args);
		kprintf("\n");
	}
#endif
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
	struct showReqStruct texts[] =
	{
		{NULL, Req_Cancel },
		{"No ioport", Req_Cancel },
		{"Couldn't open device %s", Req_Cancel },
		{"Couldn't add disk as dosentry", Req_Cancel },
		{"Disk is not validated!", Req_CheckCancel },
		{"Wrong data block %lu", Req_Cancel },
		{"Wrong checksum on block %lu", Req_CheckCancel },
		{"Missing some more bitmap blocks", Req_Cancel },
		{"Wrong blocktype on block %lu", Req_CheckCancel },
		{"Read/Write Error %ld accessing block %lu", Req_Cancel },
		{"*** This may be a non-AFS disk. ***\n"
			"Any attempt to fix it in this case may render the original\n"
			"file system invalid, and its contents unrecoverable.\n\n"
			"Please select what to do", Req_ContinueCancel },	
		{"Block %lu used twice", Req_Cancel},
		{"Block %lu is located outside volume scope\nand will be removed.", Req_Continue},
		{"Repairing disk structure will lead to data loss.\n"
			"It's best to make a backup before proceeding.\n\n"
			"Please select what to do.", Req_ContinueCancel },
		{NULL, Req_Cancel },
		{"Unknown error", Req_Cancel}
	};

	if (error == ERR_ALREADY_PRINTED)
		return 0;
	if (error >= ERR_UNKNOWN)
	{
		return showPtrArgsText(afsbase, texts[ERR_UNKNOWN].text, texts[ERR_UNKNOWN].type, (ULONG *)&error);
	}

	return showPtrArgsText(afsbase, texts[error].text, texts[error].type, (ULONG *)(&error+1));
}


/* vim: set ts=3 noet fdm=marker fmr={,}: */
