/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaOS specific ReqTools initialization code.
    Lang: English.
*/

/****************************************************************************************/

#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <libraries/reqtools.h>
#include <intuition/classes.h>
#include <proto/exec.h>

#include "reqtools_intern.h"
#include "general.h"
#include "rtfuncs.h"

#define VERSION 39
#define REVISION 4

#define NAME_STRING      "reqtools.library"
#define VERSION_STRING   "$VER: reqtools 39.4 (17.01.2002)\r\n"

/****************************************************************************************/

extern const char name[];
extern const char version[];
extern const APTR inittable[4];
extern void *const functable[];
extern struct ReqToolsBase *libinit();
extern const char libend;

/****************************************************************************************/

int entry(void)
{
  return -1;
}

/****************************************************************************************/

const struct Resident ROMTag =
{
  RTC_MATCHWORD,
  (struct Resident *)&ROMTag,
  (APTR)&libend,
  RTF_AUTOINIT,
  VERSION,
  NT_LIBRARY,
  0,
  (char *)name,
  (char *)&version[6],
  (ULONG *)inittable
};

/****************************************************************************************/

const char name[]    = NAME_STRING;
const char version[] = VERSION_STRING;

const APTR inittable[4] =
{
    (APTR)sizeof(struct ReqToolsBase),
    (APTR)functable,
    NULL,
    (APTR)&libinit
};

/****************************************************************************************/

#define __RT_DEFINEVARS
#include "globalvars.h"
#undef __RT_DEFINEVARS

/****************************************************************************************/

struct ReqToolsBase * SAVEDS ASM libinit(REGPARAM(d0, struct ReqToolsBase *, RTBase),
    	    	    	    	    	    REGPARAM(a0, BPTR, segList))
{
    ReqToolsBase = RTBase;
    SysBase = *(struct ExecBase **)4L;
    
    ReqToolsBase->LibNode.lib_Node.ln_Type  = NT_LIBRARY;
    ReqToolsBase->LibNode.lib_Node.ln_Name  = (char *)name;
    ReqToolsBase->LibNode.lib_Flags         = LIBF_SUMUSED|LIBF_CHANGED;
    ReqToolsBase->LibNode.lib_Version       = VERSION;
    ReqToolsBase->LibNode.lib_Revision      = REVISION;
    ReqToolsBase->LibNode.lib_IdString      = (char *)&version[6];
    
    return RTFuncs_Init(RTBase, segList);
}

/****************************************************************************************/

extern ULONG ASM SAVEDS GetString (REGPARAM(a1, UBYTE *, stringbuff),
				   REGPARAM(d0, LONG, maxlen),
				   REGPARAM(a2, char *, title),
				   REGPARAM(d1, ULONG, checksum),
				   REGPARAM(d2, ULONG *, value),
				   REGPARAM(d3, LONG, mode),
				   REGPARAM(d4, struct rtReqInfo *, reqinfo),
				   REGPARAM(a0, struct TagItem *, taglist));


/****************************************************************************************/

#ifdef USE_STACKSWAP

/****************************************************************************************/

#ifdef __AROS__
#error No StackSwap support for AROS yet
#endif

#ifndef __GNUC__
#error Only StackSwap support for GCC compiler for now
#endif

/****************************************************************************************/

struct MyStackSwapStruct
{
    struct StackSwapStruct  sss;
    UBYTE   	    	    *stringbuff;
    LONG    	    	    maxlen;
    char    	    	    *title;
    ULONG   	    	    checksum;
    ULONG   	    	    *value;
    LONG    	    	    mode;
    struct rtReqInfo 	    *reqinfo;
    struct TagItem  	    *taglist;
};

#ifndef MIN_STACK
#define MIN_STACK 4096
#endif

/****************************************************************************************/

static ULONG CheckStack_GetString(UBYTE *stringbuff,
    	    	    	    	  LONG maxlen,
				  char *title,
				  ULONG checksum,
				  ULONG *value,
				  LONG mode,
				  struct rtReqInfo *reqinfo,
				  struct TagItem *taglist)
{
/* I could not manage to get correct code to be generated when
   using asm("a7") or asm("sp") to access sp!? :-( */
   
#define sp ((IPTR)(&sss))

    struct MyStackSwapStruct 	    	sss;
    register struct MyStackSwapStruct 	*sssptr = &sss;
    struct Task     	    	    	*me = FindTask(NULL);
    register ULONG   	       	    	retval = 0;
    
    if ((sp <= (IPTR)me->tc_SPLower) ||
        (sp >= (IPTR)me->tc_SPUpper) ||
	(sp - MIN_STACK < (IPTR)me->tc_SPLower))
    {
    	sssptr->sss.stk_Lower = AllocVec(MIN_STACK, MEMF_PUBLIC);
	if (sssptr->sss.stk_Lower)
	{
	    sssptr->sss.stk_Upper = ((IPTR)sssptr->sss.stk_Lower) + MIN_STACK;
	    sssptr->sss.stk_Pointer = (APTR) sssptr->sss.stk_Upper;
	    
	    sssptr->stringbuff  = stringbuff;
	    sssptr->maxlen  	= maxlen;
	    sssptr->title   	= title;
	    sssptr->checksum 	= checksum;
	    sssptr->value   	= value;
	    sssptr->mode    	= mode;
	    sssptr->reqinfo 	= reqinfo;
	    sssptr->taglist 	= taglist;
	    
	    StackSwap(&sssptr->sss);
	    
    	    retval = GetString(sssptr->stringbuff,
	    	    	       sssptr->maxlen,
			       sssptr->title,
			       sssptr->checksum,
			       sssptr->value,
			       sssptr->mode,
			       sssptr->reqinfo,
			       sssptr->taglist);
			       	    
	    StackSwap(&sssptr->sss);
	    
	    FreeVec(sssptr->sss.stk_Lower);
	}
    }
    else
    {
    	retval = GetString(stringbuff, maxlen, title, checksum, value, mode, reqinfo, taglist);
    }
    
