#ifndef LIBRARIES_ASL_H
#define LIBRARIES_ASL_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef WORKBENCH_STARTUP_H
#   include <workbench/startup.h>
#endif

#ifndef GRAPHICS_TEXT_H
#   include <graphics/text.h>
#endif

#ifndef GRAPHICS_DISPLAYINFO_H
#   include <graphics/displayinfo.h>
#endif

/***************************************/

#define AslName "asl.library"

#define ASL_TB (TAG_USER + 0x80000)


/* Requester types */
#define ASL_FileRequest 	0
#define ASL_FontRequest 	1
#define ASL_ScreenModeRequest	2

/* Filerequester structure */

struct FileRequester
{
    UBYTE		fr_Reserved0[4];
    STRPTR		fr_File;		/* User selected file */
    STRPTR		fr_Drawer;		/* User selected drawer */
    UBYTE		fr_Reserved1[10];
    WORD		fr_LeftEdge;		/* Requester size/placement on exit */
    WORD		fr_TopEdge;
    WORD		fr_Width;
    WORD		fr_Height;
    UBYTE		fr_Reserved2[2];
    LONG		fr_NumArgs;		/* How many files were selected ? */
    struct WBArg 	*fr_ArgList;		/* Selected files */
    APTR		fr_UserData;
    UBYTE		fr_Reserved3[8];
    STRPTR		fr_Pattern;		/* Pattern on exit */
};


/* Filerequester tags */

#define ASLFR_Window			ASL_TB + 2
#define ASLFR_Screen			ASL_TB + 40
#define ASLFR_PubScreenName		ASL_TB + 41
#define ASLFR_PrivateIDCMP		ASL_TB + 42
#define ASLFR_IntuiMsgFunc		ASL_TB + 70
#define ASLFR_SleepWindow		ASL_TB + 43
#define ASLFR_UserData			ASL_TB + 52
#define ASLFR_PopToFront		ASL_TB + 131
#define ASLFR_Activate	    	    	ASL_TB + 132

#define ASLFR_TextAttr			ASL_TB + 51
#define ASLFR_Locale			ASL_TB + 50
#define ASLFR_TitleText 		ASL_TB + 1
#define ASLFR_PositiveText		ASL_TB + 18
#define ASLFR_NegativeText		ASL_TB + 19

#define ASLFR_InitialLeftEdge		ASL_TB + 3
#define ASLFR_InitialTopEdge		ASL_TB + 4
#define ASLFR_InitialWidth		ASL_TB + 5
#define ASLFR_InitialHeight		ASL_TB + 6
#define ASLFR_InitialFile		ASL_TB + 8
#define ASLFR_InitialDrawer		ASL_TB + 9
#define ASLFR_InitialPattern		ASL_TB + 10
#define ASLFR_InitialShowVolumes 	ASL_TB + 130

#define ASLFR_Flags1			ASL_TB + 20
#define ASLFR_Flags2			ASL_TB + 22
#define ASLFR_DoSaveMode		ASL_TB + 44
#define ASLFR_DoMultiSelect		ASL_TB + 45
#define ASLFR_DoPatterns		ASL_TB + 46

#define ASLFR_DrawersOnly		ASL_TB + 47
#define ASLFR_FilterFunc		ASL_TB + 49
#define ASLFR_RejectIcons		ASL_TB + 60
#define ASLFR_RejectPattern		ASL_TB + 61
#define ASLFR_AcceptPattern		ASL_TB + 62
#define ASLFR_FilterDrawers		ASL_TB + 63
#define ASLFR_HookFunc			ASL_TB + 7

#define ASLFR_SetSortBy			ASL_TB + 124
#define ASLFR_GetSortBy			ASL_TB + 125
#define ASLFR_SetSortDrawers		ASL_TB + 126
#define ASLFR_GetSortDrawers		ASL_TB + 127
#define ASLFR_SetSortOrder		ASL_TB + 128
#define ASLFR_GetSortOrder		ASL_TB + 129

/* Flags for ASLFR_Flags1 */

#define FRB_FILTERFUNC			7
#define FRB_INTUIFUNC			6
#define FRB_DOSAVEMODE			5
#define FRB_PRIVATEIDCMP		4
#define FRB_DOMULTISELECT		3
#define FRB_DOPATTERNS			0

#define FRF_FILTERFUNC			(1L << FRB_FILTERFUNC)
#define FRF_INTUIFUNC			(1L << FRB_INTUIFUNC)
#define FRF_DOSAVEMODE			(1L << FRB_DOSAVEMODE)
#define FRF_PRIVATEIDCMP		(1L << FRB_PRIVATEIDCMP)
#define FRF_DOMULTISELECT		(1L << FRB_DOMULTISELECT)
#define FRF_DOPATTERNS			(1L << FRB_DOPATTERNS)

