/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Basic helpfuncs for Asl.
    Lang: english
*/

#undef	AROS_ALMOST_COMPATIBLE
#define AROS_ALMOST_COMPATIBLE

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/dos.h>

#include <exec/lists.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <libraries/iffparse.h>

#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

/*****************************************************************************************/

STATIC BOOL GetRequesterFont(struct LayoutData *, struct AslBase_intern *);

STATIC struct FontPrefs *GetFontPrefs(struct AslBase_intern *);
STATIC VOID FreeFontPrefs(struct FontPrefs *, struct AslBase_intern *);

/*****************************************************************************************/

/* Finds the internal requester data for a requester */

struct ReqNode *FindReqNode(APTR req, struct AslBase_intern *AslBase)
{
    struct ReqNode *reqnode, *foundreqnode = NULL;

    ObtainSemaphoreShared( &(AslBase->ReqListSem) );

    ForeachNode( &(AslBase->ReqList), reqnode)
    {
	if (reqnode->rn_Req == req)
	{
	    foundreqnode = reqnode;
	    break;
	}
    }

    ReleaseSemaphore( &(AslBase->ReqListSem) );

    return (foundreqnode);
}

/*****************************************************************************************/

VOID ParseCommonTags
(
    struct IntReq		*intreq,
    struct TagItem		*taglist,
    struct AslBase_intern	*AslBase
)
{
    struct TagItem *tag, *tstate;

    tstate = taglist;

    while ((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
    {
    	IPTR tidata = tag->ti_Data;
	
	/* The tags that are put "in a row" are defined as the same value,
		and therefor we only use one of them, but the effect is for all of them
	*/
	
	switch (tag->ti_Tag)
	{
	    case ASLFR_Window:
/*	    case ASLFO_Window:
	    case ASLSM_Window:
	    case ASL_Window: */  /* Obsolete */
		intreq->ir_Window = (struct Window *)tidata;
		break;

	    case ASLFR_Screen:
/*	    case ASLFO_Screen:
	    case ASLSM_Screen: */
		intreq->ir_Screen = (struct Screen *)tidata;
		break;

	    case ASLFR_PubScreenName:
/*	    case ASLFO_PubScreenName:
	    case ASLSM_PubScreenName: */
		if (tidata)
		    intreq->ir_PubScreenName = (STRPTR)tidata;
		break;

	    case ASLFR_PrivateIDCMP:
/*	    case ASLFO_PrivateIDCMP:
	    case ASLSM_PrivateIDCMP: */
		if (tidata)
		    intreq->ir_Flags |= IF_PRIVATEIDCMP;
		else
		    intreq->ir_Flags &= ~IF_PRIVATEIDCMP;
		break;


	    case ASLFR_IntuiMsgFunc:
/*	    case ASLFO_IntuiMsgFunc:
	    case ASLSM_IntuiMsgFunc: */
		intreq->ir_IntuiMsgFunc = (struct Hook *)tidata;
		break;

	    case ASLFR_SleepWindow:
/*	    case ASLFO_SleepWindow:
	    case ASLSM_SleepWindow: */
		if (tidata)
		    intreq->ir_Flags |= IF_SLEEPWINDOW;
		else
		    intreq->ir_Flags &= ~IF_SLEEPWINDOW;
		break;

	    case ASLFR_PopToFront:
/*	    case ASLFO_PopToFront:
	    case ASLSM_PopToFront: */
	    	if (tidata)
		    intreq->ir_Flags |= IF_POPTOFRONT;
		else
		    intreq->ir_Flags &= ~IF_POPTOFRONT;
		break;

	    case ASLFR_TextAttr:
/*	    case ASLFO_TextAttr:
	    case ASLSM_TextAttr: */
		intreq->ir_TextAttr = (struct TextAttr *)tidata;
		break;
	    
	    case ASLFR_Locale:
/*	    case ASLFO_Locale:
	    case ASLSM_Locale: */
		intreq->ir_Locale = (struct Locale *)tidata;
		break;

	    case ASLFR_TitleText:
/*	    case ASLFO_TitleText:
	    case ASLSM_TitleText:
	    case ASL_Hail: */ /* Obsolete */
		if (tidata)
		    intreq->ir_TitleText = (STRPTR)tidata;
		break;


	    case ASLFR_PositiveText:
/*	    case ASLFO_PositiveText:
	    case ASLSM_PositiveText:
	    case ASL_OKText: */ /* Obsolete */
		if (tidata)
		{
		    intreq->ir_PositiveText = (STRPTR)tidata;
		    intreq->ir_Flags |= IF_USER_POSTEXT;
		}
		break;

	    case ASLFR_NegativeText:
/*	    case ASLFO_NegativeText:
	    case ASLSM_NegativeText:
	    case ASL_CancelText: */ /* Obsolete */
	    	if (tidata)
		{
		    intreq->ir_NegativeText = (STRPTR)tidata;
		    intreq->ir_Flags |= IF_USER_NEGTEXT;
		}
		break;

	    case ASLFR_InitialLeftEdge:
/*	    case ASLFO_InitialLeftEdge:
	    case ASLSM_InitialLeftEdge:
	    case ASL_LeftEdge: */ /* Obsolete */
		intreq->ir_LeftEdge = (UWORD)tidata;
		break;

	    case ASLFR_InitialTopEdge:
/*	    case ASLFO_InitialTopEdge:
	    case ASLSM_InitialTopEdge:
	    case ASL_TopEdge: */ /* Obsolete */
		intreq->ir_TopEdge = (UWORD)tidata;
		break;

	    case ASLFR_InitialWidth:
/*	    case ASLFO_InitialWidth:
	    case ASLSM_InitialWidth:
	    case ASL_Width: */ /* Obsolete */
		intreq->ir_Width = (UWORD)tidata;
		break;

	    case ASLFR_InitialHeight:
/*	    case ASLFO_InitialHeight:
	    case ASLSM_InitialHeight:
	    case ASL_Height: */ /* Obsolete */
		intreq->ir_Height = (UWORD)tidata;
		break;

	    default:
		break;
		
	} /* switch (tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL) */
    return;
}

/*****************************************************************************************/

VOID FreeCommon(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    if (ld)
    {

	if (ld->ld_Menu)
	{
	    if (ld->ld_Window)
		ClearMenuStrip(ld->ld_Window);

	    FreeMenus(ld->ld_Menu);
	}

	if (ld->ld_Window)
	    CloseWindow(ld->ld_Window);

	D(bug("Window freed\n"));

	if (ld->ld_VisualInfo) FreeVisualInfo(ld->ld_VisualInfo);
	if (ld->ld_Dri) FreeScreenDrawInfo(ld->ld_Screen, ld->ld_Dri);
	
	if (ld->ld_ScreenLocked)
	    UnlockPubScreen(NULL, ld->ld_Screen);

	if (ld->ld_UserData)
	    FreeVec(ld->ld_UserData);

	if (ld->ld_Font)
	    CloseFont(ld->ld_Font);

	if (ld->ld_TextAttr.ta_Name)
	    FreeVec(ld->ld_TextAttr.ta_Name);

	FreeMem(ld, sizeof (struct LayoutData));
    }

    return;
}

/*****************************************************************************************/

struct LayoutData *AllocCommon
(
    ULONG			udatasize,
    struct IntReq		*intreq,
    APTR			requester,
    struct AslBase_intern	*AslBase
)
{
    struct Screen 	*screen = NULL;
    struct LayoutData 	*ld;


    ld = AllocMem(sizeof (struct LayoutData), MEMF_ANY|MEMF_CLEAR);
    if (!ld)
	goto failure;

    /* Save the internal and public requester struct, so that the
      requester type specific hook may find them */
    ld->ld_IntReq	= intreq;
    ld->ld_Req		= requester;

    /* We need to lock the screen we should open on to be sure it
    doesn't go away
    */

    /* Find screen on which to open window */
    
    screen = intreq->ir_Screen;
    if (!screen && intreq->ir_PubScreenName)
    {
	if ((screen = LockPubScreen(intreq->ir_PubScreenName)))
	{
	    ld->ld_ScreenLocked = TRUE;
	}
    }    
    if (!screen && intreq->ir_Window)
    {
        screen = intreq->ir_Window->WScreen;
    }
    if (!screen && !intreq->ir_PubScreenName)
    {
        if ((screen = LockPubScreen(NULL)))
	{
	    ld->ld_ScreenLocked = TRUE;
	}
    }

    if (!screen) goto failure;

    ld->ld_Screen = screen;

    if (!(ld->ld_Dri = GetScreenDrawInfo(screen))) goto failure;
    
    if (!(ld->ld_VisualInfo = GetVisualInfoA(screen, NULL))) goto failure;
    
    ld->ld_WBorLeft  = screen->WBorLeft;
    ld->ld_WBorTop   = screen->WBorTop + screen->Font->ta_YSize + 1;
    ld->ld_WBorRight = screen->WBorRight;
    ld->ld_WBorBottom = 16;
    
    {
        struct TagItem sysi_tags[] =
	{
	    {SYSIA_DrawInfo	, (IPTR)ld->ld_Dri	},
	    {SYSIA_Which	, SIZEIMAGE		},
	    {TAG_DONE					}
	};
	
        Object *im;
	 
	if ((im = NewObjectA(NULL, SYSICLASS, sysi_tags)))
	{
	    IPTR height;
	    
	    if (GetAttr(IA_Height, im, &height))
	    {
	        ld->ld_WBorBottom = height;
	    }
	    DisposeObject(im);
	}
	
    }
        
    if(GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH) > 8) ld->ld_TrueColor = TRUE;
    
    if (!(ld->ld_UserData = AllocVec(udatasize, MEMF_ANY|MEMF_CLEAR)))
	goto failure;

    if (!GetRequesterFont(ld, ASLB(AslBase)))
	goto failure;

    InitRastPort(&(ld->ld_DummyRP));

    SetFont( &(ld->ld_DummyRP), ld->ld_Font );

    return (ld);

failure:
    FreeCommon(ld, ASLB(AslBase));

    return (NULL);
}


/*****************************************************************************************/

#define SKIPLONG(ptr, num) ptr += sizeof (LONG) * num

#define CONVBYTE(ptr, dest) dest = *ptr ++

#define SKIPBYTE(ptr) ptr ++;

#define CONVWORD(ptr, dest) dest = ptr[0] << 8 | ptr[1]; \
		ptr += sizeof (WORD);

/*****************************************************************************************/

STATIC struct FontPrefs *GetFontPrefs(struct AslBase_intern *AslBase)
{
    struct IFFHandle 		*iff;
    struct Library 		*IFFParseBase;

    struct StoredProperty 	*sp;

    struct FontPrefs 		*fp = NULL;

    IFFParseBase = OpenLibrary("iffparse.library", 0);
    if (IFFParseBase)
    {
	iff = AllocIFF();
	if (iff)
	{

	    iff->iff_Stream = (IPTR)Open("ENV:Sys/font.prefs", MODE_OLDFILE);
	    if (iff->iff_Stream)
	    {
		InitIFFasDOS(iff);

		if (OpenIFF(iff, IFFF_READ) == 0)
		{
		    PropChunk(iff, ID_PREF, ID_FONT);


		    if (ParseIFF(iff, IFFPARSE_SCAN) != 0)
		    {

			sp = FindProp(iff, ID_PREF, ID_FONT);
			if (sp)
			{

			    fp = AllocMem(sizeof (struct FontPrefs), MEMF_ANY);
			    if (fp)
			    {
				UBYTE *ptr;
				STRPTR fontname;


				/* Set ptr to start of struct FontPrefs */
				ptr = sp->sp_Data;

				/* Skip 4 first reserved longs */
				SKIPLONG(ptr, 4);
				CONVBYTE(ptr, fp->fp_FrontPen);
				CONVBYTE(ptr, fp->fp_BackPen);
				CONVBYTE(ptr, fp->fp_DrawMode);

				CONVWORD(ptr, fp->fp_TextAttr.ta_YSize);
				CONVBYTE(ptr, fp->fp_TextAttr.ta_Style);
				CONVBYTE(ptr, fp->fp_TextAttr.ta_Flags);


				fontname =  AllocVec( strlen(ptr) + 1, MEMF_ANY);
				if (fontname)
				{
				    strcpy(fontname, ptr);

				    fp->fp_TextAttr.ta_Name = fontname;
				}
				else
				{
				    FreeMem(fp, sizeof (struct FontPrefs));
				    fp = NULL;
				}

			    } /* if (tattr) */

			} /* if (sp) */

		    } /* if (ParseIFF(iff, IFFPARSE_SCAN) != 0) */
		    CloseIFF(iff);

		} /* if (OpenIFF(iff)) */
		Close( (BPTR)iff->iff_Stream );

	    } /* if (iff->iff_Stream) */
	    FreeIFF(iff);

	} /* if (iff) */
	CloseLibrary(IFFParseBase);

    } /* if (IFFParseBase) */

    return (fp);

} /* GetFontPrefs() */

/*****************************************************************************************/

STATIC VOID FreeFontPrefs(struct FontPrefs *fp, struct AslBase_intern *AslBase)
{

    FreeVec(fp->fp_TextAttr.ta_Name);
    FreeMem(fp, sizeof (struct FontPrefs));

    return;
}


/*****************************************************************************************/

BOOL GetRequesterFont(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct TextFont 	*font = NULL;

    struct TextAttr 	*usedattr;

    BOOL 		success = FALSE;

    struct TextAttr 	topaz8 = {"topaz.font", 8, 0, 0 };

    /* Default to satisfy GCC */
    usedattr = &topaz8;

    /*
	Open the font we should use in the GUI.
	First look for a user supplied TextAttr.
	If not present, try to get the font preferences from ENV:Sys/font.prefs
	If this fails, we fall back to topaz 8
    */

    /* Is there a user supplied font */
    usedattr = ld->ld_IntReq->ir_TextAttr;

    if (usedattr)
    {
	font = OpenDiskFont (usedattr);
    }

    /* If no font has been opened yet, try the preferences one */
    if (!font)
    {
	struct FontPrefs	*fprefs;

	fprefs = GetFontPrefs(ASLB(AslBase));
	if (fprefs)
	{
	    D(bug("Fontprefs found\n"));

	    D(bug("Name: %s, YSize :%d", fprefs->fp_TextAttr.ta_Name, fprefs->fp_TextAttr.ta_YSize));

	    usedattr = &(fprefs->fp_TextAttr);
	    font = OpenDiskFont(usedattr);

	    if (!font)
	    {
		FreeFontPrefs(fprefs, ASLB(AslBase));
	    }
	}
    }

    /* Yet no font, try topaz 8 */

    if (!font)
    {
	usedattr = &topaz8;

	/* Here we should really use OpenDiskFont, but
	 * since AROS can't render diskfonts yet, we must use OpenFont()
	 */

	font = OpenFont(usedattr);
    }

    if (font)
    {
	STRPTR fontname;
	/* We have to store the used textattr for later */

	fontname = AllocVec (strlen (usedattr->ta_Name) + 1, MEMF_ANY);

	if (fontname)
	{
	    strcpy (fontname, usedattr->ta_Name);

	    ld->ld_TextAttr.ta_Name  = fontname;
	    ld->ld_TextAttr.ta_YSize = usedattr->ta_YSize;
	    ld->ld_TextAttr.ta_Style = usedattr->ta_Style;
	    ld->ld_TextAttr.ta_Flags = usedattr->ta_Flags;

	    ld->ld_Font = font;

	    success = TRUE;
	}
    }
    return (success);
}

/*****************************************************************************************/

BOOL HandleEvents(struct LayoutData *ld, struct AslReqInfo *reqinfo, struct AslBase_intern *AslBase)
{
    struct IntReq	*intreq = ld->ld_IntReq;
    APTR		req = ld->ld_Req;
    struct IntuiMessage *imsg;
    struct MsgPort	*port;
    BOOL 		success = TRUE;
    BOOL 		terminated = FALSE;

    EnterFunc(bug("HandleEvents(ld=%p, reqinfo=%p)\n", ld, reqinfo));
    port = ld->ld_Window->UserPort;

    while (!terminated)
    {
	if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
	{
	    if ((imsg->IDCMPWindow == ld->ld_Window) ||
	        (imsg->IDCMPWindow == ld->ld_Window2))
	    {
		switch (imsg->Class)
		{
		    case IDCMP_MOUSEMOVE:
			break;

		    case IDCMP_NEWSIZE:
			ld->ld_Command = LDCMD_LAYOUT;
			CallHookPkt(&(reqinfo->GadgetryHook), ld, ASLB(AslBase));
			break;

		    case IDCMP_REFRESHWINDOW:
			BeginRefresh(imsg->IDCMPWindow);
			EndRefresh(imsg->IDCMPWindow, TRUE);
			break;

		    default:
			/* Call the requester specific hook to handle events */
			ld->ld_Command = LDCMD_HANDLEEVENTS;
			ld->ld_Event = imsg;

			success = CallHookPkt( &(reqinfo->GadgetryHook), ld, ASLB(AslBase));
			if (success == LDRET_FINISHED)
			{
			    success    = TRUE;
			    terminated = TRUE;
			}
			if (!success)
			{
			    success = FALSE;
			    terminated = TRUE;
			}
			break;


		} /* switch (imsg->Class */
	    
	    } /* if (imsg->IDCMPWindow is ld->ld_Window or ld->ld_Window2) */
	    else if (intreq->ir_IntuiMsgFunc)
	    {
		CallHookPkt(intreq->ir_IntuiMsgFunc, req, imsg);
	    }
	    ReplyMsg((struct Message *)imsg);

	} /* if ((imsg = GetMsg(port)) != NULL) */
	else
	{
	    Wait(1L << port->mp_SigBit);
	}
	
    } /* while (!terminated) */
    
    ReturnBool ("HandleEvents", success);
    
} /* HandleEvents() */

/*****************************************************************************************/

UWORD BiggestTextLength(STRPTR          *strarray,
			UWORD		numstrings,
			struct RastPort *rp,
			struct AslBase_intern * AslBase)
{

    UWORD i, w = 0, new_w;

    for (i = 0; i < numstrings; i ++)
    {
	new_w = TextLength(rp, strarray[i], strlen(strarray[i]));
	if (new_w > w)
	{
	    w = new_w;
	}
    }
    return (w);
}

/*****************************************************************************************/

/* Strip special info from the requester structure */

VOID StripRequester(APTR req, UWORD reqtype, struct AslBase_intern *AslBase)
{
    switch (reqtype)
    {
	case ASL_FileRequest:

	    #undef GetFR
	    #define GetFR(r) ((struct FileRequester *)r)

	    FreeVecPooled(GetFR(req)->fr_Drawer, AslBase);
	    GetFR(req)->fr_Drawer = NULL;

	    FreeVecPooled(GetFR(req)->fr_File, AslBase);
	    GetFR(req)->fr_File = NULL;

	    FreeVecPooled(GetFR(req)->fr_Pattern, AslBase);
	    GetFR(req)->fr_Pattern = NULL;
	    
	    if (GetFR(req)->fr_ArgList)
	    {
		struct WBArg *wbarg;
		BPTR lock = GetFR(req)->fr_ArgList->wa_Lock;
		
		if (lock) UnLock(lock);
		
		for (wbarg = GetFR(req)->fr_ArgList; GetFR(req)->fr_NumArgs --; wbarg ++)
		{
		    FreeVecPooled(wbarg->wa_Name, AslBase);
		}
		
		FreeVecPooled(GetFR(req)->fr_ArgList, AslBase);
		GetFR(req)->fr_ArgList = NULL;
	    }

	    break;

	case ASL_FontRequest:
	    break;

	case ASL_ScreenModeRequest:
	    break;

    }
    return;
}

/*****************************************************************************************/

WORD CountNodes(struct List *list, WORD flag)
{
    struct Node *node;
    WORD 	result = 0;
    
    ForeachNode(list, node)
    {
        if ((node->ln_Pri & flag) == flag) result++;
    }
    
    return result;
}

/*****************************************************************************************/

struct Node *FindListNode(struct List *list, WORD which)
{
    struct Node *node = NULL;
    
    if (which >= 0)
    {
	for(node = list->lh_Head; node->ln_Succ && which; node = node->ln_Succ, which--)
	{
	}
	if (!node->ln_Succ) node = NULL;
    }
    
    return node;
}

/*****************************************************************************************/

void SortInNode(APTR req, struct List *list, struct Node *node,
		WORD (*compare)(APTR req, APTR node1, APTR node2, struct AslBase_intern *AslBase),
		struct AslBase_intern *AslBase)
{
    struct Node *prevnode = NULL;
    struct Node *checknode;
    
    ForeachNode(list, checknode)
    {
        if (compare(req, node, checknode, AslBase) < 0) break;

	prevnode = checknode;
    }
    
    Insert(list, node, prevnode);
}

/*****************************************************************************************/

APTR AllocVecPooled(APTR pool, IPTR size, struct AslBase_intern *AslBase)
{
    IPTR *mem;
    
    size += sizeof(APTR) * 2;

    if ((mem = AllocPooled(pool, size)))
    {
        *mem++ = (IPTR)pool;
	*mem++ = size;
    }
    
    return mem;
}

/*****************************************************************************************/

void FreeVecPooled(APTR mem, struct AslBase_intern *AslBase)
{
    IPTR *imem = (IPTR *)mem;
    
    if (mem)
    {
        IPTR size = *--imem;
        APTR pool = (APTR)*--imem;
	
	FreePooled(pool, imem, size);
    }
}

/*****************************************************************************************/

char *PooledCloneString(const char *name1, const char *name2, APTR pool,
			struct AslBase_intern *AslBase)
{
    char *clone;
    WORD len1 = strlen(name1) + 1;
    WORD len2 = name2 ? strlen(name2) : 0;

    if ((clone = AllocPooled(pool, len1 + len2)))
    {
        CopyMem(name1, clone, len1);
	if (name2) strcat(clone, name2);
    }
    
    return clone;
}

/*****************************************************************************************/

char *VecCloneString(const char *name1, const char *name2, struct AslBase_intern *AslBase)
{
    char *clone;
    WORD len1 = strlen(name1) + 1;
    WORD len2 = name2 ? strlen(name2) : 0;

    if ((clone = AllocVec(len1 + len2, MEMF_PUBLIC)))
    {
        CopyMem(name1, clone, len1);
	if (name2) strcat(clone, name2);
    }
    
    return clone;
}

/*****************************************************************************************/

char *VecPooledCloneString(const char *name1, const char *name2, APTR pool, struct AslBase_intern *AslBase)
{
    char *clone;
    WORD len1 = strlen(name1) + 1;
    WORD len2 = name2 ? strlen(name2) : 0;

    if ((clone = AllocVecPooled(pool, len1 + len2, AslBase)))
    {
        CopyMem(name1, clone, len1);
	if (name2) strcat(clone, name2);
    }
    
    return clone;
}

/*****************************************************************************************/

AROS_UFH2 (void, puttostr,
	AROS_UFHA(UBYTE, chr, D0),
	AROS_UFHA(STRPTR *,strPtrPtr,A3)
)
{
    AROS_LIBFUNC_INIT
    *(*strPtrPtr)= chr;
    (*strPtrPtr) ++;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************************/

char *PooledIntegerToString(IPTR value, APTR pool, struct AslBase_intern *AslBase)
{
    char buffer[30];
    char *str = buffer;
    char *clone;
    WORD len;
    
    /* Create the text */

    RawDoFmt("%ld", &value, (VOID_FUNC)puttostr, &str);

    len = strlen(buffer) + 1;
    
    if ((clone = AllocPooled(pool, len)))
    {
        CopyMem(buffer, clone, len);
    }
    
    return clone;
}

/*****************************************************************************************/

void CloseWindowSafely(struct Window *window, struct AslBase_intern *AslBase)
{
    struct IntuiMessage *msg;

    Forbid();

    if(window->UserPort != NULL)
    {
	while((msg = (struct IntuiMessage *)GetMsg(window->UserPort)) != NULL)
	{
	    if(msg->IDCMPWindow == window)
	    {
		Remove((struct Node *)msg);
		ReplyMsg((struct Message *)msg);
	    }
	}
    }

    window->UserPort = NULL;

    ModifyIDCMP(window, 0);

    Permit();

    CloseWindow(window);
}
    
/*****************************************************************************************/

AROS_UFH3(ULONG, StringEditFunc,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(struct SGWork *,		sgw,		A2),
    AROS_UFHA(ULONG *, 			command,	A1))
{
    ULONG retcode = 0;
        
    switch(*command)
    {
        case SGH_KEY:
	    retcode = 1;
    	    switch(sgw->IEvent->ie_Code)
	    {
	        case CURSORUP:
		    sgw->EditOp  = EO_SPECIAL;
		    sgw->Code    = STRINGCODE_CURSORUP;
		    sgw->Actions = SGA_END;
		    break;
		    
		case CURSORDOWN:
		    sgw->EditOp  = EO_SPECIAL;
		    sgw->Code    = STRINGCODE_CURSORDOWN;
		    sgw->Actions = SGA_END;
		    break;
		    
		case 0x45: /* escape */
		    sgw->EditOp  = EO_SPECIAL;
		    sgw->Code    = 27;
		    sgw->Actions = SGA_END | SGA_REUSE;
		    break;
	    }
    	    break;
    
    }
    
    return retcode;
}

/*****************************************************************************************/
