/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ASL initialization code.
*/


#include <stddef.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <intuition/screens.h> 
#include <graphics/modeid.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "asl_intern.h"
#include LC_LIBDEFS_FILE

#define CATCOMP_NUMBERS
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************************/

/* Requester type specific default data */
const struct IntFileReq def_filereq =
{
    {
	ASL_FileRequest,
	NULL,			/* Window		*/
	NULL,			/* Screen		*/
	NULL,			/* PubScreenName	*/
	NULL,			/* IntuiMsgFunc 	*/
	NULL,			/* TextAttr		*/
	NULL,			/* Locale		*/
	NULL,			/* Catalog		*/
	NULL,			/* MemPool		*/
	2048,			/* MemPoolPuddle	*/
	2048,			/* MemPoolThresh	*/
	MSG_FILEREQ_TITLE,   	/* TitleID  	    	*/
	NULL,		    	/* TitleText		*/
	NULL,			/* PositiveText		*/
	NULL,			/* NegativeText		*/
	-1, -1,	 		/* --> center on screen */
	300, 300		/* Width/Height		*/
    },

    "",				/* File 	 	*/
    "",     			/* Drawer        	*/
    "#?",       		/* Pattern       	*/
    NULL,			/* AcceptPattern 	*/ /* def. = "#?", but must be ParsePatternNoCase'ed */
    NULL,			/* RejectPattern 	*/ /* def. = "~(#?)", but must be ParsePatternNoCase'ed */
    0,				/* Flags1	 	*/
    FRF_REJECTICONS,		/* Flags2	 	*/
    NULL,			/* FilterFunc	 	*/
    NULL,			/* HookFunc	 	*/
    NULL,			/* GetSortBy	 	*/
    NULL,			/* GetSortOrder  	*/
    NULL,			/* GetSortDrawers	*/
    ASLFRSORTBY_Name,   	/* SortBy        	*/
    ASLFRSORTORDER_Ascend,	/* SortOrder     	*/
    ASLFRSORTDRAWERS_First,	/* SortDrawers   	*/
    FALSE			/* InitialShowVolumes 	*/
};

/*****************************************************************************************/

const struct IntSMReq def_smreq =
{
    {
	ASL_ScreenModeRequest,
	NULL,				/* Window		*/
	NULL,				/* Screen		*/
	NULL,				/* PubScreenName	*/
	NULL,				/* IntuiMsgFunc 	*/
	NULL,				/* TextAttr		*/
	NULL,				/* Locale		*/
	NULL,				/* Catalog		*/
	NULL,				/* MemPool		*/
	2048,				/* MemPoolPuddle	*/
	2048,				/* MemPoolThresh	*/
	MSG_MODEREQ_TITLE,   	    	/* TitleID  	    	*/
	NULL,		    	    	/* TitleText		*/
	NULL,				/* PositiveText		*/
	NULL,			    	/* NegativeText		*/
	-1, -1,	 			/* --> center on screen */
	300, 300			/* Width/Height		*/
    },

    NULL,				/* CustomSMList 	*/
    NULL,				/* FilterFunc 		*/
    0,					/* Flags 		*/
    LORES_KEY,				/* DisplayID 		*/
    640,				/* DisplayWidth 	*/
    200,				/* DisplayHeight 	*/
    640,				/* BitMapWidth 		*/
    200,				/* BitMapHeight 	*/
    2,					/* DisplayDepth 	*/
    OSCAN_TEXT,				/* OverscanType 	*/
    TRUE,				/* AutoScroll 		*/
    DIPF_IS_WB,				/* PropertyFlags 	*/
    DIPF_IS_WB,				/* PropertyMask 	*/
    1,					/* MinDepth 		*/
    24,					/* MaxDepth 		*/
    16,					/* MinWidth 		*/
    16384,				/* MaxWidth 		*/
    16,					/* MinHeight 		*/
    16384,				/* MaxHeight 		*/
    20,					/* InfoLeftEdge 	*/
    20,					/* InfoTopEdge 		*/
    FALSE				/* InfoOpened 		*/        
};

/*****************************************************************************************/

