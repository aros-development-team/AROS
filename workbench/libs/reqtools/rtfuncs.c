#include "filereq.h"
#include "boopsi.h"
#include "rtfuncs.h"
#include "globalvars.h"

#include <devices/conunit.h>

#ifdef __AROS__

#include <aros/debug.h>
#include <aros/macros.h>

#else

#define D(x)
#define AROS_ASMSYMNAME(x)  x
#define AROS_LONG2BE(x)     x
#define AROS_WORD2BE(x)     x

#endif

/****************************************************************************************/

struct rtWindowLock
{
    struct Requester     rtwl_Requester;
    LONG                 rtwl_Magic;
    struct rtWindowLock *rtwl_RequesterPtr;
    ULONG                rtwl_LockCount;
    ULONG                rtwl_ReqInstalled;

    /* To save window parameters */
    APTR                 rtwl_Pointer;
    BYTE                 rtwl_PtrHeight;
    BYTE                 rtwl_PtrWidth;
    BYTE                 rtwl_XOffset;
    BYTE                 rtwl_YOffset;
    WORD                 rtwl_MinWidth;
    WORD                 rtwl_MaxWidth;
    WORD                 rtwl_MinHeight;
    WORD                 rtwl_MaxHeight;
};

/****************************************************************************************/

SAVEDS ASM struct ReqToolsBase *RTFuncs_Init(REGPARAM(d0, struct ReqToolsBase *, RTBase),
    	    	    	    	    	     REGPARAM(a0, BPTR, segList))					      
{
#ifdef __AROS__
    /* SysBase is setup in reqtools_init.c */
#else
    SysBase = *(struct ExecBase **)4L;

    RTBase->SegList = segList;
#endif

    InitSemaphore(&RTBase->ReqToolsPrefs.PrefsSemaphore);
    RTBase->ReqToolsPrefs.PrefsSize = RTPREFS_SIZE;

    /* Set default preferences */
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].Size = 75;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].LeftOffset = 25;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].TopOffset = 18;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].MinEntries = 10;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].MaxEntries = 50;

    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].Size = 65;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].LeftOffset = 25;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].TopOffset = 18;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].MinEntries = 6;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].MaxEntries = 10;

    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].Size = 65;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].LeftOffset = 25;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].TopOffset = 18;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].MinEntries = 6;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].MaxEntries = 10;

    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].Size = 65;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].LeftOffset = 25;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].TopOffset = 18;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].MinEntries = 6;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].MaxEntries = 10;

    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].Size = 65;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].LeftOffset = 25;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].TopOffset = 18;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].MinEntries = 6;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].MaxEntries = 10;

    //    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].Size = 65;
    //    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].LeftOffset = 25;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].TopOffset = 18;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].MinEntries = 6;
    RTBase->ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].MaxEntries = 10;

    return RTBase;
}

/****************************************************************************************/

