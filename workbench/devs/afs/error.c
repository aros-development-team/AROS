#include <proto/intuition.h>

#include <intuition/intuition.h>

#include "error.h"

void showPtrArgsText(char *string, ULONG *args) {
struct EasyStruct es[]={sizeof (struct EasyStruct),0,"AFFS",0,"Cancel"};

	es->es_TextFormat=string;
	EasyRequestArgs(0,es,0,args);
}

void showText(char *string, ...) {

	showPtrArgsText(string,(ULONG *)(&string+1));
}

void showError(ULONG error, ...) {
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
		showPtrArgsText(texts[ERR_UNKNOWN],(ULONG *)&error);
	}
	else
		showPtrArgsText(texts[error],(ULONG *)(&error+1));
}