const struct IntFontReq def_fontreq =
{
    {
	ASL_FontRequest,
	NULL,				/* Window		*/
	NULL,				/* Screen		*/
	NULL,				/* PubScreenName	*/
	NULL,				/* IntuiMsgFunc 	*/
	NULL,				/* TextAttr		*/
	NULL,				/* Locale		*/
	NULL,				/* Catalog		*/
	NULL,				/* MemPool		*/
	2048,				/* MemPoolPuddle	*/
	2048,				/* MemPoolThresh	*/
	MSG_FONTREQ_TITLE,   	    	/* TitleID  	    	*/
	NULL,			    	/* TitleText		*/
	NULL,				/* PositiveText		*/
	NULL,			    	/* NegativeText		*/
	-1, -1,				/* --> center on screen */
	300, 300			/* Width/Height		*/
    },
    {"topaz", 8, FS_NORMAL,FPF_ROMFONT},/* Default textattr 	*/
    1,				    	/* FrontPen		*/
    0,			    	    	/* BackPen		*/
    JAM1,				/* DrawMode		*/
    0,					/* Flags		*/

    5,					/* Minheight		*/
    24,				    	/* MaxHeight		*/
    NULL,			    	/* FilterFunc		*/
    NULL,				/* HookFunc		*/
    32, 				/* MaxFrontPen		*/
    32, 				/* MaxBackPen		*/

    NULL,				/* ModeList		*/
    NULL,				/* FrontPens		*/
    NULL,				/* BackPens		*/
    
};

/* coolimages may fail to open */
LONG CoolImagesBase_version = -1;

/*****************************************************************************************/

VOID InitReqInfo(struct AslBase_intern *);

/*****************************************************************************************/