SAVEDS ASM struct ReqToolsBase *RTFuncs_Open(REGPARAM(a6, struct ReqToolsBase *, RTBase),
    	    	    	    	    	     REGPARAM(d0, ULONG, ver))
{
    if (DOSBase == NULL)
    {
        UBYTE configbuffer[RTPREFS_SIZE];
	
        DOSBase = RTBase->DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
        if (DOSBase == NULL)
            return NULL;

	    
	/* Read config file */
	
        D(bug("reqtools.library: Inside libopen func. Reading config file\n"));
	
	memset(configbuffer, 0, sizeof(configbuffer));
	
	if (GetVar("ReqTools.prefs",
		   configbuffer,
		   sizeof(configbuffer),
		   GVF_BINARY_VAR | GVF_GLOBAL_ONLY | LV_VAR | GVF_DONT_NULL_TERM) == RTPREFS_SIZE)
	{
	    UBYTE *configptr = configbuffer;
	    ULONG val;
	    WORD  i;

	    D(bug("reqtools.library: Inside libopen func. Configfile loaded successfully\n"));
	    
#define READ_ULONG 	*((ULONG *)configptr);configptr += sizeof(ULONG)
#define READ_UWORD 	*((UWORD *)configptr);configptr += sizeof(UWORD)
#define RTPREFS 	(RTBase->ReqToolsPrefs)

	    val = READ_ULONG;
	    RTPREFS.Flags = AROS_LONG2BE(val);

	    for(i = 0;i < RTPREF_NR_OF_REQ; i++)
	    {
		val = READ_ULONG;
		RTPREFS.ReqDefaults[i].Size = AROS_LONG2BE(val);

		val = READ_ULONG;
		RTPREFS.ReqDefaults[i].ReqPos = AROS_LONG2BE(val);

		val = READ_UWORD;
		RTPREFS.ReqDefaults[i].LeftOffset = AROS_WORD2BE(val);

		val = READ_UWORD;
		RTPREFS.ReqDefaults[i].TopOffset = AROS_WORD2BE(val);

		val = READ_UWORD;
		RTPREFS.ReqDefaults[i].MinEntries = AROS_WORD2BE(val);

		val = READ_UWORD;
		RTPREFS.ReqDefaults[i].MaxEntries = AROS_WORD2BE(val);	    
	    }
	    	
	}
	
    } /* if (DOSBase == NULL) */
    
    if(IntuitionBase == NULL)
	IntuitionBase = RTBase->IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    if(IntuitionBase == NULL)
	return NULL;

    if(GfxBase == NULL)
	GfxBase = RTBase->GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if(GfxBase == NULL)
	return NULL;
    
    if(UtilityBase == NULL)
	UtilityBase = RTBase->UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37);
    if(UtilityBase == NULL)
	return NULL;

    if(GadToolsBase == NULL)
	GadToolsBase = RTBase->GadToolsBase = OpenLibrary("gadtools.library", 37);
    if(GadToolsBase == NULL)
	return NULL;

    if(LayersBase == NULL)
	LayersBase = OpenLibrary("layers.library", 37);
    if(LayersBase == NULL)
	return NULL;

    if(LocaleBase == NULL)
	LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 37);
    if(LocaleBase == NULL)
	return NULL;

    D(bug("reqtools.library: Inside libopen func. Libraries opened successfully.\n"));

    if (ConsoleDevice == NULL)
    {
        iorequest.io_Message.mn_Length = sizeof(iorequest);
	
        if (OpenDevice("console.device", CONU_LIBRARY, (struct IORequest *)&iorequest, 0))
	{
	    return NULL;
	}
	ConsoleDevice = iorequest.io_Device;
    }
    if (ConsoleDevice == NULL)
        return NULL;

    D(bug("reqtools.library: Inside libopen func. Console.device opened successfully.\n"));
	
    if (ButtonImgClass == NULL)
    {
        ButtonImgClass = MakeClass(NULL, IMAGECLASS, NULL, sizeof(struct LocalObjData), 0);
	if (ButtonImgClass)
	{
	    ButtonImgClass->cl_Dispatcher.h_Entry = (APTR)AROS_ASMSYMNAME(myBoopsiDispatch);
	    ButtonImgClass->cl_Dispatcher.h_SubEntry = NULL;
	    ButtonImgClass->cl_UserData = (IPTR)RTBase;
	}
    }
    if (ButtonImgClass == NULL)
        return NULL;
    
    D(bug("reqtools.library: Inside libopen func. ButtonImgClass create successfully.\n"));

#ifndef __AROS__
    /* I have one more opener. */
    RTBase->LibNode.lib_Flags &= ~LIBF_DELEXP;
    RTBase->LibNode.lib_OpenCnt++;
#endif
    
    D(bug("reqtools.library: Inside libopen func. Returning success.\n"));
    
    return RTBase;
}

/****************************************************************************************/

SAVEDS ASM BPTR RTFuncs_Close(REGPARAM(a6, struct ReqToolsBase *, RTBase))
{
#ifndef __AROS__
    /* I have one fewer opener. */
    RTBase->LibNode.lib_OpenCnt--;
    
    if((RTBase->LibNode.lib_Flags & LIBF_DELEXP) != 0)
    {
    	/* CHECKME: used expunge() from reqtools_intern.h. */
	
	if(RTBase->LibNode.lib_OpenCnt == 0)
	    return RTFuncs_Expunge(RTBase);
	
	RTBase->LibNode.lib_Flags &= ~LIBF_DELEXP;
    }
#endif
    
    return NULL;
}