/* Flags for ASLSM_Flags2 */

#define FRB_DRAWERSONLY 		0
#define FRB_FILTERDRAWERS		1
#define FRB_REJECTICONS 		2

#define FRF_DRAWERSONLY 		(1L << FRB_DRAWERSONLY)
#define FRF_FILTERDRAWERS		(1L << FRB_FILTERDRAWERS)
#define FRF_REJECTICONS 		(1L << FRB_REJECTICONS)

/* Sort criteria for ASLFR_SetSortBy/ASLFR_GetSortBy tags */

#define ASLFRSORTBY_Name		0
#define ASLFRSORTBY_Date		1
#define ASLFRSORTBY_Size		2

/* Drawer placement for ASLFR_SetSortDrawers/ASLFR_GetSortDrawers tags */

#define ASLFRSORTDRAWERS_First		0
#define ASLFRSORTDRAWERS_Mix		1
#define ASLFRSORTDRAWERS_Last		2

/* Sort order for the ASLFR_SetSortOrder/ASLFR_GetSortOrder tags */

#define ASLFRSORTORDER_Ascend		0
#define ASLFRSORTORDER_Descend		1

/* Fontrequester structure */
struct FontRequester
{
    UBYTE		fo_Reserved[8];
    struct TextAttr	fo_Attr;
    UBYTE		fo_FrontPen;
    UBYTE		fo_BackPen;
    UBYTE		fo_DrawMode;
    UBYTE		fo_Reserved1;
    APTR		fo_UserData;
    WORD		fo_LeftEdge;
    WORD		fo_TopEdge;
    WORD		fo_Width;
    WORD		fo_Height;
    struct TTextAttr	fo_TAttr;
};

/* Fontrequester tags */

#define ASLFO_Window			ASL_TB + 2
#define ASLFO_Screen			ASL_TB + 40
#define ASLFO_PubScreenName		ASL_TB + 41
#define ASLFO_PrivateIDCMP		ASL_TB + 42
#define ASLFO_IntuiMsgFunc		ASL_TB + 70
#define ASLFO_SleepWindow		ASL_TB + 43
#define ASLFO_UserData			ASL_TB + 52
#define ASLFO_PopToFront		ASL_TB + 131
#define ASLFO_Activate	    	    	ASL_TB + 132

#define ASLFO_TextAttr			ASL_TB + 51
#define ASLFO_Locale			ASL_TB + 50
#define ASLFO_TitleText 		ASL_TB + 1
#define ASLFO_PositiveText		ASL_TB + 18
#define ASLFO_NegativeText		ASL_TB + 19

#define ASLFO_InitialLeftEdge		ASL_TB + 3
#define ASLFO_InitialTopEdge		ASL_TB + 4
#define ASLFO_InitialWidth		ASL_TB + 5
#define ASLFO_InitialHeight		ASL_TB + 6
#define ASLFO_InitialName		ASL_TB + 10
#define ASLFO_InitialSize		ASL_TB + 11
#define ASLFO_InitialStyle		ASL_TB + 12
#define ASLFO_InitialFlags		ASL_TB + 13
#define ASLFO_InitialFrontPen		ASL_TB + 14
#define ASLFO_InitialBackPen		ASL_TB + 15
#define ASLFO_InitialDrawMode		ASL_TB + 59

#define ASLFO_Flags			ASL_TB + 20
#define ASLFO_DoFrontPen		ASL_TB + 44
#define ASLFO_DoBackPen 		ASL_TB + 45
#define ASLFO_DoStyle			ASL_TB + 46
#define ASLFO_DoDrawMode		ASL_TB + 47
#define ASLFO_SampleText    	    	ASL_TB + 133

#define ASLFO_FixedWidthOnly		ASL_TB + 48
#define ASLFO_MinHeight 		ASL_TB + 16
#define ASLFO_MaxHeight 		ASL_TB + 17
#define ASLFO_FilterFunc		ASL_TB + 49
#define ASLFO_HookFunc			ASL_TB + 7
#define ASLFO_MaxFrontPen		ASL_TB + 66
#define ASLFO_MaxBackPen		ASL_TB + 67

#define ASLFO_ModeList			ASL_TB + 21
#define ASLFO_FrontPens 		ASL_TB + 64
#define ASLFO_BackPens			ASL_TB + 65

/* Flags for ASLFO_Flags */

