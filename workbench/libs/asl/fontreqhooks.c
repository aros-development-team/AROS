/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Font requester specific code.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/diskfont.h>
#include <proto/boopsi.h>
#include <intuition/icclass.h>
#include <exec/memory.h>
#include <utility/hooks.h>
#include <gadgets/aroslist.h>
#include <gadgets/aroslistview.h>

#include <stdlib.h> /* qsort() */
#include <stdio.h> /* snprintf() */


#include "asl_intern.h"
#include "fontreqhooks.h"
#include "layout.h"

#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>

STATIC BOOL FOGadInit(struct LayoutData *, struct AslBase_intern *);
STATIC BOOL FOGadLayout(struct LayoutData *, struct AslBase_intern *);
STATIC VOID FOGadCleanup(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG FOHandleEvents(struct LayoutData *, struct AslBase_intern *);

STATIC int AFSortFunc(const struct AvailFonts *, const struct AvailFonts *);
STATIC struct AvailFontsHeader *GetAF(LONG, struct AslBase_intern *);
STATIC BOOL InitFontList(Object *, struct AvailFontsHeader *,  struct AslBase_intern *);
STATIC BOOL GetSizeList(Object *, struct NameEntry *, struct AvailFontsHeader *,
					struct AslBase_intern *);

AROS_UFP3S(VOID, NameDisplayHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(STRPTR *,                 dharray,        A2),
    AROS_UFPA(struct NameEntry *,       ne,             A1)
);
AROS_UFP3S(VOID, SizeDisplayHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(STRPTR *,                 dharray,        A2),
    AROS_UFPA(UWORD *,                  ysize,          A1)
);

AROS_UFP3S(APTR, NameConstructHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(APTR,                     pool,           A2),
    AROS_UFPA(struct NameEntry *,       ne,             A1)
);

AROS_UFP3S(VOID, NameDestructHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(APTR,                     pool,           A2),
    AROS_UFPA(struct NameEntry *,       ne,             A1)
);

#define ID_BUTOK		1
#define ID_BUTCANCEL		2

#define ID_NAMELISTVIEW 	3
#define ID_SIZELISTVIEW 	4

#undef NUMBUTS
#define NUMBUTS 2L

#define DISPHOOKBUFSIZE 256

#define DEF_SIZELV_WIDTH 40   /* default width of fontsize listview */
#define DEF_NAMELV_WIDTH  100 /* default size of fontname listview */
#define DEF_PROP_WIDTH 20


/****************
**  FOTagHook  **
****************/

AROS_UFH3(VOID, FOTagHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    struct TagItem *tag, *tstate;

    struct IntFontReq *iforeq;

    D(bug("FOTagHook(hook=%p, pta=%p)\n", hook, pta));

    iforeq = (struct IntFontReq *)pta->pta_IntReq;

    tstate = pta->pta_Tags;

    while ((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
    {
	IPTR data = tag->ti_Data;

	switch (tag->ti_Tag)
	{
	    /* The tags that are put "in a row" are defined as the same value,
	    and therefor we only use one of them, but the effect is for all of them
	    */
	    case ASLFO_InitialName:
	    /*	case ASL_FontName:  Obsolete */
		iforeq->ifo_TextAttr.ta_Name = (STRPTR)data;
		break;

	    case ASLFO_InitialSize:
	    /* case ASL_FontHeight:  Obsolete */
		iforeq->ifo_TextAttr.ta_YSize = (UWORD)data;
		break;

	    case ASLFO_InitialStyle:
	    /*	case ASL_Styles:  Obsolete */
		iforeq->ifo_TextAttr.ta_Style = (UBYTE)data;
		break;

	    case ASLFO_UserData:
		((struct FontRequester *)pta->pta_Req)->fo_UserData = (APTR)data;
		break;

	    /* Options */

	    case ASLFO_Flags:
		iforeq->ifo_Flags = (UBYTE)data;
		/* Extract some flags that are common to all requester types and
		put them into IntReq->ir_Flags
		*/
		SETFLAG(GetIR(iforeq)->ir_Flags, (iforeq->ifo_Flags & FOF_PRIVATEIDCMP), IF_PRIVATEIDCMP);
		break;

	    case ASLFO_DoFrontPen:
		SETFLAG(iforeq->ifo_Flags, data, FOF_DOFRONTPEN);
		break;

	    case ASLFO_DoBackPen:
		SETFLAG(iforeq->ifo_Flags, data, FOF_DOBACKPEN);
		break;

	    case ASLFO_DoStyle:
		SETFLAG(iforeq->ifo_Flags, data, FOF_DOSTYLE);
		break;

	    case ASLFO_DoDrawMode:
		SETFLAG(iforeq->ifo_Flags, data, FOF_DODRAWMODE);
		break;

	    case ASLFO_FixedWidthOnly:
		SETFLAG(iforeq->ifo_Flags, data, FOF_FIXEDWIDTHONLY);
		break;

	    case ASLFO_FilterFunc:
		iforeq->ifo_FilterFunc = (struct Hook *)data;
		iforeq->ifo_Flags |= FOF_FILTERFUNC;
		break;

	    case ASLFO_MaxFrontPen:
		iforeq->ifo_MaxFrontPen = (UBYTE)data;
		break;

	    case ASLFO_MaxBackPen:
		iforeq->ifo_MaxBackPen = (UBYTE)data;
		break;

	    case ASLFO_ModeList:
		iforeq->ifo_ModeList = (STRPTR *)data;
		break;

	    case ASLFO_FrontPens:
		iforeq->ifo_FrontPens = (UBYTE *)data;
		break;

	    case ASLFO_BackPens:
		iforeq->ifo_BackPens = (UBYTE *)data;
		break;


	    default:
		break;
	} /* switch (tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&tstate)) != NULL) */

    ReturnVoid("FRTagHook");
}


/*********************
**  FOGadgetryHook  **
*********************/
AROS_UFH3(ULONG, FOGadgetryHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct LayoutData *,      ld,             A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    ULONG retval;

    switch (ld->ld_Command)
    {
    case LDCMD_INIT:
	retval = (ULONG)FOGadInit(ld, ASLB(AslBase));
	break;

    case LDCMD_LAYOUT:
	retval = (ULONG)FOGadLayout(ld, ASLB(AslBase));
	break;

    case LDCMD_HANDLEEVENTS:
	retval = (ULONG)FOHandleEvents(ld, ASLB(AslBase));
	break;

    case LDCMD_CLEANUP:
	FOGadCleanup(ld, ASLB(AslBase));
	retval = GHRET_OK;
	break;

    default:
	retval = GHRET_FAIL;
    }

    return (retval);
}

/****************
**  FOGadInit  **
****************/
STATIC const struct TagItem prop2lv[] =
{
    {PGA_Top, AROSA_Listview_First},
    {TAG_END}
};

STATIC const struct TagItem lv2prop[] =
{
    {AROSA_Listview_Total,	PGA_Total},
    {AROSA_Listview_Visible,	PGA_Visible},
    {AROSA_Listview_First,	PGA_Top},
    {TAG_END}
};


STATIC BOOL FOGadInit(struct LayoutData *ld, struct AslBase_intern *AslBase)
{

    struct FOUserData *udata;
    struct IntFontReq *iforeq;
    BOOL success = FALSE;
    STRPTR butstr[NUMBUTS];

    #define EXIT_ID 1



    D(bug("FRGadInit(ld=%p)\n", ld));

    udata = ld->ld_UserData;
    iforeq = (struct IntFontReq *)ld->ld_IntReq;

    udata->AFBuf = GetAF(AFF_MEMORY|AFF_DISK|AFF_SCALED, ASLB(AslBase));
    if (!udata->AFBuf)
	goto bye;

    udata->NameConstructHook.h_Entry = (APTR)AROS_ASMSYMNAME(NameConstructHook);
    udata->NameConstructHook.h_Data  = udata;
    udata->NameDestructHook.h_Entry  = (APTR)AROS_ASMSYMNAME(NameDestructHook);
    udata->NameDestructHook.h_Data   = udata;

    D(bug("\tCreating NameList\n"));
    udata->NameList = NewObject(NULL, AROSLISTCLASS,
			AROSA_List_ConstructHook, &(udata->NameConstructHook),
			AROSA_List_DestructHook,  &(udata->NameDestructHook),
			TAG_END);
    if (!udata->NameList)
	goto bye;

    D(bug("\tInitializing NameList\n"));
    /* Insert entries into the fontlist */
    if (!InitFontList(udata->NameList, udata->AFBuf, ASLB(AslBase)))
	goto bye;

    D(bug("\tCreating SizeList\n"));
    udata->SizeList = NewObject(NULL, AROSLISTCLASS,
			TAG_END);
    if (!udata->SizeList)
	goto bye;

    D(bug("\tAllocating disphookbuf\n"));
    udata->DispHookBuf = AllocMem(DISPHOOKBUFSIZE, MEMF_ANY);
    if (!udata->DispHookBuf)
	goto bye;

    udata->NameDisplayHook.h_Entry = (APTR)AROS_ASMSYMNAME(NameDisplayHook);
    udata->NameDisplayHook.h_Data  = udata;

    udata->SizeDisplayHook.h_Entry = (APTR)AROS_ASMSYMNAME(SizeDisplayHook);
    udata->SizeDisplayHook.h_Data  = udata;
    D(bug("\tCreating NameListView\n"));
    udata->NameListview = NewObject(NULL, AROSLISTVIEWCLASS,
		GA_ID,				ID_NAMELISTVIEW,
		GA_Immediate,			TRUE,
		AROSA_Listview_MaxColumns,	1,
		AROSA_Listview_DisplayHook,	&(udata->NameDisplayHook),
		AROSA_Listview_Format,		"P=l",
		TAG_END);

    if (!udata->NameListview)
	goto bye;

    ld->ld_GList = (struct Gadget *)udata->NameListview;
    D(bug("\tCreating SizeListView\n"));
    udata->SizeListview = NewObject(NULL, AROSLISTVIEWCLASS,
		GA_ID,				ID_SIZELISTVIEW,
		GA_Previous,			udata->NameListview,
		GA_Immediate,			TRUE,
		AROSA_Listview_MaxColumns,	1,
		AROSA_Listview_DisplayHook,	&(udata->SizeDisplayHook),
		AROSA_Listview_Format,		"P=l",
		TAG_END);

    if (!udata->SizeListview)
	goto bye;
    D(bug("\tCreating Prop\n"));
    udata->Prop = NewObject(NULL, PROPGCLASS,
				GA_Previous,	udata->SizeListview,
				TAG_END);

    if (!udata->Prop)
	goto bye;

    SetAttrs(udata->NameListview,
			ICA_TARGET,		udata->Prop,
			ICA_MAP,		lv2prop,
			TAG_END);

    SetAttrs(udata->Prop,       ICA_TARGET,     udata->NameListview,
				ICA_MAP,	prop2lv,
				TAG_END);


    D(bug("\tCreating Frame\n"));
    udata->ButFrame = NewObject(NULL, FRAMEICLASS, TAG_END);
    if (!udata->ButFrame)
	goto bye;
    D(bug("\tCreating OKBut\n"));
    udata->OKBut = NewObject(NULL, FRBUTTONCLASS,
			GA_Image,	udata->ButFrame,
			GA_Previous,	udata->Prop,
			GA_ID,		ID_BUTOK,
			GA_RelVerify,	TRUE,
			GA_Text,	GetIR(iforeq)->ir_PositiveText,
			TAG_END);
    if (!udata->OKBut)
	goto bye;

    D(bug("\tCreating CancelBut\n"));
    udata->CancelBut = NewObject(NULL, FRBUTTONCLASS,
			GA_Image,	udata->ButFrame,
			GA_Previous,	udata->OKBut,
			GA_ID,		ID_BUTCANCEL,
			GA_RelVerify,	TRUE,
			GA_Text,	GetIR(iforeq)->ir_NegativeText,
			TAG_END);
    if (!udata->CancelBut)
	goto bye;



    butstr[0] = GetIR(iforeq)->ir_PositiveText;
    butstr[1] = GetIR(iforeq)->ir_NegativeText;
    D(bug("\tGetting biggest textlength\n"));
    udata->ButWidth = BiggestTextLength(butstr,
					NUMBUTS,
					&(ld->ld_DummyRP),
					ASLB(AslBase));
    udata->ButHeight = ld->ld_Font->tf_YSize + 6;



    ld->ld_MinWidth =  ld->ld_Screen->WBorLeft + ld->ld_Screen->WBorRight
		     + MIN_SPACING * 3	+ DEF_NAMELV_WIDTH + DEF_SIZELV_WIDTH;

    ld->ld_MinHeight = ld->ld_Screen->WBorTop + ld->ld_Screen->Font->ta_YSize + 1
		  + MIN_SPACING * 3
		  + udata->ButHeight * 3;


    success = TRUE;

bye:

    if (!success)
    {
	FOGadCleanup(ld, ASLB(AslBase));
    }

    ReturnBool ("FRGadInit", success);

}

/******************
**  FOGadLayout  **
******************/

STATIC BOOL FOGadLayout(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    /* Get the window inner dimensions */

    struct Window *win;
    struct FOUserData *udata = (struct FOUserData *)ld->ld_UserData;


    D(bug("FRGadLayout(ld=%p)\n", ld));

    win     = ld->ld_Window;


    if (!(udata->Flags & FOFLG_LAYOUTED))
    {
	struct TagItem buttags[] ={
		{GA_RelBottom,	0},
		{GA_Width,	0},
		{GA_Height,	0},
		{TAG_END}};

	WORD left, butrelbottom;

	butrelbottom = - (win->BorderBottom + MIN_SPACING + udata->ButHeight);
	left = win->BorderLeft + MIN_SPACING;

	SetAttrs(udata->NameListview,
		GA_Left,	left,
		GA_Top, 	win->BorderTop	+ MIN_SPACING,
		GA_RelWidth,	- (   win->BorderRight
				    + 3 * MIN_SPACING
				    + DEF_SIZELV_WIDTH
				    + DEF_PROP_WIDTH),
		GA_RelHeight,	butrelbottom - MIN_SPACING - 2,
		TAG_END);

	SetAttrs(udata->SizeListview,
		GA_RelRight,	- (win->BorderRight + MIN_SPACING + DEF_SIZELV_WIDTH),
		GA_Top, 	win->BorderTop	+ MIN_SPACING,
		GA_Width,	DEF_SIZELV_WIDTH,
		GA_RelHeight,	butrelbottom - MIN_SPACING - 2,
		TAG_END);

	SetAttrs(udata->Prop,
		GA_RelRight,	- (  win->BorderRight
				   + 2 * MIN_SPACING
				   + DEF_SIZELV_WIDTH
				   + DEF_PROP_WIDTH),
		GA_Top, 	win->BorderTop	+ MIN_SPACING,
		GA_Width,	DEF_PROP_WIDTH,
		GA_RelHeight,	butrelbottom - MIN_SPACING - 2,
		TAG_END);


	/* Need to SetGadgetAttrs() this if notification should work
	** (needs GadgetInfo)
	*/
	SetGadgetAttrs((struct Gadget *)udata->NameListview, ld->ld_Window, NULL,
				AROSA_Listview_List,	udata->NameList,
				TAG_END);

	SetGadgetAttrs((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL,
				AROSA_Listview_List,	udata->SizeList,
				TAG_END);


	/* GA_RelBottom */
	buttags[0].ti_Data = butrelbottom;
	/* GA_Width */
	buttags[1].ti_Data = udata->ButWidth;
	/* GA_Height */
	buttags[2].ti_Data = udata->ButHeight;

	SetAttrs(udata->OKBut,
		GA_Left,	left,
		TAG_MORE,	buttags);

	SetAttrs(udata->CancelBut,
		GA_RelRight,	-(win->BorderRight + MIN_SPACING + udata->ButWidth),
		TAG_MORE,	buttags);

	udata->Flags |= FOFLG_LAYOUTED;
    };

    ReturnBool ("FOGadLayout", TRUE );
}


/*********************
**  FOHandleEvents  **
*********************/

#undef Exit
#define Exit(ret) {retval = ret; break;}
STATIC ULONG FOHandleEvents(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct IntuiMessage *imsg;
    ULONG retval =GHRET_OK;
    struct FOUserData *udata = (struct FOUserData *)ld->ld_UserData;


    imsg = ld->ld_Event;
D(bug("FRHandleEvents: Class: %d\n", imsg->Class));
    switch (imsg->Class)
    {
    case IDCMP_GADGETUP:
	switch (((struct Gadget *)imsg->IAddress)->GadgetID)
	{
	case ID_BUTCANCEL:
	    retval = FALSE;
	    break;

	case ID_SIZELISTVIEW:
	    break;

	case ID_NAMELISTVIEW:
	{
	    LONG active;
	    struct NameEntry *entry;

	    /* Get pos of active entry */
	    GetAttr(AROSA_List_Active, udata->NameList, &active);

	    D(bug("Selected pos: %d", active));

	    DoMethod(udata->NameList, AROSM_List_GetEntry, active, &entry);

	    D(bug("Got entry\nname: %s\n", entry->ne_FontName));

	    RemoveGList(ld->ld_Window, (struct Gadget *)udata->SizeListview, 1);

	    D(bug("Getting sizelist\n"));
	    if (!GetSizeList(udata->SizeList, entry, udata->AFBuf, ASLB(AslBase)))
		{retval = FALSE; break; }
	    D(bug("Re-adding listview\n"));
	    AddGList(ld->ld_Window, (struct Gadget *)udata->SizeListview, -1, 1, NULL);
	    D(bug("Refreshing GList\n"));
	    RefreshGList((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, 1);
	    D(bug("GList refreshed\n"));


	} break;


	case ID_BUTOK:
	    break;
	}

    } /* switch (imsg->Class) */

    return (retval);

}

/*******************
**  FOGadCleanup  **
*******************/
STATIC VOID FOGadCleanup(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData *udata = (struct FOUserData *)ld->ld_UserData;

    EnterFunc(bug("FOGadCleanup(ld=%p)\n", ld));

    if (udata->NameList)
	DisposeObject(udata->NameList);

    if (udata->SizeList)
	DisposeObject(udata->SizeList);

    if (udata->NameListview)
	DisposeObject(udata->NameListview);

    if (udata->SizeListview)
	DisposeObject(udata->SizeListview);

    if (udata->Prop)
	DisposeObject(udata->Prop);

    if (udata->OKBut)
	DisposeObject(udata->OKBut);

    if (udata->CancelBut)
	DisposeObject(udata->CancelBut);

    if (udata->ButFrame)
	DisposeObject(udata->ButFrame);

    if (udata->DispHookBuf)
	FreeMem(udata->DispHookBuf, DISPHOOKBUFSIZE);

    if (udata->AFBuf)
	FreeVec(udata->AFBuf);

    ReturnVoid("FOGadCleanup");
}


/************
**  GetAF  **
************/

STATIC struct AvailFontsHeader *GetAF(LONG flags, struct AslBase_intern *AslBase)
{
    /* Makes an initial read of the fonts available on the system. */

    struct Library *DiskfontBase;
    struct AvailFontsHeader *afh;
    /* Try to guess how many bytes are needed */
    ULONG afsize	= 10000;
    ULONG afshortage;

    DiskfontBase = OpenLibrary("diskfont.library", 37);
    if (!DiskfontBase)
	return (FALSE);

    do
    {
	afh = (struct AvailFontsHeader *)AllocVec(afsize, MEMF_ANY);
	if (afh)
	{
	    afshortage = AvailFonts((STRPTR)afh, afsize, AFF_MEMORY|AFF_DISK);
	    if (afshortage)
	    {
		FreeVec(afh);
		afsize += afshortage;
		afh = (struct AvailFontsHeader*)(-1L);
	    }
	}
    } while (afh && afshortage);

    /* Sort the availfonts array */

    CloseLibrary(DiskfontBase);

    return (afh);
}




/*******************
**  InitFontList  **
*******************/
STATIC int AFSortFunc(const struct AvailFonts *af1, const struct AvailFonts *af2)
{
    int retval;

    retval = strcmp(af1->af_Attr.ta_Name, af2->af_Attr.ta_Name);

    if (retval == 0)
    {
	retval = af1->af_Attr.ta_YSize - af2->af_Attr.ta_YSize;
    }
    return (retval);
}

STATIC BOOL InitFontList(Object *fontlist, struct AvailFontsHeader *afh, struct AslBase_intern *AslBase)
{
    struct AvailFonts *afptr = (struct AvailFonts *)&afh[1];
    STRPTR lastname;

    register UWORD i;
    register UWORD numentries = afh->afh_NumEntries;

    D(bug("\tSorting the fonts\n"));
    /* Sort the buffer of available fonts */
    qsort(afptr,
	numentries,
	sizeof(struct AvailFonts),
	(void *)AFSortFunc);

    D(bug("\tInserting fonts\n"));
    lastname = "blah";

    for (i = 0; i < numentries; i ++)
    {
	STRPTR fontname = afptr->af_Attr.ta_Name;

	/* Only insert distinct entries */
	if (strcmp(lastname, fontname) != 0)
	{
	    struct NameEntry ne;

	    ne.ne_FontName = fontname;
	    ne.ne_AFOffset = i;

	    D(bug("\tInserting font &d: %s\n", i, ne.ne_FontName));
	    if (!DoMethod(fontlist, AROSM_List_InsertSingle, &ne, AROSV_List_Insert_Bottom))
		return (FALSE);

	    lastname = fontname;
	} /* if (fontname is not the same as that of the last AvailFonts struct) */

	afptr ++;
    } /* for (all AvailFonts structs in the buffer) */
    return (TRUE);
}

/******************
**  GetSizeList  **
******************/

STATIC BOOL GetSizeList(Object          *sizelist,
		struct NameEntry	*ne,
		struct AvailFontsHeader *afh,
		struct AslBase_intern	*AslBase)
{


    struct AvailFonts *afptr = (struct AvailFonts *)&afh[1];
    UWORD lastsize;
    BOOL more = TRUE;
    UWORD offset = ne->ne_AFOffset;

    D(bug("GetFileList(list=%p, ne=%s, afh=%p)\n", sizelist, ne->ne_FontName, afh));
    DoMethod(sizelist, AROSM_List_Clear);

    /* Get offset to first entry matching NameEntry in the sorted buf of AvailFonts */
    D(bug("offset: %d\n", ne->ne_AFOffset));
    afptr = &afptr[offset];

    D(bug("afptr=%s\n", afptr->af_Attr.ta_Name));

    lastsize = 0;

    /* Get sizes from fonts of equal size */
    do /* There is of course alwasy at least 1 font in the AF buf with this name */
    {
	D(bug("Compared fontnames\n"));
	D(bug("Ysize: %d\n", afptr->af_Attr.ta_YSize));
	if (afptr->af_Attr.ta_YSize != lastsize)
	{
	    if (!DoMethod(sizelist,
			AROSM_List_InsertSingle,
			&(afptr->af_Attr.ta_YSize),
			AROSV_List_Insert_Bottom))
	    {
		return (FALSE);
	    }

	    lastsize = afptr->af_Attr.ta_YSize;
	}
	afptr ++;
	offset ++;

	/* Last entry in buffer ? */
	if (offset != afh->afh_NumEntries)
	{
	    if (strcmp(ne->ne_FontName, afptr->af_Attr.ta_Name) != 0)
		more = FALSE;
	}
	else
	     more = FALSE;
    }
    while (more);

    ReturnBool ("GetSizeList", TRUE);
}

/**********************
**  NameDisplayHook  **
**********************/
AROS_UFH3S(VOID, NameDisplayHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(STRPTR *,                 dharray,        A2),
    AROS_UFHA(struct NameEntry *,       ne,             A1)
)
{
     *dharray = ne->ne_FontName;

     return;
}

/**********************
**  SizeDisplayHook  **
**********************/
AROS_UFH3S(VOID, SizeDisplayHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(STRPTR *,                 dharray,        A2),
    AROS_UFHA(UWORD *,                  ysize,          A1)
)
{
    struct FOUserData *udata = hook->h_Data;

    snprintf(udata->DispHookBuf, DISPHOOKBUFSIZE, "%d", *ysize);
    *dharray = udata->DispHookBuf;

    return;
}


/************************
**  NameConstructHook  **
************************/
AROS_UFH3S(APTR, NameConstructHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(APTR,                     pool,           A2),
    AROS_UFHA(struct NameEntry *,       ne,             A1)
)
{
    struct NameEntry *newne;


    newne = AllocPooled(pool, sizeof (struct NameEntry));
    if (!newne)
	return (NULL);

    /* We do not need to copy the fontname, since the buffer filled
    ** by AvailFonts() won't be freed until after the listview
    ** is disposed of
    */
    newne->ne_FontName = ne->ne_FontName;
    newne->ne_AFOffset = ne->ne_AFOffset;

    D(bug("\tNameConstructHook %s, %d\n", newne->ne_FontName, newne->ne_AFOffset));

    return (newne);
}

/***********************
**  NameDestructHook  **
***********************/

AROS_UFH3S(VOID, NameDestructHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(APTR,                     pool,           A2),
    AROS_UFHA(struct NameEntry *,       ne,             A1)
)
{

    FreePooled(pool, ne, sizeof (struct NameEntry));

    return;
}