/****************************************************************************************/

SAVEDS ASM BPTR RTFuncs_Expunge(REGPARAM(a6, struct ReqToolsBase *, RTBase))
{
    BPTR ret = NULL;

#ifndef __AROS__
    if(RTBase->LibNode.lib_OpenCnt != 0)
    {
	/* Set the delayed expunge flag and return. */
	RTBase->LibNode.lib_Flags |= LIBF_DELEXP;
	return NULL;
    }
    
    /* Get rid of the library. Remove it from the list. */
    Remove(&RTBase->LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = RTBase->SegList;
#endif

    D(bug("reqtools.library: Inside libexpunge func. Freeing ButtonImgClass.\n"));

    if (ButtonImgClass) FreeClass(ButtonImgClass);

    D(bug("reqtools.library: Inside libexpunge func. Closing console.device.\n"));
    
    if (ConsoleDevice) CloseDevice((struct IORequest *)&iorequest);

    D(bug("reqtools.library: Inside libexpunge func. Closing libraries.\n"));
    
    CloseLibrary((struct Library *)DOSBase);
    CloseLibrary((struct Library *)IntuitionBase);
    CloseLibrary((struct Library *)UtilityBase);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)LocaleBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary(LayersBase);

    D(bug("reqtools.library: Inside libexpunge func. Freeing libbase.\n"));

#ifndef __AROS__
    /* Free the memory. */
    FreeMem((char *)RTBase-RTBase->LibNode.lib_NegSize,
	    RTBase->LibNode.lib_NegSize + RTBase->LibNode.lib_PosSize);
#endif
    
    return ret;
}

/****************************************************************************************/

SAVEDS ASM int RTFuncs_Null(REGPARAM(a6, struct ReqToolsBase *, RTBase))
{
    return 0;
}


/****************************************************************************************/

SAVEDS ASM struct ReqToolsPrefs *RTFuncs_LockPrefs(REGPARAM(a6, struct ReqToolsBase *, ReqToolsBase))
{
    ObtainSemaphore(&ReqToolsBase->ReqToolsPrefs.PrefsSemaphore);

    return &ReqToolsBase->ReqToolsPrefs;
}

/****************************************************************************************/

SAVEDS ASM void RTFuncs_UnlockPrefs(REGPARAM(a6, struct ReqToolsBase *, ReqToolsBase))
{
    ReleaseSemaphore(&ReqToolsBase->ReqToolsPrefs.PrefsSemaphore);
}

/****************************************************************************************/

SAVEDS ASM ULONG RTFuncs_rtReqHandlerA(REGPARAM(a1, struct rtHandlerInfo *, handlerinfo),
    	    	    	    	       REGPARAM(d0, ULONG, sigs),
				       REGPARAM(a0, struct TagItem *, taglist))
{
    return  ((ULONG (*)(REGPARAM(a1, struct rtHandlerInfo *,),
    	    	    	REGPARAM(d0, ULONG,),
			REGPARAM(a0, struct TagItem *,)))handlerinfo->private1)(handlerinfo, sigs, taglist);
}

/****************************************************************************************/

SAVEDS ASM void RTFuncs_rtSetWaitPointer(REGPARAM(a0, struct Window *, window))
{
    struct TagItem tags[] = { { WA_BusyPointer, TRUE },
			      { TAG_DONE, 0 } };

    SetWindowPointerA(window, (struct TagItem *)&tags);
}

/****************************************************************************************/