#define FOB_DOFRONTPEN			0
#define FOB_DOBACKPEN			1
#define FOB_DOSTYLE			2
#define FOB_DODRAWMODE			3
#define FOB_FIXEDWIDTHONLY		4
#define FOB_PRIVATEIDCMP		5
#define FOB_INTUIFUNC			6
#define FOB_FILTERFUNC			7

#define FOF_DOFRONTPEN			(1L << FOB_DOFRONTPEN)
#define FOF_DOBACKPEN			(1L << FOB_DOBACKPEN)
#define FOF_DOSTYLE			(1L << FOB_DOSTYLE)
#define FOF_DODRAWMODE			(1L << FOB_DODRAWMODE)
#define FOF_FIXEDWIDTHONLY		(1L << FOB_FIXEDWIDTHONLY)
#define FOF_PRIVATEIDCMP		(1L << FOB_PRIVATEIDCMP)
#define FOF_INTUIFUNC			(1L << FOB_INTUIFUNC)
#define FOF_FILTERFUNC			(1L << FOB_FILTERFUNC)


/* Screenmoderequester structure */

struct ScreenModeRequester
{
    ULONG	sm_DisplayID;
    ULONG	sm_DisplayWidth;
    ULONG	sm_DisplayHeight;
    UWORD	sm_DisplayDepth;
    UWORD	sm_OverscanType;
    BOOL	sm_AutoScroll;

    ULONG	sm_BitMapWidth;
    ULONG	sm_BitMapHeight;

    WORD	sm_LeftEdge;
    WORD	sm_TopEdge;
    WORD	sm_Width;
    WORD	sm_Height;

    BOOL	sm_InfoOpened;
    WORD	sm_InfoLeftEdge;
    WORD	sm_InfoTopEdge;
    WORD	sm_InfoWidth;
    WORD	sm_InfoHeight;

    APTR	sm_UserData;

};

/* Structure for adding custom display modes to the requester list */
struct DisplayMode
{
    struct Node 	 dm_Node;
    struct DimensionInfo dm_DimensionInfo;
    ULONG		 dm_PropertyFlags;

};

/* ScreenModerequester tags */

#define ASLSM_Window			ASL_TB + 2
#define ASLSM_Screen			ASL_TB + 40
#define ASLSM_PubScreenName		ASL_TB + 41
#define ASLSM_PrivateIDCMP		ASL_TB + 42
#define ASLSM_IntuiMsgFunc		ASL_TB + 70
#define ASLSM_SleepWindow		ASL_TB + 43
#define ASLSM_UserData			ASL_TB + 52
#define ASLSM_PopToFront		ASL_TB + 131
#define ASLSM_Activate	    	    	ASL_TB + 132

#define ASLSM_TextAttr			ASL_TB + 51
#define ASLSM_Locale			ASL_TB + 50
#define ASLSM_TitleText 		ASL_TB + 1
#define ASLSM_PositiveText		ASL_TB + 18
#define ASLSM_NegativeText		ASL_TB + 19

#define ASLSM_InitialLeftEdge		ASL_TB + 3
#define ASLSM_InitialTopEdge		ASL_TB + 4
#define ASLSM_InitialWidth		ASL_TB + 5
#define ASLSM_InitialHeight		ASL_TB + 6

#define ASLSM_InitialDisplayID		ASL_TB + 100
#define ASLSM_InitialDisplayWidth	ASL_TB + 101
#define ASLSM_InitialDisplayHeight	ASL_TB + 102
#define ASLSM_InitialDisplayDepth	ASL_TB + 103
#define ASLSM_InitialOverscanType	ASL_TB + 104
#define ASLSM_InitialAutoScroll 	ASL_TB + 105
#define ASLSM_InitialInfoOpened 	ASL_TB + 106
#define ASLSM_InitialInfoLeftEdge	ASL_TB + 107
#define ASLSM_InitialInfoTopEdge	ASL_TB + 108

#define ASLSM_DoWidth			ASL_TB + 109
#define ASLSM_DoHeight			ASL_TB + 110
#define ASLSM_DoDepth			ASL_TB + 111
#define ASLSM_DoOverscanType		ASL_TB + 112
#define ASLSM_DoAutoScroll		ASL_TB + 113

#define ASLSM_PropertyFlags		ASL_TB + 114
#define ASLSM_PropertyMask		ASL_TB + 115
#define ASLSM_MinWidth			ASL_TB + 116
#define ASLSM_MaxWidth			ASL_TB + 117
#define ASLSM_MinHeight 		ASL_TB + 118
#define ASLSM_MaxHeight 		ASL_TB + 119
#define ASLSM_MinDepth			ASL_TB + 120
#define ASLSM_MaxDepth			ASL_TB + 121
#define ASLSM_FilterFunc 		ASL_TB + 122

