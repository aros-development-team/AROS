/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"

#  undef DEBUG
#  define DEBUG 1
#  include AROS_DEBUG_H_FILE

ULONG mystrlen(char *string){
	ULONG ret=0;
	while(string[ret]!=0) ret++;
	return ret;
}

ASM void stuffChar( REG(d0) UBYTE in,REG(a3) char **stream){
	stream[0]++;
	stream[0][-1]=in;
}


void mysprintf(struct CamdBase *CamdBase,char *string,char *fmt,...){
	void *start=&fmt+1;

	// You should change your proto-file, if there is a warning about const.
	CONST_STRPTR string2=string;

	D(bug("mystprintf_start\n"));
	RawDoFmt(
		 fmt,
		 start,
		 (VOID_FUNC)stuffChar,
		 (APTR)&string2
		 );
	D(bug("mystprintf_end\n"));
}

/*
main(){
	char testing[100];
	mysprintf(testing,"asfd %ld %ld %ld %ld %ld",5,6,7,8,9);
	printf("-%s-\n",testing);
}
*/

