#include <proto/intuition.h>

#include <intuition/intuition.h>

#include "error.h"
#include "baseredef.h"

void showPtrArgsText(struct afsbase *afsbase, char *string, ULONG *args) {
struct EasyStruct es={sizeof (struct EasyStruct),0,"AFFS",0,"Cancel"};

	es.es_TextFormat=string;
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	if (IntuitionBase->FirstScreen)
#endif
		EasyRequestArgs(0,&es,0,args);
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
	else
	{
#warning kprintf for error printing when gfx.hidd is not initialized
		kprintf(string,args);
		kprintf("\n");
	}
#endif
}

void showText(struct afsbase *afsbase, char *string, ...) {

	showPtrArgsText(afsbase, string,(ULONG *)(&string+1));
}

void showError(struct afsbase *afsbase, ULONG error, ...) {
char *texts[]={0,
				"No ioport",
				"No device",
				"Couldn't add disk as dosentry",
				"Disk is not validated!\n",
				"Wrong data block %ld",
				"Wrong checksum on block %ld",
				"Missing some more bitmap blocks",
				"Wrong blocktype on block %ld",
				"Read/Write Error",
				0,
				"Unknown error"
};
	if (error==ERR_ALREADY_PRINTED) return;
	if (error>=ERR_UNKNOWN) {
		showPtrArgsText(afsbase, texts[ERR_UNKNOWN],(ULONG *)&error);
	}
	else
		showPtrArgsText(afsbase, texts[error],(ULONG *)(&error+1));
}
