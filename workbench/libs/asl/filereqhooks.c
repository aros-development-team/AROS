/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: File requester specific code.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/boopsi.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <gadgets/aroslistview.h>
#include <gadgets/aroslist.h>
#include <string.h>

#include "asl_intern.h"
#include "filereqhooks.h"
#include "layout.h"
#include "dirlist.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>



STATIC BOOL FRGadInit(struct LayoutData *, struct AslBase_intern *);
STATIC BOOL FRGadLayout(struct LayoutData *, struct AslBase_intern *);
STATIC VOID FRGadCleanup(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG FRHandleEvents(struct LayoutData *, struct AslBase_intern *);

STATIC ULONG GetSelectedFiles(struct FRUserData *, struct LayoutData *, struct AslBase_intern *AslBase);




#define ID_BUTOK	1
#define ID_BUTVOLUMES	2
#define ID_BUTPARENT	3
#define ID_BUTCANCEL	4

#define ID_DIRLIST	5

#undef NUMBUTS
#define NUMBUTS 4L


/****************
**  FRTagHook  **
****************/

AROS_UFH3(VOID, FRTagHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    struct TagItem *tag, *tstate;

    struct IntFileReq *ifreq;

    EnterFunc(bug("FRTagHook(hook=%p, pta=%p)\n", hook, pta));

    ifreq = (struct IntFileReq *)pta->pta_IntReq;

    tstate = pta->pta_Tags;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {

	switch (tag->ti_Tag)
	{
	    /* The tags that are put "in a row" are defined as the same value,
	    and therefor we only use one of them, but the effect is for all of them
	    */
	    case ASLFR_InitialDrawer:
	    /*	case ASL_Dir:  Obsolete */
		ifreq->ifr_Drawer = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_InitialFile:
	    /* case ASL_File:  Obsolete */
		ifreq->ifr_File = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_InitialPattern:
	    /*	case ASL_Pattern:  Obsolete */
		ifreq->ifr_Pattern = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_UserData:
		((struct FileRequester *)pta->pta_Req)->fr_UserData = (APTR)tag->ti_Data;
		break;

	    /* Options */

	    case ASLFR_Flags1:
		ifreq->ifr_Flags1 = (UBYTE)tag->ti_Data;
		/* Extract some flags that are common to all requester types and
		put them into IntReq->ir_Flags
		*/
		if (ifreq->ifr_Flags1 & FRF_PRIVATEIDCMP)
		    GetIR(ifreq)->ir_Flags |= IF_PRIVATEIDCMP;
		else
		    GetIR(ifreq)->ir_Flags &= ~IF_PRIVATEIDCMP;
		break;

	    case ASLFR_Flags2:
		ifreq->ifr_Flags2 = (UBYTE)tag->ti_Data;
		break;

	    case ASLFR_DoSaveMode:
		if (tag->ti_Data)
		    ifreq->ifr_Flags1 |= FRF_DOSAVEMODE;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOSAVEMODE;
		break;

	    case ASLFR_DoMultiSelect:
		if (tag->ti_Data)
		    ifreq->ifr_Flags1 |= FRF_DOMULTISELECT;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOMULTISELECT;
		break;

	    case ASLFR_DoPatterns:
		if (tag->ti_Data)
		    ifreq->ifr_Flags1 |= FRF_DOPATTERNS;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOPATTERNS;
		break;

	    case ASLFR_FilterFunc:
		ifreq->ifr_FilterFunc = (struct Hook *)tag->ti_Data;
		ifreq->ifr_Flags1 |= FRF_FILTERFUNC;
		break;

	    case ASLFR_DrawersOnly:
		if (tag->ti_Data)
		    ifreq->ifr_Flags2 |= FRF_DRAWERSONLY;
		else
		    ifreq->ifr_Flags2 &= ~FRF_DRAWERSONLY;
		break;

	    case ASLFR_FilterDrawers:
		if (tag->ti_Data)
		    ifreq->ifr_Flags2 |= FRF_FILTERDRAWERS;
		else
		    ifreq->ifr_Flags2 &= ~FRF_FILTERDRAWERS;
		break;

	    case ASLFR_RejectIcons:
		if (tag->ti_Data)
		    ifreq->ifr_Flags2 |= FRF_REJECTICONS;
		else
		    ifreq->ifr_Flags2 &= ~FRF_REJECTICONS;
		break;


	    default:
		break;
	} /* switch (tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&tstate)) != 0) */

    ReturnVoid("FRTagHook");
}


/*********************
**  FRGadgetryHook  **
*********************/
AROS_UFH3(ULONG, FRGadgetryHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct LayoutData *,      ld,             A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    ULONG retval;

    switch (ld->ld_Command)
    {
    case LDCMD_INIT:
	retval = (ULONG)FRGadInit(ld, ASLB(AslBase));
	break;

    case LDCMD_LAYOUT:
	retval = (ULONG)FRGadLayout(ld, ASLB(AslBase));
	break;

    case LDCMD_HANDLEEVENTS:
	retval = (ULONG)FRHandleEvents(ld, ASLB(AslBase));
	break;

    case LDCMD_CLEANUP:
	FRGadCleanup(ld, ASLB(AslBase));
	retval = GHRET_OK;
	break;

    default:
	retval = GHRET_FAIL;
	break;
    }

    return (retval);
}


/****************
**  FRGadInit  **
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


STATIC BOOL FRGadInit(struct LayoutData *ld, struct AslBase_intern *AslBase)
{

    struct FRUserData *udata;
    struct IntFileReq *ifreq;
    STRPTR butstr[NUMBUTS];

    #define EXIT_ID 1



    EnterFunc(bug("FRGadInit(ld=%p)\n", ld));

    udata = ld->ld_UserData;
    ifreq = (struct IntFileReq *)ld->ld_IntReq;

    udata->DirListClass = InitDirListClass(AslBase);
    if (!udata->DirListClass)
	goto failure;

    D(bug("Dirlist class created\n"));	

    udata->DirList = NewObject(udata->DirListClass, NULL,
		GA_ID,				ID_DIRLIST,
		GA_Immediate,			TRUE,
		AROSA_DirList_Path,		ifreq->ifr_Drawer,
/*		AROSA_Listview_MultiSelect,	TRUE, */
		TAG_DONE);

    if (!udata->DirList)
	goto failure;

    /* Needs to return pointer to first gadget so that AslRequest can AddGList() them */
    ld->ld_GList = (struct Gadget *)udata->DirList;

    D(bug("DirList created: %p\n", udata->DirList));
    

    udata->Prop = NewObject(NULL, PROPGCLASS,
				GA_Previous,	udata->DirList,
				TAG_END);

    if (!udata->Prop)
	goto failure;

D(bug("Prop created\n"));
    SetAttrs(udata->DirList,
			ICA_TARGET,		udata->Prop,
			ICA_MAP,		lv2prop,
			TAG_END);

    SetAttrs(udata->Prop,       ICA_TARGET,     udata->DirList,
				ICA_MAP,	prop2lv,
				TAG_END);


    udata->ButFrame = NewObject(NULL, FRAMEICLASS, TAG_END);
    if (!udata->ButFrame)
	goto failure;

    udata->OKBut = NewObject(NULL, FRBUTTONCLASS,
			GA_Image,	udata->ButFrame,
			GA_Previous,	udata->Prop,
			GA_ID,		ID_BUTOK,
			GA_RelVerify,	TRUE,
			GA_Text,	GetIR(ifreq)->ir_PositiveText,
			TAG_END);
    if (!udata->OKBut)
	goto failure;
D(bug("OKBut created\n"));
    udata->VolumesBut = NewObject(NULL, FRBUTTONCLASS,
			GA_Image,	udata->ButFrame,
			GA_Previous,	udata->OKBut,
			GA_ID,		ID_BUTVOLUMES,
			GA_RelVerify,	TRUE,
			GA_Text,	ifreq->ifr_VolumesText,
			TAG_END);
    if (!udata->VolumesBut)
	goto failure;
D(bug("VolumesBut created\n"));
    udata->ParentBut = NewObject(NULL, FRBUTTONCLASS,
			GA_Image,	udata->ButFrame,
			GA_Previous,	udata->VolumesBut,
			GA_ID,		ID_BUTPARENT,
			GA_RelVerify,	TRUE,
			GA_Text,	ifreq->ifr_ParentText,
			TAG_END);

    if (!udata->ParentBut)
	goto failure;
D(bug("ParentBut created\n"));

    udata->CancelBut = NewObject(NULL, FRBUTTONCLASS,
			GA_Image,	udata->ButFrame,
			GA_Previous,	udata->ParentBut,
			GA_ID,		ID_BUTCANCEL,
			GA_RelVerify,	TRUE,
			GA_Text,	GetIR(ifreq)->ir_NegativeText,
			TAG_END);
    if (!udata->CancelBut)
	goto failure;

D(bug("CancelBut created\n"));

    butstr[0] = GetIR(ifreq)->ir_PositiveText;
    butstr[1] = ifreq->ifr_VolumesText;
    butstr[2] = ifreq->ifr_ParentText;
    butstr[3] = GetIR(ifreq)->ir_NegativeText;

    udata->ButWidth = 8 + BiggestTextLength(butstr,
					    NUMBUTS,
					    &(ld->ld_DummyRP),
					    ASLB(AslBase));
D(bug("Got biggest textlength\n"));					
    D(bug("screen=%p\nscreen font=%p\n, ld font=%p\n"
    	, ld->ld_Screen, ld->ld_Screen->Font, ld->ld_Font));
    udata->ButHeight = ld->ld_Font->tf_YSize + 6;


    /* The bottom buttonrow's widths are most significant
    ** to the window's minimal width
    */
    

    ld->ld_MinWidth =  ld->ld_Screen->WBorLeft + ld->ld_Screen->WBorRight
		     + MIN_SPACING * (NUMBUTS + 5)
		     + udata->ButWidth * NUMBUTS;

    ld->ld_MinHeight = ld->ld_Screen->WBorTop + ld->ld_Font->tf_YSize + 1
		  + MIN_SPACING * 3
		  + udata->ButHeight * 3;

    ReturnBool ("FRGadInit", TRUE);
failure:

D(bug("failure\n"));

    FRGadCleanup(ld, ASLB(AslBase));

    ReturnBool ("FRGadInit", FALSE);

}

