/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: File requester specific code.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <gadgets/aroslistview.h>
#include <gadgets/aroslist.h>
#include <string.h>

#include "asl_intern.h"
#include "filereqhooks.h"
#include "layout.h"

#undef TURN_OFF_DEBUG

#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>



STATIC BOOL FRGadInit(struct LayoutData *, struct AslBase_intern *);
STATIC BOOL FRGadLayout(struct LayoutData *, struct AslBase_intern *);
STATIC VOID FRGadCleanup(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG FRHandleEvents(struct LayoutData *, struct AslBase_intern *);

STATIC BOOL AddToPath(STRPTR, STRPTR *, ULONG *, struct AslBase_intern *);

STATIC BOOL UpdateFileList(struct FRUserData *, struct Window *, struct AslBase_intern *);




#define ID_BUTOK	1
#define	ID_BUTVOLUMES	2
#define ID_BUTPARENT	3
#define ID_BUTCANCEL	4

#define ID_LISTVIEW	5

#undef NUMBUTS
#define NUMBUTS 4L


/****************
**  FRTagHook  **
****************/

AROS_UFH3(VOID, FRTagHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(struct ParseTagArgs *,	pta,		A2),
    AROS_UFHA(struct AslBase_intern *,	AslBase,	A1)
)
{
    struct TagItem *tag, *tstate;
	
    struct IntFileReq *ifreq;
    
    D(bug("FRTagHook(hook=%p, pta=%p)\n", hook, pta));    

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
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(struct LayoutData *,	ld,		A2),
    AROS_UFHA(struct AslBase_intern *,	AslBase,	A1)
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
    BOOL success = FALSE;
    STRPTR butstr[NUMBUTS];
    
    #define EXIT_ID 1

    
    
    D(bug("FRGadInit(ld=%p)\n", ld));

    udata = ld->ld_UserData;
    ifreq = (struct IntFileReq *)ld->ld_IntReq;


    if (!AddToPath( ifreq->ifr_Drawer,
    		    &(udata->CurPath),
    		    &(udata->PathBufSize),
    		    ASLB(AslBase)))
    	goto bye;
    	

/* Create file list */
    udata->FLConstructHook.h_Entry = (APTR)AROS_ASMSYMNAME(FLConstructHook);
    udata->FLConstructHook.h_Data  = AslBase;
    udata->FLDestructHook.h_Entry = (APTR)AROS_ASMSYMNAME(FLDestructHook);
    udata->FLDestructHook.h_Data  = AslBase;

    
    udata->FileList = NewObject(NULL, AROSLISTCLASS, 
    			AROSA_List_ConstructHook, &(udata->FLConstructHook),
    			AROSA_List_DestructHook, &(udata->FLDestructHook),
    			TAG_END);
    if (!udata->FileList)
    	goto bye;    			
    	
    D(bug("\tFilelist created\n"));

/* Create volume list */
    udata->VLConstructHook.h_Entry = (APTR)AROS_ASMSYMNAME(VLConstructHook);
    udata->VLConstructHook.h_Data  = AslBase;
    udata->VLDestructHook.h_Entry = (APTR)AROS_ASMSYMNAME(VLDestructHook);
    udata->VLDestructHook.h_Data  = AslBase;

    
    udata->VolumesList = NewObject(NULL, AROSLISTCLASS, 
    			AROSA_List_ConstructHook, &(udata->VLConstructHook),
    			AROSA_List_DestructHook, &(udata->VLDestructHook),
    			TAG_END);

    if (!udata->VolumesList)
    	goto bye;

    if (!GetDir(udata->CurPath, udata->FileList, ASLB(AslBase)))
    	goto bye;
    
    if (!GetVolumes(udata->VolumesList, ASLB(AslBase)))
    	goto bye;
    	
    udata->Flags |= FRFLG_FILELIST;

    udata->DispHookBuf = AllocMem(DISPHOOKBUFSIZE, MEMF_ANY);
    if (!udata->DispHookBuf)	
    	goto bye;

    udata->VLDisplayHook.h_Entry = (APTR)AROS_ASMSYMNAME(VLDisplayHook);
    udata->VLDisplayHook.h_Data  = udata;

    udata->FLDisplayHook.h_Entry = (APTR)AROS_ASMSYMNAME(FLDisplayHook);
    udata->FLDisplayHook.h_Data  = udata;
    
    udata->Listview = NewObject(NULL, AROSLISTVIEWCLASS, 
    		GA_ID,				ID_LISTVIEW,
   		GA_Immediate,		    	TRUE,
    		AROSA_Listview_MaxColumns,  	2,
    		AROSA_Listview_DisplayHook, 	&(udata->FLDisplayHook),
    		AROSA_Listview_Format,	    	"P=l, P=l",
//    		AROSA_Listview_MultiSelect, 	TRUE,
    		TAG_END);
    	
    if (!udata->Listview)
    	goto bye;
    	
    udata->Prop = NewObject(NULL, PROPGCLASS, 
    				GA_Previous,	udata->Listview,
    				TAG_END);
       				
    if (!udata->Prop)
    	goto bye;
    	
    SetAttrs(udata->Listview, 
    			ICA_TARGET,		udata->Prop,
    			ICA_MAP,		lv2prop,
    			TAG_END);

    SetAttrs(udata->Prop,	ICA_TARGET,	udata->Listview,
    				ICA_MAP,	prop2lv,
    				TAG_END);    		
   	
    /* Remember this gadget so we can free it in FreeCommon */
    ld->ld_GList = (struct Gadget *)udata->Listview;

    D(bug("Listview created: %p\n", udata->Listview));
    
    udata->ButFrame = NewObject(NULL, FRAMEICLASS, TAG_END);
    if (!udata->ButFrame)
    	goto bye;
    			
    udata->OKBut = NewObject(NULL, FRBUTTONCLASS,
    			GA_Image,	udata->ButFrame,
    			GA_Previous,	udata->Prop,
    			GA_ID, 		ID_BUTOK,
    			GA_RelVerify,	TRUE,
    			GA_Text,	GetIR(ifreq)->ir_PositiveText,
    			TAG_END);
    if (!udata->OKBut)
    	goto bye;
    	
    udata->VolumesBut = NewObject(NULL, FRBUTTONCLASS,
    			GA_Image,	udata->ButFrame,
    			GA_Previous,	udata->OKBut,
    			GA_ID, 		ID_BUTVOLUMES,
    			GA_RelVerify,	TRUE,
    			GA_Text,	ifreq->ifr_VolumesText,
    			TAG_END);
    if (!udata->VolumesBut)
   	goto bye;
   	
    udata->ParentBut = NewObject(NULL, FRBUTTONCLASS,
    			GA_Image,	udata->ButFrame,
    			GA_Previous,	udata->VolumesBut,
    			GA_ID, 		ID_BUTPARENT,
    			GA_RelVerify,	TRUE,
    			GA_Text,	ifreq->ifr_ParentText,
    			TAG_END);
 
    if (!udata->ParentBut)
   	goto bye;
	

    			
    	
    udata->CancelBut = NewObject(NULL, FRBUTTONCLASS,
    			GA_Image,	udata->ButFrame,
    			GA_Previous,	udata->ParentBut,
    			GA_ID, 		ID_BUTCANCEL,
    			GA_RelVerify,	TRUE,
    			GA_Text,	GetIR(ifreq)->ir_NegativeText,
    			TAG_END);
    if (!udata->CancelBut)
    	goto bye;
    	
    

    butstr[0] = GetIR(ifreq)->ir_PositiveText;
    butstr[1] =	ifreq->ifr_VolumesText;    
    butstr[2] =	ifreq->ifr_ParentText;
    butstr[3] =	GetIR(ifreq)->ir_NegativeText;

    udata->ButWidth = BiggestTextLength(butstr,
    					NUMBUTS,
    					&(ld->ld_DummyRP),
    					ASLB(AslBase));
    udata->ButHeight = ld->ld_Font->tf_YSize + 6;    					
    					
    				
    /* The bottom buttonrow's widths are most significant 
    ** to the window's minimal width 
    */

    ld->ld_MinWidth =  ld->ld_Screen->WBorLeft + ld->ld_Screen->WBorRight
    		     + MIN_SPACING * (NUMBUTS + 1)
    		     + udata->ButWidth * NUMBUTS;
    		     
    ld->ld_MinHeight = ld->ld_Screen->WBorTop + ld->ld_Screen->Font->ta_YSize + 1
    		  + MIN_SPACING * 3
    		  + udata->ButHeight * 3;
    		   
		
    success = TRUE;

bye:

    if (!success)
    {
    	FRGadCleanup(ld, ASLB(AslBase));
    }
    	
    ReturnBool ("FRGadInit", success);	
	
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
    	{GA_RelBottom, 	0},
    	{GA_Width, 	0},
    	{GA_Height,	0},
    	{TAG_END}};
    	

    D(bug("FRGadLayout(ld=%p)\n", ld));
	
    udata   = (struct FRUserData *)ld->ld_UserData;
    win	    = ld->ld_Window;
	

    butrelbottom = - (win->BorderBottom + MIN_SPACING + udata->ButHeight);
    left = win->BorderLeft + MIN_SPACING;
    
    if (!(udata->Flags & FRFLG_LAYOUTED))
    {
    	SetAttrs(udata->Listview,
    		GA_Left,	left,
    		GA_Top,		win->BorderTop  + MIN_SPACING,
    		GA_RelWidth,	- (win->BorderRight  + 3 * MIN_SPACING + DEF_PROPWIDTH),
    		GA_RelHeight,	butrelbottom - MIN_SPACING - 2,
    		TAG_END);
    		    	
    	/* Need to SetGadgetAttrs this if notification should work
    	** (needs GadgetInfo)
    	*/		
    	SetGadgetAttrs((struct Gadget *)udata->Listview, ld->ld_Window, NULL,
    				AROSA_Listview_List,	udata->FileList,
    				TAG_END);


    	SetAttrs(udata->Prop,
    		GA_RelRight,	- (win->BorderRight  + MIN_SPACING + DEF_PROPWIDTH),
    		GA_Top,		win->BorderTop  + MIN_SPACING,
    		GA_Width,	DEF_PROPWIDTH,
    		GA_RelHeight,	butrelbottom - MIN_SPACING - 2,
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

#undef Exit
#define Exit(ret) {retval = ret; break;}
STATIC ULONG FRHandleEvents(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct IntuiMessage *imsg;
    ULONG retval =GHRET_OK;
    struct FRUserData *udata = (struct FRUserData *)ld->ld_UserData;


    imsg = ld->ld_Event;
D(bug("FRHandleEvents: Class: %d\n", imsg->Class));        
    switch (imsg->Class)
    {
    	case IDCMP_GADGETUP:

D(bug("GADGETUP! gadgetid=%d\n", ((struct Gadget *)imsg->IAddress)->GadgetID));

    	    switch (((struct Gadget *)imsg->IAddress)->GadgetID)
    	    {
	    	case ID_BUTCANCEL:
		    retval = FALSE;
		    break;
		    
		case ID_LISTVIEW:
		{
		    ULONG doubleclick;
		    
		    BOOL changedir = FALSE;

D(bug("ID_LISTVIEW event\n"));
		    GetAttr(AROSA_Listview_DoubleClick, udata->Listview, &doubleclick);
		    
		    if (doubleclick + 1)
		    {
		    	Object *list;
	    	        LONG active;
	    	        
	    	        #define GetEAD(e) ((struct ExAllData  *)e)
	    	        #define GetVI(e)  ((struct VolumeInfo *)e)
	    	        APTR entry;
D(bug("\tDoubleclick "));
	    	        list = ((udata->Flags & FRFLG_FILELIST)
	    	        		? udata->FileList : udata->VolumesList);


		    	
		    	/* Get the active entry */
		    	GetAttr(AROSA_List_Active, list, &active);
		    	if (active == AROSV_List_Active_None)
		    	    Exit(GHRET_OK);
D(bug("on entry %d\n", active));		    	    
		    	DoMethod(list, AROSM_List_GetEntry, active, &entry);
		    	
		    	if (udata->Flags & FRFLG_FILELIST)
		    	{

		    	    if (GetEAD(entry)->ed_Type > 0)
		    	    {
D(bug("Directory names. last: %s clicked: %s\n", udata->CurPath, GetEAD(entry)->ed_Name));
		    	    	if (!AddToPath(	GetEAD(entry)->ed_Name,
		    	    			&(udata->CurPath),
		    	    			&(udata->PathBufSize),
		    	    			ASLB(AslBase)))
		    	    	   Exit(FALSE);
		    	    	   
D(bug("\tNew curpath: %s\n", udata->CurPath));		    	    			    	    	
		    	    	changedir = TRUE;
		    	    } /* Clicked entry is a directory */
		    	    

		    	}
		    	else
		    	{
		    	    udata->CurPath[0] = 0;
		    	    
		    	    if (!AddToPath(	GetVI(entry)->vi_Name, 
		    	    			&(udata->CurPath),
		    	    			&(udata->PathBufSize),
		    	    			ASLB(AslBase)))
		    	    	Exit(FALSE);

		    	    changedir = TRUE;
		    	} /* Volme list is shown */
		    	
		    	if (changedir)
		    	{
		    	    UpdateFileList(udata, ld->ld_Window, ASLB(AslBase));
		    	}
		    	    
		    }
		
		} break;
		
		case ID_BUTVOLUMES:
		{
		    RemoveGList(ld->ld_Window, (struct Gadget *)udata->Listview, 1);

		    SetGadgetAttrs((struct Gadget *)udata->Listview, ld->ld_Window, NULL,
		    		AROSA_Listview_List, udata->VolumesList,
		    		AROSA_Listview_DisplayHook, &(udata->VLDisplayHook),
		    		AROSA_Listview_Format, "P=l",
		    		TAG_END);
		   	
		    udata->Flags &= ~FRFLG_FILELIST;
		    
		    AddGList(ld->ld_Window, (struct Gadget *)udata->Listview, -1, 1, NULL);

		    RefreshGList((struct Gadget *)udata->Listview, ld->ld_Window, NULL, 1);		    
		} break;
		
		case ID_BUTPARENT:
		{
		    /* Show parent directory */
		    if ( !AddToPath("/", 
		    		    &(udata->CurPath),
		    		    &(udata->PathBufSize),
		    		    ASLB(AslBase)))
		    {
		    	D(bug("Failed path: %s\n",udata->CurPath));		    
		 	Exit(FALSE);
		    }	
		    D(bug("OK path: %s\n", udata->CurPath));
		    if (!UpdateFileList(udata, ld->ld_Window, ASLB(AslBase)))
		    	Exit(FALSE);
		    
		}
		
		case ID_BUTOK:
		{
		    if (!(udata->Flags & FRFLG_FILELIST))
		    {
			break;
		    }
		    else
		    {
		    	struct IntFileReq *ifreq;
		    	struct FileRequester *req;
		    	
		    	ifreq = (struct IntFileReq *)ld->ld_IntReq;
		    	req = (struct FileRequester *)ld->ld_Req;
		    	
		    	if (!(ifreq->ifr_Flags1 & FRF_DOMULTISELECT))
		    	{
		    	    LONG active;
		    	    struct ExAllData *ead;
		    	    /* Only singleselect */
		    	    
		    	    GetAttr(AROSA_List_Active, udata->FileList, &active);
		    	    if (active == AROSV_List_Active_None)
		    	    	Exit(GHRET_OK);
		    	    	
		    	    DoMethod(udata->FileList, AROSM_List_GetEntry, active, &ead);
		    	    /* File selected ? */
		    	    if (ead->ed_Type > 0)
		    	    	Exit(GHRET_OK);
		    	    	
		    	    req->fr_Drawer = AllocVec(strlen(udata->CurPath) + 1, MEMF_ANY);
		    	    if (!req->fr_Drawer)
		    	    	Exit(FALSE);
		    	    	
		    	    req->fr_File = AllocVec(strlen(ead->ed_Name) + 1, MEMF_ANY);
		    	    if (!req->fr_File)
		    	    	Exit(FALSE);
		    	    	
		    	    strcpy(req->fr_Drawer, udata->CurPath);
		    	    strcpy(req->fr_File, ead->ed_Name);
		    	    
		    	}
		    	else
		    	{
		    	    LONG selcount;
		    	    struct WBArg *wbarg;
		    	    LONG id;
		    	    
		    	    /* Create a WBArg structure for the selected files */
		    	    
		    	    DoMethod(udata->FileList,
		    	    	AROSM_List_Select,
		    	    	AROSV_List_Select_All,
		    	    	AROSV_List_Select_Ask,
		    	    	&selcount);
		    	    	
		    	    #undef UB
		    	    #define UB(x) ((UBYTE *)x)
		    	    /* Allocate WBArg structures */
		    	    req->fr_ArgList = wbarg = AllocVec(
		    	    			UB(&wbarg[selcount]) - UB(&wbarg[0]),
		    	    			MEMF_ANY|MEMF_CLEAR);
			    if (!req->fr_ArgList)
			    	Exit(FALSE);
			    	
	    		    id = AROSV_List_NextSelected_Start;
	    		    
	    		    req->fr_NumArgs = selcount;
	    
	    		    for (;;)
	    		    {
	    		    	struct ExAllData *ead;
	    		    	
	    			DoMethod(udata->FileList, AROSM_List_NextSelected, &id);
	    			if (id == AROSV_List_NextSelected_End)
	    	    		    break;
	    	
	    			DoMethod(udata->FileList, AROSM_List_GetEntry, &ead);
	    			
	    			wbarg->wa_Name = AllocVec(strlen(ead->ed_Name) + 1, MEMF_ANY);
	    			if (!wbarg->wa_Name)
	    			    Exit(FALSE);
	    			    
	    			strcpy(wbarg->wa_Name, ead->ed_Name);
	    			
	    		    }
		    	    
		    	}
		    
		    }
		
		} break;
		
	} break;
	    
	    
    } /* switch (imsg->Class) */
    
    return (retval);

}

/*******************
**  FRGadCleanup  **
*******************/
STATIC VOID FRGadCleanup(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData *udata;
	
    D(bug("FRGadCleanup(ld=%p)\n", ld));
    udata = (struct FRUserData *)ld->ld_UserData;

D(bug("Freeing filelist\n"));
    if (udata->FileList)	
    	DisposeObject(udata->FileList);
    	
    if (udata->Listview)
    	DisposeObject(udata->Listview);

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
    	
D(bug("Freeing volumeslist\n"));
    if (udata->VolumesList)	
    	DisposeObject(udata->VolumesList);
D(bug("Freeing disphookbuf\n"));    	
    if (udata->DispHookBuf)
    	FreeMem(udata->DispHookBuf, DISPHOOKBUFSIZE);
D(bug("Freeing curpath\n"));    	
    if (udata->CurPath)
    	FreeMem(udata->CurPath, udata->PathBufSize);

    ReturnVoid("FRGadCleanup");
}



/****************
**  AddToPath  **
*****************/
STATIC BOOL AddToPath(STRPTR			path2add,
			STRPTR			*bufptr,
			ULONG			*lptr,
			struct AslBase_intern	*AslBase)
{
#undef SIZEINCREASE
#define SIZEINCREASE 100

    if (!*bufptr)
    {
    	*bufptr = AllocMem(SIZEINCREASE, MEMF_ANY);
    	if (!*bufptr)
    	    return (FALSE);
    	    
    	**bufptr = 0; /* null-terminate */
    	*lptr = SIZEINCREASE;
    }
    
    while (!AddPart(*bufptr, path2add, *lptr))
    {
    	STRPTR newbuf;
    	
    	newbuf = AllocMem(*lptr + SIZEINCREASE, MEMF_ANY);
    	if (!newbuf)
    	    return (FALSE);
    	  
    	strcpy(newbuf, *bufptr);
    	FreeMem(*bufptr, *lptr);
    	
    	*lptr += SIZEINCREASE;
    }
    
    return (TRUE);
    
}

/*******************
**  UpdateFileList  **
*******************/
STATIC BOOL UpdateFileList( struct FRUserData		*udata,
		    	    struct Window 		*w,
		   	    struct AslBase_intern	*AslBase)
{

    RemoveGList(w, (struct Gadget *)udata->Listview, 1);
    DoMethod(udata->FileList, AROSM_List_Clear);

    if (!GetDir(udata->CurPath, udata->FileList, ASLB(AslBase)))
	return (FALSE);

    SetGadgetAttrs((struct Gadget *)udata->Listview, w, NULL,
	AROSA_Listview_List, 	 udata->FileList,
	AROSA_Listview_Format, 	 "P=l, P=l",
	AROSA_Listview_DisplayHook, &(udata->FLDisplayHook),
	TAG_END);
		    	    
    udata->Flags |= FRFLG_FILELIST;
    
    AddGList(w, (struct Gadget *)udata->Listview, -1, 1, NULL);
    RefreshGList((struct Gadget *)udata->Listview, w, NULL, 1);
				
    return (TRUE);
}		    
