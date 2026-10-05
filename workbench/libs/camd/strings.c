/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: 
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdarg.h>
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
/* Taking the arguments from &fmt+1 only works where they are passed on the
   stack one after the other (m68k, i386). On AArch64 and x86_64 they come in
   registers, so every cluster name came out as garbage. A va_list is right
   everywhere; VNewRawDoFmt() reads a %ld from it as an int, which is what
   the callers pass. */
void mysprintf(struct CamdBase *CamdBase,char *string,char *fmt,...){
	va_list args;

	va_start(args,fmt);
	VNewRawDoFmt(fmt,RAWFMTFUNC_STRING,string,args);
	va_end(args);
}
#endif

