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
#include <proto/dos.h>

#include <exec/lists.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <libraries/iffparse.h>

#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

STATIC BOOL GetRequesterFont(struct LayoutData *, struct AslBase_intern *);

STATIC struct FontPrefs *GetFontPrefs(struct AslBase_intern *);
STATIC VOID FreeFontPrefs(struct FontPrefs *, struct AslBase_intern *);


/****************/
/* FindReqNode	*/
/****************/

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

/********************/
/* ParseCommonTags	*/
/********************/

VOID ParseCommonTags
(
    struct IntReq		*intreq,
    struct TagItem		*taglist,
    struct AslBase_intern	*AslBase
)
{
    struct TagItem *tag, *tstate;

    tstate = taglist;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
	/* The tags that are put "in a row" are defined as the same value,
		and therefor we only use one of them, but the effect is for all of them
	*/

	switch (tag->ti_Tag)
	{
	    case ASLFR_Window:
/*	    case ASLFO_Window:
	    case ASLSM_Window:
	    case ASL_Window: */  /* Obsolete */
		intreq->ir_Window = (struct Window *)tag->ti_Data;
		break;

	    case ASLFR_Screen:
/*	    case ASLFO_Screen:
	    case ASLSM_Screen: */
		intreq->ir_Screen = (struct Screen *)tag->ti_Data;
		break;

	    case ASLFR_PubScreenName:
/*	    case ASLFO_PubScreenName:
	    case ASLSM_PubScreenName: */
		intreq->ir_PubScreenName = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_PrivateIDCMP:
/*	    case ASLFO_PrivateIDCMP:
	    case ASLSM_PrivateIDCMP: */
		if (tag->ti_Data)
		    intreq->ir_Flags |= IF_PRIVATEIDCMP;
		else
		    intreq->ir_Flags &= ~IF_PRIVATEIDCMP;
		break;


	    case ASLFR_IntuiMsgFunc:
/*	    case ASLFO_IntuiMsgFunc:
	    case ASLSM_IntuiMsgFunc: */
		intreq->ir_IntuiMsgFunc = (struct Hook *)tag->ti_Data;
		break;

	    case ASLFR_SleepWindow:
/*	    case ASLFO_SleepWindow:
	    case ASLSM_SleepWindow: */
		if (tag->ti_Data)
		    intreq->ir_Flags |= IF_SLEEPWINDOW;
		else
		    intreq->ir_Flags &= ~IF_SLEEPWINDOW;
		break;

	    case ASLFR_TextAttr:
/*	    case ASLFO_TextAttr:
	    case ASLSM_TextAttr: */
		intreq->ir_TextAttr = (struct TextAttr *)tag->ti_Data;
		break;

	    case ASLFR_Locale:
/*	    case ASLFO_Locale:
	    case ASLSM_Locale: */
		intreq->ir_Locale = (struct Locale *)tag->ti_Data;
		break;

	    case ASLFR_TitleText:
/*	    case ASLFO_TitleText:
	    case ASLSM_TitleText:
	    case ASL_Hail: */ /* Obsolete */
		intreq->ir_TitleText = (STRPTR)tag->ti_Data;
		break;


	    case ASLFR_PositiveText:
/*	    case ASLFO_PositiveText:
	    case ASLSM_PositiveText:
	    case ASL_OKText: */ /* Obsolete */
		intreq->ir_PositiveText = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_NegativeText:
/*	    case ASLFO_NegativeText:
	    case ASLSM_NegativeText:
	    case ASL_CancelText: */ /* Obsolete */
		intreq->ir_NegativeText = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_InitialLeftEdge:
/*	    case ASLFO_InitialLeftEdge:
	    case ASLSM_InitialLeftEdge:
	    case ASL_LeftEdge: */ /* Obsolete */
		intreq->ir_LeftEdge = (UWORD)tag->ti_Data;
		break;

	    case ASLFR_InitialTopEdge:
/*	    case ASLFO_InitialTopEdge:
	    case ASLSM_InitialTopEdge:
	    case ASL_TopEdge: */ /* Obsolete */
		intreq->ir_TopEdge = (UWORD)tag->ti_Data;
		break;

	    case ASLFR_InitialWidth:
/*	    case ASLFO_InitialWidth:
	    case ASLSM_InitialWidth:
	    case ASL_Width: */ /* Obsolete */
		intreq->ir_Width = (UWORD)tag->ti_Data;
		break;

	    case ASLFR_InitialHeight:
/*	    case ASLFO_InitialHeight:
	    case ASLSM_InitialHeight:
	    case ASL_Height: */ /* Obsolete */
		intreq->ir_Height = (UWORD)tag->ti_Data;
		break;

	    default:
		break;
	}
    }
    return;
}
/****************/
/* FreeCommon	*/
/****************/

