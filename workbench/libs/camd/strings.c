/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include "camd_intern.h"

ULONG mystrlen(char *string){
	ULONG ret=0;
	while(string[ret]!=0) ret++;
	return ret;
}

BOOL mystrcmp(char *one,char *two){
  while(*one==*two){
    if(*one==0) return TRUE;
    one++;
    two++;
  }
  return FALSE;
}

char *findonlyfilename(char *pathfile){
  char *temp=pathfile;
  while(*pathfile!=0){
    if(*pathfile=='/') temp=pathfile+1;
    if(*pathfile==':') temp=pathfile+1;
    pathfile++;
  }
  return temp;
}

#ifdef __amigaos4__
ASM void stuffChar( REG(d0, UBYTE in),REG(a3, char **stream)){
#else
ASM void stuffChar( REG(d0) UBYTE in,REG(a3) char **stream){
#endif
	stream[0]++;
	stream[0][-1]=in;
}


#ifndef __amigaos4__
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
#endif