SAVEDS ASM APTR RTFuncs_rtLockWindow(REGPARAM(a0, struct Window *, window))
{
    struct rtWindowLock *winLock;

    /* Is this window already locked? */
    if(window->FirstRequest != NULL)
    {
	struct rtWindowLock *wLock = (struct rtWindowLock *)window->FirstRequest;

	while(wLock != NULL)
	{
	    if(wLock->rtwl_Magic ==  ('r' << 24 | 't' << 16 | 'L' << 8 | 'W'))
	    {
		if(wLock->rtwl_RequesterPtr == wLock)
		{
		    /* Window was already locked */
		    wLock->rtwl_LockCount++;

		    return wLock;
		}
	    }

	    wLock = (struct rtWindowLock *)wLock->rtwl_Requester.OlderRequest;
	}
    }
    winLock = (struct rtWindowLock *)AllocVec(sizeof(struct rtWindowLock),
					      MEMF_CLEAR);
    
    /* No memory? */
    if(winLock == NULL)
	return NULL;

    winLock->rtwl_Magic = 'r' << 24 | 't' << 16 | 'L' << 8 | 'W';
    winLock->rtwl_RequesterPtr = winLock;
    
    winLock->rtwl_MinHeight = window->MinHeight;
    winLock->rtwl_MaxHeight = window->MaxHeight;
    winLock->rtwl_MinWidth  = window->MinWidth;
    winLock->rtwl_MaxWidth  = window->MaxWidth;

    WindowLimits(window, window->Width, window->Height,
		 window->Width, window->Height);
    
    InitRequester((struct Requester *)winLock);
    winLock->rtwl_ReqInstalled = Request((struct Requester *)winLock, window);
    
    winLock->rtwl_Pointer = window->Pointer;
    winLock->rtwl_PtrHeight = window->PtrHeight;
    winLock->rtwl_PtrWidth = window->PtrWidth;
    
    rtSetWaitPointer(window);
    
    return (APTR)winLock;
}

/****************************************************************************************/

SAVEDS ASM VOID RTFuncs_rtUnlockWindow(REGPARAM(a0, struct Window *, window),
    	    	    	    	       REGPARAM(a1, APTR, windowlock))
{
    
    struct rtWindowLock *wLock = (struct rtWindowLock *)windowlock;

    if(wLock == NULL)
	return;

    if(wLock->rtwl_LockCount != 0)
    {
	wLock->rtwl_LockCount--;
    }
    else
    {
	struct TagItem tags[] = { { WA_Pointer, (IPTR)wLock->rtwl_Pointer },
				  { TAG_DONE  , 0 } };

	SetWindowPointerA(window, tags);

    	if (wLock->rtwl_ReqInstalled)
	    EndRequest((struct Requester *)wLock, window);
	
	WindowLimits(window, wLock->rtwl_MinWidth, wLock->rtwl_MinHeight,
		     wLock->rtwl_MaxWidth, wLock->rtwl_MaxHeight);

	FreeVec(wLock);
    }
}

/****************************************************************************************/

SAVEDS ASM void RTFuncs_rtSpread(REGPARAM(a0, ULONG *, posarray),
    	    	    		 REGPARAM(a1, ULONG *, sizearray),
				 REGPARAM(d0, ULONG, totalsize),
				 REGPARAM(d1, ULONG, min),
				 REGPARAM(d2, ULONG, max),
				 REGPARAM(d3, ULONG, num))
{
    ULONG gadpos = min << 16;
    ULONG gadgap;
    UWORD i;

    gadgap = ((max - min - totalsize) << 16) / (num - 1); 

    posarray[0] = min;

    for(i = 1; i < num - 1; i++)
    {
	gadpos += (sizearray[i - 1] << 16) + gadgap;
	posarray[i] = gadpos >> 16;
    }

    posarray[num - 1] = max - sizearray[i];

}

/****************************************************************************************/