AROS_SET_LIBFUNC(InitBase, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    D(bug("Inside InitBase of asl.library\n"));

    D(bug("SysBase: %p lh_SysBase %p\n", SysBase, LIBBASE->lh.lh_SysBase));
    
    NEWLIST(&LIBBASE->ReqList);

    InitSemaphore(&LIBBASE->ReqListSem);

    InitReqInfo(LIBBASE);

    if (!LIBBASE->aslpropclass)
        LIBBASE->aslpropclass = makeaslpropclass(LIBBASE);
    if (!LIBBASE->aslpropclass)
        return FALSE;

    if (!LIBBASE->aslarrowclass)
        LIBBASE->aslarrowclass = makeaslarrowclass(LIBBASE);
    if (!LIBBASE->aslarrowclass)
        return FALSE;
	
    if (!LIBBASE->asllistviewclass)
        LIBBASE->asllistviewclass = makeasllistviewclass(LIBBASE);
    if (!LIBBASE->asllistviewclass)
        return FALSE;

    if (!LIBBASE->aslbuttonclass)
        LIBBASE->aslbuttonclass = makeaslbuttonclass(LIBBASE);
    if (!LIBBASE->aslbuttonclass)
        return FALSE;

    if (!LIBBASE->aslstringclass)
        LIBBASE->aslstringclass = makeaslstringclass(LIBBASE);
    if (!LIBBASE->aslstringclass)
        return FALSE;

    if (!LIBBASE->aslcycleclass)
        LIBBASE->aslcycleclass = makeaslcycleclass(LIBBASE);
    if (!LIBBASE->aslcycleclass)
        return FALSE;

    if (!LIBBASE->aslfontpreviewclass)
        LIBBASE->aslfontpreviewclass = makeaslfontpreviewclass(LIBBASE);
    if (!LIBBASE->aslfontpreviewclass)
        return FALSE;

    if (!LIBBASE->aslfontstyleclass)
        LIBBASE->aslfontstyleclass = makeaslfontstyleclass(LIBBASE);
    if (!LIBBASE->aslfontstyleclass)
        return FALSE;
 
    if (!LIBBASE->aslcolorpickerclass)
        LIBBASE->aslcolorpickerclass = makeaslcolorpickerclass(LIBBASE);
    if (!LIBBASE->aslcolorpickerclass)
        return FALSE;

    if (!LIBBASE->asleraserclass)
        LIBBASE->asleraserclass = makeasleraserclass(LIBBASE);
    if (!LIBBASE->asleraserclass)
        return FALSE;

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

/*****************************************************************************************/

AROS_SET_LIBFUNC(CleanUp, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT;
    
    if (LIBBASE->aslpropclass)
    {
	FreeClass(LIBBASE->aslpropclass);
	LIBBASE->aslpropclass = NULL;
    }

    if (LIBBASE->aslarrowclass)
    {
	FreeClass(LIBBASE->aslarrowclass);
	LIBBASE->aslarrowclass = NULL;
    }
	
    if (LIBBASE->asllistviewclass)
    {
	FreeClass(LIBBASE->asllistviewclass);
	LIBBASE->asllistviewclass = NULL;
    }
	
    if (LIBBASE->aslbuttonclass)
    {
	FreeClass(LIBBASE->aslbuttonclass);
	LIBBASE->aslbuttonclass = NULL;
    }

    if (LIBBASE->aslstringclass)
    {
	FreeClass(LIBBASE->aslstringclass);
	LIBBASE->aslstringclass = NULL;
    }

    if (LIBBASE->aslcycleclass)
    {
	FreeClass(LIBBASE->aslcycleclass);
	LIBBASE->aslcycleclass = NULL;
    }

    if (LIBBASE->aslfontpreviewclass)
    {
	FreeClass(LIBBASE->aslfontpreviewclass);
	LIBBASE->aslfontpreviewclass = NULL;
    }

    if (LIBBASE->aslfontstyleclass)
    {
	FreeClass(LIBBASE->aslfontstyleclass);
	LIBBASE->aslfontstyleclass = NULL;
    }

    if (LIBBASE->aslcolorpickerclass)
    {
	FreeClass(LIBBASE->aslcolorpickerclass);
	LIBBASE->aslcolorpickerclass = NULL;
    }

    if (LIBBASE->asleraserclass)
    {
	FreeClass(LIBBASE->asleraserclass);
	LIBBASE->asleraserclass = NULL;
    }
	
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT;
}

ADD2INITLIB(InitBase, 0);
ADD2EXPUNGELIB(CleanUp, 0);

/*****************************************************************************************/

#include <string.h>
#include "filereqhooks.h"
#include "fontreqhooks.h"
#include "modereqhooks.h"

/*****************************************************************************************/

VOID InitReqInfo(struct AslBase_intern *AslBase)
{
    struct AslReqInfo *reqinfo;

    /* Set file requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_FileRequest]);
    D(bug("AslBase: %p reqinfo: %p\n", AslBase, reqinfo));
    reqinfo->IntReqSize 	= sizeof (struct IntFileReq);
    reqinfo->ReqSize		= sizeof (struct FileRequester);
    reqinfo->DefaultReq 	= (struct IntFileReq *)&def_filereq;
    reqinfo->UserDataSize	= sizeof (struct FRUserData);

    bzero(&(reqinfo->ParseTagsHook), sizeof (struct Hook));
    bzero(&(reqinfo->GadgetryHook), sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry	= (void *)AROS_ASMSYMNAME(FRTagHook);
    reqinfo->GadgetryHook.h_Entry	= (void *)AROS_ASMSYMNAME(FRGadgetryHook);

    /* Set font requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_FontRequest]);
    reqinfo->IntReqSize 	= sizeof (struct IntFontReq);
    reqinfo->ReqSize		= sizeof (struct FontRequester);
    reqinfo->DefaultReq 	= (struct IntFontReq *)&def_fontreq;
    reqinfo->UserDataSize	= sizeof (struct FOUserData);

    bzero(&(reqinfo->ParseTagsHook), sizeof (struct Hook));
    bzero(&(reqinfo->GadgetryHook), sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry	= (void *)AROS_ASMSYMNAME(FOTagHook);
    reqinfo->GadgetryHook.h_Entry	= (void *)AROS_ASMSYMNAME(FOGadgetryHook);

    /* Set screenmode requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_ScreenModeRequest]);
    reqinfo->IntReqSize 	= sizeof (struct IntSMReq);
    reqinfo->ReqSize		= sizeof (struct ScreenModeRequester);
    reqinfo->DefaultReq 	= (struct IntSMReq *)&def_smreq;
    reqinfo->UserDataSize	= sizeof(struct SMUserData);

    bzero(&(reqinfo->ParseTagsHook), sizeof (struct Hook));
    bzero(&(reqinfo->GadgetryHook), sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry	= (void *)AROS_ASMSYMNAME(SMTagHook);
    reqinfo->GadgetryHook.h_Entry	= (void *)AROS_ASMSYMNAME(SMGadgetryHook);

    return;
}

/*****************************************************************************************/