/******************
**  FRGadLayout  **
******************/

STATIC BOOL FRGadLayout(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    /* Get the window inner dimensions */

    struct Window *win;
    struct FRUserData *udata;

    WORD left, butrelbottom;

    WORD temp;

    struct TagItem buttags[] ={
	{GA_RelBottom,	0},
	{GA_Width,	0},
	{GA_Height,	0},
	{TAG_END}};


    EnterFunc(bug("FRGadLayout(ld=%p)\n", ld));

    udata   = (struct FRUserData *)ld->ld_UserData;
    win     = ld->ld_Window;


    butrelbottom = - (win->BorderBottom + udata->ButHeight + 1);
    
/* kprintf("butrelbuttom: %d, borderbottom: %d, minspacing: %d, butheight: %d\n"
	, butrelbottom, win->BorderBottom, MIN_SPACING, udata->ButHeight);
*/	
    left = win->BorderLeft + MIN_SPACING;

    if (!(udata->Flags & FRFLG_LAYOUTED))
    {
	Object *list;

	SetAttrs(udata->DirList,
		GA_Left,	left,
		GA_Top, 	win->BorderTop	+ MIN_SPACING,
		GA_RelWidth,	- (win->BorderRight  + 3 * MIN_SPACING + DEF_PROPWIDTH),
		GA_RelHeight,	butrelbottom - MIN_SPACING * 3 - 2,
		TAG_END);

	SetAttrs(udata->Prop,
		GA_RelRight,	- (win->BorderRight  + MIN_SPACING + DEF_PROPWIDTH),
		GA_Top, 	win->BorderTop	+ MIN_SPACING,
		GA_Width,	DEF_PROPWIDTH,
		GA_RelHeight,	butrelbottom - MIN_SPACING * 3 - 2,
		TAG_END);

	/* Get & set the list, so that the prop receives OM_UPDATE and resizes
	** itself properly.
	*/
	GetAttr(AROSA_Listview_List, udata->DirList, (IPTR *)&list);

	SetGadgetAttrs((struct Gadget *)udata->DirList, ld->ld_Window, NULL,
			AROSA_Listview_List,	list,
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

	SetAttrs(udata->VolumesBut,
		GA_Left, left + MIN_SPACING * 2 + udata->ButWidth,
		TAG_MORE,	buttags);

	temp = -(win->BorderRight + MIN_SPACING + udata->ButWidth);

	SetAttrs(udata->CancelBut,
		GA_RelRight,	temp,
		TAG_MORE,	buttags);

	temp -= MIN_SPACING * 2 + udata->ButWidth;

	SetAttrs(udata->ParentBut,
		GA_RelRight,	temp,
		TAG_MORE,	buttags);

	udata->Flags |= FRFLG_LAYOUTED;
    };

    ReturnBool ("FRGadLayout", TRUE );
}


/*********************
**  FRHandleEvents  **
*********************/

STATIC ULONG FRHandleEvents(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct IntuiMessage *imsg;
    ULONG retval =GHRET_OK;
    struct FRUserData *udata = (struct FRUserData *)ld->ld_UserData;


EnterFunc(bug("FRHandleEvents: Class: %d\n", imsg->Class));

    imsg = ld->ld_Event;


    switch (imsg->Class)
    {
    case IDCMP_GADGETUP:

	D(bug("GADGETUP! gadgetid=%d\n", ((struct Gadget *)imsg->IAddress)->GadgetID));

	switch (((struct Gadget *)imsg->IAddress)->GadgetID)
	{
	case ID_BUTCANCEL:
	    retval = FALSE;
	    break;

	case ID_BUTVOLUMES:
	    DoGadgetMethod((struct Gadget *)udata->DirList,
			ld->ld_Window,
			NULL,
			AROSM_DirList_ShowVolumes, NULL);
	    break;

	case ID_BUTPARENT:
	    DoGadgetMethod((struct Gadget *)udata->DirList,
			ld->ld_Window,
			NULL,
			AROSM_DirList_ShowParent, NULL);
	    break;

	case ID_BUTOK:
	    retval = GetSelectedFiles(udata, ld, ASLB(AslBase));
	    break;
	
	case ID_DIRLIST:
	    {
		IPTR doubleclicked = FALSE;
	    
		GetAttr(AROSA_Listview_DoubleClick, udata->DirList, (IPTR *)&doubleclicked);
		
	   	if (doubleclicked)
		{
		    retval = GetSelectedFiles(udata, ld, ASLB(AslBase));
		}
	    }
	    break;
	    
	} /* switch (gadget ID) */

	break; /* case IDCMP_GADGETUP: */

    } /* switch (imsg->Class) */

    ReturnInt ("FRHandleEvents", ULONG, retval);
}

/*******************
**  FRGadCleanup  **
*******************/
STATIC VOID FRGadCleanup(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData *udata;

    EnterFunc(bug("FRGadCleanup(ld=%p)\n", ld));
    udata = (struct FRUserData *)ld->ld_UserData;

    if (udata->DirList)
	DisposeObject(udata->DirList);

    if (udata->Prop)
	DisposeObject(udata->Prop);

    if (udata->OKBut)
	DisposeObject(udata->OKBut);

    if (udata->VolumesBut)
	DisposeObject(udata->VolumesBut);

    if (udata->ParentBut)
	DisposeObject(udata->ParentBut);

    if (udata->CancelBut)
	DisposeObject(udata->CancelBut);

    if (udata->ButFrame)
	DisposeObject(udata->ButFrame);

    if (udata->DirListClass)
	CleanupDirListClass(udata->DirListClass, AslBase);

    ReturnVoid("FRGadCleanup");
}

STATIC ULONG GetSelectedFiles(  struct FRUserData       *udata,
				struct LayoutData	*ld,
				struct AslBase_intern	*AslBase)
{
    struct path *cur_path;
    
    ULONG volumes_shown;
    ULONG retval = GHRET_OK;

    GetAttr(AROSA_DirList_VolumesShown, udata->DirList, &volumes_shown);

    if (volumes_shown)
	retval = GHRET_OK;
    else
    {
	struct IntFileReq *ifreq;
	struct FileRequester *req;
	Object *filelist;


	GetAttr(AROSA_Listview_List, udata->DirList, (IPTR *)&filelist);

	ifreq = (struct IntFileReq *)ld->ld_IntReq;
	req = (struct FileRequester *)ld->ld_Req;

	if (!(ifreq->ifr_Flags1 & FRF_DOMULTISELECT))
	{
	    LONG active;
	    struct ExAllData *ead;

	    GetAttr(AROSA_List_Active, filelist, &active);

	    if (active == AROSV_List_Active_None)
		{ retval = GHRET_OK; goto bye; }

	    DoMethod(filelist, AROSM_List_GetEntry, active, &ead);
	    /* File or directory selected ? */

	    if (ead->ed_Type > 0)
		{ retval = GHRET_OK; goto bye; }

	    /* The next allocated stuff is freed by the StripRequester()
	    ** function if the allocations fail.
	    */
	    
	    #if 0
	    	/* Georg Steger: udata->CurPath 0 ??? never used ???? */
		
	    	req->fr_Drawer = AllocVec(strlen(udata->CurPath) + 1, MEMF_ANY);
	    
	    #else

	    	GetAttr(AROSA_DirList_Path, udata->DirList, (IPTR *)&cur_path);

	    	req->fr_Drawer = AllocVec(strlen(path_string(cur_path)) + 1, MEMF_ANY);

	    #endif
	    
	    if (!req->fr_Drawer)
		{ retval = FALSE; goto bye; }

	    req->fr_File = AllocVec(strlen(ead->ed_Name) + 1, MEMF_ANY);
	    if (!req->fr_File)
		{ retval = FALSE; goto bye; }

	    #if 0
	    	/* Georg Steger: udata->CurPath unused?? */
		
	    	strcpy(req->fr_Drawer, udata->CurPath);
	    
	    #else
	    
	    	strcpy(req->fr_Drawer, path_string(cur_path));
	    
	    #endif
	    
	    strcpy(req->fr_File, ead->ed_Name);
	    
	    retval = GHRET_FINISHED_OK;
	}
	else
	{
	    LONG selcount;
	    struct WBArg *wbarg;
	    LONG id;

	    /* Create a WBArg structure for the selected files */

	    DoMethod(filelist,
		AROSM_List_Select,
		AROSV_List_Select_All,
		AROSV_List_Select_Ask,
		&selcount);

	    /* Allocate WBArg structures */
	    req->fr_ArgList = wbarg = AllocVec(
				selcount * sizeof (struct WBArg),
				MEMF_ANY|MEMF_CLEAR);
	    if (!req->fr_ArgList)
		{ retval = FALSE; goto bye; }

	    /* Iterate list and put selected files into WBArg structs */
	    id = AROSV_List_NextSelected_Start;

	    req->fr_NumArgs = selcount;

	    for (;;)
	    {
		struct ExAllData *ead;

		DoMethod(filelist, AROSM_List_NextSelected, &id);
		if (id == AROSV_List_NextSelected_End)
		    break;

		DoMethod(filelist, AROSM_List_GetEntry, &ead);

		wbarg->wa_Name = AllocVec(strlen(ead->ed_Name) + 1, MEMF_ANY);
		if (!wbarg->wa_Name)
		    { retval = FALSE; goto bye; }

		strcpy(wbarg->wa_Name, ead->ed_Name);

	    } /* for (iterate list for selected files) */

	} /* if (single- or multiselect ?) */

    } /* if (volumes currently shown ?) */

bye:
    return (retval);
}
