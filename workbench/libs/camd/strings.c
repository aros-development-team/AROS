/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"

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

	RawDoFmt(
		 fmt,
		 start,
		 (VOID_FUNC)stuffChar,
		 (APTR)&string2
		 );
}

