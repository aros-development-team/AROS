#include <exec/rawfmt.h>

#include "filereq.h"

#include <string.h>
#include <stdarg.h>

#ifndef __AROS__
typedef void (*VOID_FUNC)();
#endif

/****************************************************************************************/

void StrCat(char *s, char *s2)
{
    strcat(s, s2);
}

/****************************************************************************************/

static void filltable(char **table, char *buff, char fill)
{
    static char c;
    
    *table++ = buff;
    
    for(;;)
    {
         c = *buff++;
	 if (c == '\0') break;
	 if (c == fill)
	 {
	     buff[-1] = '\0';
	     *table++ = buff;
	 }
    }
}

/****************************************************************************************/

void FillNewLineTable (char **table, char *buff)
{
    filltable(table, buff, '\n');
}

/****************************************************************************************/

void FillBarTable (char **table, char *buff)
{
    filltable(table, buff,  '|');
}

/****************************************************************************************/

#ifdef __AROS__
AROS_UFH2 (void, puttostr,
	AROS_UFHA(UBYTE, chr, D0),
	AROS_UFHA(STRPTR *,strPtrPtr,A3)
)
{
    AROS_USERFUNC_INIT
#else
void puttostr(REGPARAM(d0, UBYTE, chr),
    	      REGPARAM(a3, STRPTR *, strPtrPtr))
{
#endif
    *(*strPtrPtr)= chr;
    (*strPtrPtr) ++;
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/****************************************************************************************/

#ifdef __AROS__
AROS_UFH2 (void, CountBarsAndChars,
	AROS_UFHA(UBYTE, chr, D0),
	AROS_UFHA(ULONG *,ptr,A3)
)
{
    AROS_USERFUNC_INIT
#else
void CountBarsAndChars(REGPARAM(d0, UBYTE, chr),
    	     	       REGPARAM(a3, ULONG *, ptr))
{
#endif
    if (chr == '|') (ptr[0])++;
    (ptr[1])++;
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/****************************************************************************************/

#ifdef __AROS__
AROS_UFH2 (void, CountNewLinesAndChars,
	AROS_UFHA(UBYTE, chr, D0),
	AROS_UFHA(ULONG *,ptr,A3)
)
{
    AROS_USERFUNC_INIT
#else
void CountNewLinesAndChars(REGPARAM(d0, UBYTE, chr),
    	     	           REGPARAM(a3, ULONG *, ptr))
{
#endif
    if (chr == '\n') (ptr[0])++;
    (ptr[1])++;
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/****************************************************************************************/

APTR DofmtArgs (char *buff, char *fmt, ...)
{
    APTR retval;
    char *str = buff;
#ifdef __AROS__
    va_list ap;

    /* Some AROS architectures don't have uniform varadics
     * (ie an array of IPTR), so they can't use RawDoFmt()
     * - we will use VNewRawDoFmt() instead.
     */
    va_start(ap, fmt);
    retval = VNewRawDoFmt(fmt, (VOID_FUNC)RAWFMTFUNC_STRING, &str, ap);
    va_end(ap);
#else
    retval = RawDoFmt(fmt, &fmt + 1, (VOID_FUNC)puttostr, &str);
#endif

    return retval;
}

/****************************************************************************************/

APTR Dofmt (char *buff, char *fmt, APTR args)
{
    char *str = buff;

    return RawDoFmt(fmt, args, (VOID_FUNC)puttostr, &str);
   
}

/****************************************************************************************/

APTR DofmtCount (char *fmt, APTR args, ULONG *ptr, int mode)
{
    ptr[0] = ptr[1] = 1;
    
    return RawDoFmt(fmt,
    		    args,
		    (mode ? ((VOID_FUNC)CountBarsAndChars) : ((VOID_FUNC)CountNewLinesAndChars) ),
		    ptr); 
}

/****************************************************************************************/

void ShortDelay(void)
{
    Delay(5);
}

/****************************************************************************************/

IPTR LoopReqHandler(struct rtHandlerInfo *rthi)
{
    IPTR handler_retval, sigs;

    do
    {
       if (rthi->DoNotWait)
	    sigs = 0;
	else
	    sigs = Wait(rthi->WaitMask);

	handler_retval = rtReqHandlerA(rthi, sigs, NULL);
	
    } while (handler_retval == CALL_HANDLER);
    
    return handler_retval;
    
}

/****************************************************************************************/

void DoWaitPointer(struct Window *win, int doit, int setpointer)
{
    if (win && doit)
    {
        if (setpointer)
	    rtSetWaitPointer(win);
	else
	    ClearPointer(win);
    }
    
}

/****************************************************************************************/

APTR DoLockWindow(struct Window *win, int doit, APTR lock, int lockit)
{
    if (!doit || !win) return NULL;
    
    if (lockit) return rtLockWindow(win);
    
    rtUnlockWindow(win, lock);
    
    return NULL;
}

/****************************************************************************************/

void SetWinTitleFlash(struct Window *win, char *str)
{
    DisplayBeep(win->WScreen);
    SetWindowTitles(win, str, (UBYTE *)-1);
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