#define ASLSM_CustomSMList		ASL_TB + 123

/****************************/

#define ASL_LAST_TAG			ASL_TB + 131

/****************************/

/* Obsolete pre-V38 ASL defienitions. Only here for source
compatibility. DO NOT USE iN NEW CODE.

To make sure you not use them, #define ASL_V38_NAMES_ONLY

*/

#ifndef ASL_V38_NAMES_ONLY

/* !!! DO NOT USE !!! */

#define rf_File 	fr_File
#define rf_Dir		fr_Drawer
#define rf_LeftEdge	fr_LeftEdge
#define rf_TopEdge	fr_TopEdge
#define rf_Width	fr_Width
#define rf_Height	fr_Height
#define rf_NumArgs	fr_NumArgs
#define rf_ArgList	fr_ArgList
#define rf_UserData	fr_UserData
#define rf_Pat		fr_Pattern

/* !!! DO NOT USE !!! */

#define ASL_Dummy	(TAG_USER + 0x80000)
#define ASL_Hail	ASL_Dummy + 1
#define ASL_Window	ASL_Dummy + 2
#define ASL_LeftEdge	ASL_Dummy + 3
#define ASL_TopEdge	ASL_Dummy + 4
#define ASL_Width	ASL_Dummy + 5
#define ASL_Height	ASL_Dummy + 6
#define ASL_HookFunc	ASL_Dummy + 7
#define ASL_File	ASL_Dummy + 8
#define ASL_Dir 	ASL_Dummy + 9
#define ASL_FontName	ASL_Dummy + 10
#define ASL_FontHeight	ASL_Dummy + 11
#define ASL_FontStyles	ASL_Dummy + 12
#define ASL_FontFlags	ASL_Dummy + 13
#define ASL_FrontPen	ASL_Dummy + 14
#define ASL_BackPen	ASL_Dummy + 15
#define ASL_MinHeight	ASL_Dummy + 16
#define ASL_MaxHeight	ASL_Dummy + 17
#define ASL_OKText	ASL_Dummy + 18
#define ASL_CancelText	ASL_Dummy + 19
#define ASL_FuncFlags	ASL_Dummy + 20
#define ASL_ModeList	ASL_Dummy + 21
#define ASL_ExtFlags1	ASL_Dummy + 22
#define ASL_Pattern	ASL_FontName

/* !!! DO NOT USE !!! */

#define FILB_DOWILDFUNC 	7L
#define FILB_DOMSGFUNC		6L
#define FILB_SAVE		5L
#define FILB_NEWIDCMP		4L
#define FILB_MULTISELECT	3L
#define FILB_PATGAD		0L

#define FILF_DOWILDFUNC 	(1L << FILB_DOWILDFUNC)
#define FILF_DOMSGFUNC		(1L << FILB_DOMSGFUNC)
#define FILF_SAVE		(1L << FILB_SAVE)
#define FILF_NEWIDCMP		(1L << FILB_NEWIDCMP)
#define FILF_MULTISELECT	(1L << FILB_MULTISELECT)
#define FILF_PATGAD		(1L << FILB_PATGAD)
#define FIL1B_NOFILES	0L
#define FIL1B_MATCHDIRS 1L
#define FIL1F_NOFILES	(1L << FIL1B_NOFILES)
#define FIL1F_MATCHDIRS (1L << FIL1B_MATCHDIRS)
#define FONB_FRONTCOLOR 0L
#define FONB_BACKCOLOR	1L
#define FONB_STYLES	2L
#define FONB_DRAWMODE	3L
#define FONB_FIXEDWIDTH 4L
#define FONB_NEWIDCMP	5L
#define FONB_DOMSGFUNC	6L
#define FONB_WILDFUNC	7L
#define FONF_FRONTCOLOR (1L << FONB_FRONTCOLOR)
#define FONF_BACKCOLOR	(1L << FONB_BACKCOLOR)
#define FONF_STYLES	(1L << FONB_STYLES)
#define FONF_DRAWMODE	(1L << FONB_DRAWMODE)
#define FONF_FIXEDWIDTH (1L << FONB_FIXEDWIDTH)
#define FONF_NEWIDCMP	(1L << FONB_NEWIDCMP)
#define FONF_DOMSGFUNC	(1L << FONB_DOMSGFUNC)
#define FONF_DOWILDFUNC (1L << FONB_DOWILDFUNC)

#endif /* ASL_V38_NAMES_ONLY */

/*************************/

#endif /* LIBRARIES_ASL_H */