SAVEDS ASM void RTFuncs_ScreenToFrontSafely(REGPARAM(a0, struct Screen *, screen))
{
#ifndef USE_FORBID
    ULONG ilock;
#endif
    struct Screen *scr;

    /* Bugfixes: 1. Lock *before* peeking IntuitionBase->FirstScreen
                 2. Favor LockIBase() over Forbid() */

#ifdef USE_FORBID
    Forbid();
#else
    ilock = LockIBase(0);
#endif

    scr = IntuitionBase->FirstScreen;

    while(scr != NULL)
    {
	if(scr == screen)
	{
#ifdef USE_FORBID
	    ScreenToFront(screen);
	    break;
#else
	    /* Forbid before UnlockIBase() to avoid screen from disappearing */
	    Forbid();

	    /* UnlockIBase() basically does ReleaseSemaphore() and that never
	       Wait(), and thus cannot break Forbid(). */
	    UnlockIBase(ilock);

	    /* Actually this will break the Forbid() if it need to Wait() for
	       semaphore, so this function isn't 100% bulletproof anyway... */
	    ScreenToFront(screen);

	    Permit();

	    /* Note: return not break!
	    */
	    return;
#endif
	}
	
	scr = scr->NextScreen;
    }

#ifdef USE_FORBID
    Permit();
#else
    UnlockIBase(ilock);
#endif
}

/****************************************************************************************/

SAVEDS ASM void RTFuncs_rtSetReqPosition(REGPARAM(d0, ULONG, reqpos),
    	    	    	    	    	REGPARAM(a0, struct NewWindow *, nw),
					REGPARAM(a1, struct Screen *, scr),
					REGPARAM(a2, struct Window *, win))
{
#warning Taken from rtfuncs.asm where the C version was in comments. Might be out of date

    int mx = 0, my = 0, val, leftedge, topedge;
    ULONG scrwidth, scrheight;
    int width, height, left, top;

    rtGetVScreenSize (scr, &scrwidth, &scrheight);

    leftedge = -scr->LeftEdge;    
    if (leftedge < 0) leftedge = 0;
    
    topedge = -scr->TopEdge;
    if (topedge < 0) topedge = 0;

    left = leftedge; top = topedge;
    width = scrwidth; height = scrheight;
    
    switch (reqpos)
    {
	case REQPOS_DEFAULT:
	    nw->LeftEdge = 25;
	    nw->TopEdge = 18;
	    goto topleftscr;
	    
	case REQPOS_POINTER:
	    mx = scr->MouseX; my = scr->MouseY;
	    break;
	    
	case REQPOS_CENTERWIN:
	    if (win)
	    {
		left = win->LeftEdge; top = win->TopEdge;
		width = win->Width; height = win->Height;
	    }
	    
	case REQPOS_CENTERSCR:
	    mx = (width - nw->Width) / 2 + left;
	    my = (height - nw->Height) / 2 + top;
	    break;
	    
	case REQPOS_TOPLEFTWIN:
	    if (win)
	    {
		left = win->LeftEdge;
		top = win->TopEdge;
	    }
	    
	case REQPOS_TOPLEFTSCR:
topleftscr:
	    mx = left; my = top;
	    break;
	    
    } /* switch (reqpos) */

    /* keep window completely visible */
    mx += nw->LeftEdge; my += nw->TopEdge;
    val = leftedge + scrwidth - nw->Width;
    
    if (mx < leftedge) mx = leftedge;
    else if (mx > val) mx = val;
    
    val = topedge + scrheight - nw->Height;
    
    if (my < topedge) my = topedge;
    else if (my > val) my = val;

    nw->LeftEdge = mx; nw->TopEdge = my;
}


/****************************************************************************************/

/* This one is from closewindowsafely.asm */

SAVEDS ASM void RTFuncs_CloseWindowSafely(REGPARAM(a0, struct Window *, window))
{
    struct IntuiMessage *msg;
    struct Node     	*succ;

    Forbid();

    if(window->UserPort != NULL)
    {
    	msg = (struct IntuiMessage *)window->UserPort->mp_MsgList.lh_Head;
	
	while((succ = msg->ExecMessage.mn_Node.ln_Succ))
	{
	    if(msg->IDCMPWindow == window)
	    {
		Remove((struct Node *)msg);
		ReplyMsg((struct Message *)msg);
	    }
	    
	    msg = (struct IntuiMessage *)succ;
	}
    }

    window->UserPort = NULL;

    ModifyIDCMP(window, 0);

    Permit();

    CloseWindow(window);
}

/****************************************************************************************/
/****************************************************************************************/

