SAVEDS ASM struct ReqToolsBase *RTFuncs_Init(REGPARAM(d0, struct ReqToolsBase *, RTBase),
    	    	    	    	    	     REGPARAM(a0, BPTR, segList));					      
SAVEDS ASM struct ReqToolsBase *RTFuncs_Open(REGPARAM(a6, struct ReqToolsBase *, RTBase),
    	    	    	    	    	     REGPARAM(d0, ULONG, ver));
SAVEDS ASM BPTR RTFuncs_Close(REGPARAM(a6, struct ReqToolsBase *, RTBase));
SAVEDS ASM BPTR RTFuncs_Expunge(REGPARAM(a6, struct ReqToolsBase *, RTBase));
SAVEDS ASM int RTFuncs_Null(REGPARAM(a6, struct ReqToolsBase *, RTBase));

SAVEDS ASM struct ReqToolsPrefs *RTFuncs_LockPrefs(REGPARAM(a6, struct ReqToolsBase *, ReqToolsBase));
SAVEDS ASM void RTFuncs_UnlockPrefs(REGPARAM(a6, struct ReqToolsBase *, ReqToolsBase));

SAVEDS ASM IPTR RTFuncs_rtReqHandlerA(REGPARAM(a1, struct rtHandlerInfo *, handlerinfo),
    	    	    	    	       REGPARAM(d0, ULONG, sigs),
				       REGPARAM(a0, struct TagItem *, taglist));

SAVEDS ASM void RTFuncs_rtSetWaitPointer(REGPARAM(a0, struct Window *, window));

SAVEDS ASM APTR RTFuncs_rtLockWindow(REGPARAM(a0, struct Window *, window));
SAVEDS ASM VOID RTFuncs_rtUnlockWindow(REGPARAM(a0, struct Window *, window),
    	    	    	    	       REGPARAM(a1, APTR, windowlock));
SAVEDS ASM void RTFuncs_rtSpread(REGPARAM(a0, ULONG *, posarray),
    	    	    		 REGPARAM(a1, ULONG *, sizearray),
				 REGPARAM(d0, ULONG, totalsize),
				 REGPARAM(d1, ULONG, min),
				 REGPARAM(d2, ULONG, max),
				 REGPARAM(d3, ULONG, num));
SAVEDS ASM void RTFuncs_ScreenToFrontSafely(REGPARAM(a0, struct Screen *, screen));
SAVEDS ASM void RTFuncs_rtSetReqPosition(REGPARAM(d0, ULONG, reqpos),
    	    	    	    	    	REGPARAM(a0, struct NewWindow *, nw),
					REGPARAM(a1, struct Screen *, scr),
					REGPARAM(a2, struct Window *, win));
SAVEDS ASM void RTFuncs_CloseWindowSafely(REGPARAM(a0, struct Window *, window)); /* was in closewindowsafely.asm */