    return retval;
}

/****************************************************************************************/

#define GETSTRING CheckStack_GetString

#else

#define GETSTRING GetString

/****************************************************************************************/

#endif /* USE_STACKSWAP */

/****************************************************************************************/

ULONG SAVEDS ASM librtEZRequestA(REGPARAM(a1, char *, bodyfmt),
                                 REGPARAM(a2, char *, gadfmt),
			         REGPARAM(a3, struct rtReqInfo *, reqinfo),
			         REGPARAM(a4, APTR, argarray),
			         REGPARAM(a0, struct TagItem *, taglist))
{
    return GETSTRING(bodyfmt,
    		     (LONG)argarray,
		     gadfmt,
		     0,
		     NULL,
		     IS_EZREQUEST,
		     reqinfo,
		     taglist);
}

/****************************************************************************************/

ULONG SAVEDS ASM librtGetStringA(REGPARAM(a1, UBYTE *, buffer),
    	    	    	    	 REGPARAM(d0, ULONG, maxchars),
				 REGPARAM(a2, char *, title),
				 REGPARAM(a3, struct rtReqInfo *, reqinfo),
				 REGPARAM(a0, struct TagItem *, taglist))
{
    return GETSTRING(buffer,
    		     maxchars,
		     title,
		     0,
		     NULL,
		     ENTER_STRING,
		     reqinfo,
		     taglist);
}

/****************************************************************************************/

ULONG SAVEDS ASM librtGetLongA(REGPARAM(a1, ULONG *, longptr),
    	    	    	       REGPARAM(a2, char *, title),
			       REGPARAM(a3, struct rtReqInfo *, reqinfo),
			       REGPARAM(a0, struct TagItem *, taglist))
{
    return GETSTRING(NULL,
    		     0,
		     title,
		     0,
		     longptr,
		     ENTER_NUMBER,
		     reqinfo,
		     taglist);
}

/****************************************************************************************/

/* NOTE: Used by powerpacker.library/ppGetPassword() */

BOOL SAVEDS ASM librtInternalGetPasswordA(REGPARAM(a1, UBYTE *, buffer),
					  REGPARAM(d1, ULONG, checksum),
					  REGPARAM(d2, PWCALLBACKFUNPTR, pwcallback),
					  REGPARAM(a3, struct rtReqInfo *, reqinfo),
					  REGPARAM(a0, struct TagItem *, taglist))
{
    buffer[0] = '\0';

    return GETSTRING(buffer,
    		     16,
    		     "Password",
		     checksum & 0xffff,
		     (ULONG *) pwcallback,
		     CHECK_PASSWORD,
		     reqinfo,
		     taglist);
}

/****************************************************************************************/

/* NOTE: Used by powerpacker.library/ppEnterPassword */

BOOL SAVEDS ASM librtInternalEnterPasswordA(REGPARAM(a1, UBYTE *, buffer),
					    REGPARAM(d2, PWCALLBACKFUNPTR, pwcallback),
					    REGPARAM(a3, struct rtReqInfo *, reqinfo),
					    REGPARAM(a0, struct TagItem *, taglist))
{
    return GETSTRING(buffer,
    		     16,
    		     "Password",
		     0,
		     (ULONG *) pwcallback,
		     ENTER_PASSWORD,
		     reqinfo,
		     taglist);
}

/****************************************************************************************/

extern void libopen(void);
extern void libclose(void);
extern void libexpunge(void);
extern void libnull(void);

extern void AllocRequestA(void);
extern void FreeRequest(void);
extern void FreeReqBuffer(void);
extern void ChangeReqAttrA(void);
extern void FileRequestA(void);
extern void FreeFileList(void);
extern void PaletteRequestA(void);
extern void GetVScreenSize(void);

/****************************************************************************************/

void *const functable[]=
{
    RTFuncs_Open,
    RTFuncs_Close,
    RTFuncs_Expunge,
    RTFuncs_Null,
    AllocRequestA,
    FreeRequest,
    FreeReqBuffer,
    ChangeReqAttrA,
    FileRequestA,
    FreeFileList,
    librtEZRequestA,
    librtGetStringA,
    librtGetLongA,
    librtInternalGetPasswordA,
    librtInternalEnterPasswordA,
    FileRequestA,
    PaletteRequestA,
    RTFuncs_rtReqHandlerA,
    RTFuncs_rtSetWaitPointer,
    GetVScreenSize,
    RTFuncs_rtSetReqPosition,
    RTFuncs_rtSpread,
    RTFuncs_ScreenToFrontSafely,
    FileRequestA,
    RTFuncs_CloseWindowSafely,
    RTFuncs_rtLockWindow,
    RTFuncs_rtUnlockWindow,
    RTFuncs_LockPrefs,
    RTFuncs_UnlockPrefs,
    (void *)-1L
};

/****************************************************************************************/

const char libend = 0;

/****************************************************************************************/