VOID FreeCommon(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    if (ld)
    {

/* Only needed if we use gadtools menus !!!

	if (ld->ld_Menu)
	{
	    if (ld->ld_Window)
		ClearMenuStrip(ld->ld_Window);

	    FreeMenus(ld->ld_Menu);
	}
*/
	if (ld->ld_Window)
	    CloseWindow(ld->ld_Window);

	D(bug("Window freed\n"));

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

/********************/
/* AllocCommon		*/
/********************/

struct LayoutData *AllocCommon
(
    ULONG			udatasize,
    struct IntReq		*intreq,
    APTR			requester,
    struct AslBase_intern	*AslBase
)
{

    struct Screen *screen = NULL;
    STRPTR screentitle = NULL;

    struct LayoutData *ld;


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

    /* If a window opened us, the screen is allready locked by that window */
    if (intreq->ir_Window)
    {
	screen = intreq->ir_Window->WScreen;
    }
    else if (intreq->ir_Screen)
    {
	screentitle = intreq->ir_Screen->Title;
    }
    else if (intreq->ir_PubScreenName)
    {
	screentitle = intreq->ir_PubScreenName;
    }
    else
    {
	/* Use workbench screen */
	screentitle = NULL;
    }

    /* No screen locked allready ? */
    if (!screen)
    {
	screen = LockPubScreen(screentitle);
	if (!screen)
	    goto failure;
	else
	    ld->ld_ScreenLocked = TRUE;
    }
    /* Remeber the screen we should open on */
    ld->ld_Screen = screen;


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




/****************/
/* GetFontPrefs */
/****************/


#define SKIPLONG(ptr, num) ptr += sizeof (LONG) * num

#define CONVBYTE(ptr, dest) dest = *ptr ++

#define SKIPBYTE(ptr) ptr ++;

#define CONVWORD(ptr, dest) dest = ptr[0] << 8 | ptr[1]; \
		ptr += sizeof (WORD);

STATIC struct FontPrefs *GetFontPrefs(struct AslBase_intern *AslBase)
{
    struct IFFHandle *iff;
    struct Library *IFFParseBase;

    struct StoredProperty *sp;

    struct FontPrefs *fp = NULL;

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

/******************/
/* FreeFontPrefs  */
/******************/

STATIC VOID FreeFontPrefs(struct FontPrefs *fp, struct AslBase_intern *AslBase)
{

    FreeVec(fp->fp_TextAttr.ta_Name);
    FreeMem(fp, sizeof (struct FontPrefs));

    return;
}


/************/
/* GetFont  */
/************/

BOOL GetRequesterFont(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct TextFont *font = NULL;
    struct Library *DiskfontBase;

    struct TextAttr *usedattr;

    BOOL success = FALSE;

    struct TextAttr topaz8 = {"topaz.font", 8, 0, 0 };

    /* Default to satisfy GCC */
    usedattr = &topaz8;

    /*
	Open the font we should use in the GUI.
	First look for a user supplied TextAttr.
	If not present, try to get the font preferences from ENV:Sys/font.prefs
	If this fails, we fall back to topaz 8
    */
    DiskfontBase = OpenLibrary("diskfont.library", 37);

    if (DiskfontBase)
    {
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

	CloseLibrary(DiskfontBase);
    } /* if (DiskfontBase) */

    if (font)
    {
	STRPTR fontname;
	/* We have to store the used textattr for later */

	fontname = AllocVec (strlen (usedattr->ta_Name) + 1, MEMF_ANY);

	if (fontname)
	{
	    strcpy (fontname, usedattr->ta_Name);

	    ld->ld_TextAttr.ta_Name	 = fontname;
	    ld->ld_TextAttr.ta_YSize = usedattr->ta_YSize;
	    ld->ld_TextAttr.ta_Style = usedattr->ta_Style;
	    ld->ld_TextAttr.ta_Flags = usedattr->ta_Flags;

	    ld->ld_Font = font;

	    success = TRUE;
	}
    }
    return (success);
}

/****************/
/* HandleEvents */
/****************/

BOOL HandleEvents(struct LayoutData *ld, struct AslReqInfo *reqinfo, struct AslBase_intern *AslBase)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port;

    BOOL success = TRUE;

    BOOL terminated = FALSE;

    EnterFunc(bug("HandleEvents(ld=%p, reqinfo=%p)\n", ld, reqinfo));
    port = ld->ld_Window->UserPort;

    while (!terminated)
    {
	if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
	{

	    switch (imsg->Class)
	    {
		struct Gadget *glist;
		struct Window *win;

		case IDCMP_CLOSEWINDOW:
		    success = TRUE;
		    terminated = TRUE;
		    break;

		case IDCMP_MOUSEMOVE:
		    break;

		case IDCMP_NEWSIZE:
		    win = ld->ld_Window;
		    SetAPen(win->RPort, 0);
		    RectFill(win->RPort,
		             win->BorderLeft,
		             win->BorderTop,
		             win->Width - win->BorderRight,
		             win->Height- win->BorderTop);
		    
		case IDCMP_REFRESHWINDOW:

		    win = ld->ld_Window;
		    glist =  ld->ld_GList;

		    /* Window has changed its size, we must do
		    some relayout and then refresh */

//		    RemoveGList(win, glist, 2);

		    ld->ld_Command = LDCMD_LAYOUT;

//		    CallHookPkt(&(reqinfo->GadgetryHook), ld, ASLB(AslBase));

//		    AddGList(win, glist, -1,  2, NULL);
		    D(bug("HE: Refreshing gadgets\n"));
		    RefreshGList(glist, win, NULL, -1);

		    D(bug("HE: Gadgets refreshed\n"));

		    BeginRefresh(win);
		    EndRefresh(win, TRUE);

		    /* If the window was sized smaller the window borders will
		    for some reason contain the gadget imagery, so we must
		    refresh them */

		    RefreshWindowFrame(win);
		    D(bug("HE: Window frame refreshed\n"));
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
	    ReplyMsg((struct Message *)imsg);


	} /* if ((imsg = GetMsg(port)) != NULL) */
	else
	{
	    Wait(1L << port->mp_SigBit);
	}
    } /* while (!terminated) */

    ReturnBool ("HandleEvents", success);
} /* HandleEvents() */

/************************
**  BiggestTextLength  **
************************/
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

/*********************
**  StripRequester  **
*********************/

/* Strip special info from the requester structure */
VOID StripRequester(APTR req, UWORD reqtype, struct AslBase_intern *AslBase)
{
    switch (reqtype)
    {
	case ASL_FileRequest:

	    #undef GetFR
	    #define GetFR(r) ((struct FileRequester *)r)


	    if (GetFR(req)->fr_Drawer)
	    {
		FreeVec(GetFR(req)->fr_Drawer);
		GetFR(req)->fr_Drawer = NULL;
	    }

	    if (GetFR(req)->fr_File)
	    {
		FreeVec(GetFR(req)->fr_File);
		GetFR(req)->fr_File = NULL;
	    }
	    if (GetFR(req)->fr_ArgList)
	    {
		struct WBArg *wbarg;

		for (wbarg = GetFR(req)->fr_ArgList; GetFR(req)->fr_NumArgs --; wbarg ++)
		{
		    if (wbarg->wa_Name)
			FreeVec(wbarg->wa_Name);
		}
		FreeVec(GetFR(req)->fr_ArgList);
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
