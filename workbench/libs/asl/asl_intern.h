/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English.
*/

#ifndef ASL_INTERN_H
#define ASL_INTERN_H



#ifndef EXEC_TYPES_H
#    include <exec/types.h>
#endif

#ifndef EXEC_LIBRARIES_H
#    include <exec/libraries.h>
#endif

#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif

#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif

#ifndef INTUITION_SGHOOKS_H
#   include <intuition/sghooks.h>
#endif

#ifndef LIBRARIES_ASL_H
#    include <libraries/asl.h>
#endif

#ifndef LIBRARIES_LOCALE_H
#    include <libraries/locale.h>
#endif

#ifndef UTILITY_HOOKS_H
#    include <utility/hooks.h>
#endif

#ifndef LAYOUT_H
#    include "layout.h"
#endif

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#include <aros/libcall.h>

#include <libcore/base.h>

/*****************************************************************************************/

#ifdef __MORPHOS__
#define USE_SHARED_COOLIMAGES 0
#else
#define USE_SHARED_COOLIMAGES 1
#endif

/*****************************************************************************************/

// #define TURN_OFF_DEBUG

/* Predeclaration */
struct AslBase_intern;


/* Internal requester structure */
struct IntReq
{
    UWORD		ir_ReqType;
    struct Window	*ir_Window;
    struct Screen	*ir_Screen;
    STRPTR		ir_PubScreenName;
    struct Hook 	*ir_IntuiMsgFunc;
    struct TextAttr	*ir_TextAttr;
    struct Locale	*ir_Locale;
    struct Catalog  	*ir_Catalog;
    APTR		ir_MemPool;
    ULONG		ir_MemPoolPuddle;  	/* if 0, no pool is created */
    ULONG		ir_MemPoolThresh;
    LONG    	    	ir_TitleID;
    STRPTR		ir_TitleText;
    STRPTR		ir_PositiveText;
    STRPTR		ir_NegativeText;
    WORD		ir_LeftEdge;
    WORD		ir_TopEdge;
    WORD		ir_Width;
    WORD		ir_Height;
    UBYTE		ir_Flags;

    APTR		ir_BasePtr;	/* Compatability: Saved copy of REG_A4 */
};

/* Nodes in the ReqList */
struct ReqNode
{
    struct MinNode	rn_Node;
    APTR		rn_Req;
    struct IntReq	*rn_IntReq;
    struct Window   	*rn_ReqWindow;
};

/*****************************************************************************************/

/* Internal filerequester struct */

struct IntFileReq
{
    struct IntReq	ifr_IntReq;

    STRPTR		ifr_File;
    STRPTR		ifr_Drawer;
    STRPTR		ifr_Pattern;
    STRPTR		ifr_AcceptPattern;
    STRPTR		ifr_RejectPattern;
    UBYTE		ifr_Flags1;
    UBYTE		ifr_Flags2;
    struct Hook 	*ifr_FilterFunc;

#ifdef __MORPHOS__
    APTR 		ifr_HookFunc;
#else
    ULONG 		(*ifr_HookFunc)(ULONG mask, APTR object, struct FileRequester *fr);
#endif
    			/* ASLFR_HookFunc = Combined callback function */

    ULONG		*ifr_GetSortBy;
    ULONG		*ifr_GetSortOrder;
    ULONG		*ifr_GetSortDrawers;
    UWORD		ifr_SortBy;
    UWORD		ifr_SortOrder;
    UWORD		ifr_SortDrawers;
    BOOL		ifr_InitialShowVolumes;        
};

/*****************************************************************************************/

struct IntFontReq
{
    struct IntReq	ifo_IntReq;
    struct TextAttr	ifo_TextAttr;
    UBYTE		ifo_FrontPen;
    UBYTE		ifo_BackPen;
    UBYTE		ifo_DrawMode;

    UBYTE		ifo_Flags;
    UWORD		ifo_MinHeight;
    UWORD		ifo_MaxHeight;
    struct Hook		*ifo_FilterFunc;

#ifdef __MORPHOS__
    APTR 		ifo_HookFunc;
#else
    ULONG		(*ifo_HookFunc)(ULONG, APTR, struct FontRequester *);
#endif

    UWORD		ifo_MaxFrontPen;
    UWORD		ifo_MaxBackPen;

    STRPTR		*ifo_ModeList;
    STRPTR  	    	 ifo_SampleText;
    UBYTE		*ifo_FrontPens;
    UBYTE		*ifo_BackPens;

    STRPTR		ifo_DrawModeJAM1Text;
    STRPTR		ifo_DrawModeJAM2Text;
    STRPTR  	    	ifo_DrawModeCOMPText;
    STRPTR		ifo_DrawMode0Text; 
};

/*****************************************************************************************/

#define ISMF_DOAUTOSCROLL 	1
#define ISMF_DODEPTH      	2
#define ISMF_DOHEIGHT     	4
#define ISMF_DOWIDTH      	8
#define ISMF_DOOVERSCAN   	16

/*****************************************************************************************/

struct IntSMReq
{
    struct IntReq	ism_IntReq;
    struct List		*ism_CustomSMList;
    struct Hook		*ism_FilterFunc;		
    ULONG		ism_Flags;
    ULONG		ism_DisplayID;
    ULONG	 	ism_DisplayWidth;
    ULONG		ism_DisplayHeight;
    ULONG		ism_BitMapWidth;
    ULONG		ism_BitMapHeight;
    UWORD		ism_DisplayDepth;
    UWORD		ism_OverscanType;
    BOOL		ism_AutoScroll;
    ULONG		ism_PropertyFlags;
    ULONG		ism_PropertyMask;
    LONG		ism_MinDepth;
    LONG		ism_MaxDepth;
    LONG		ism_MinWidth;
    LONG		ism_MaxWidth;
    LONG		ism_MinHeight;
    LONG		ism_MaxHeight;
    LONG		ism_InfoLeftEdge;
    LONG		ism_InfoTopEdge;
    BOOL		ism_InfoOpened;
    
    STRPTR		ism_Overscan1Text;
    STRPTR		ism_Overscan2Text;
    STRPTR		ism_Overscan3Text;
    STRPTR		ism_Overscan4Text;
    STRPTR		ism_OverscanNullText;
    STRPTR		ism_AutoScrollOFFText;
    STRPTR		ism_AutoScrollONText;
    STRPTR		ism_AutoScroll0Text;
    
    STRPTR		ism_PropertyList_Title;
    STRPTR		ism_PropertyList_NotWB;
    STRPTR		ism_PropertyList_NotGenlock;
    STRPTR		ism_PropertyList_NotDraggable;
    STRPTR		ism_PropertyList_HAM;
    STRPTR		ism_PropertyList_EHB;
    STRPTR		ism_PropertyList_Interlace;
    STRPTR		ism_PropertyList_ECS;
    STRPTR		ism_PropertyList_WB;
    STRPTR		ism_PropertyList_Genlock;
    STRPTR		ism_PropertyList_Draggable;
    STRPTR		ism_PropertyList_DPFPri2;
    STRPTR		ism_PropertyList_RefreshRate;
};

#define SREQ_FIRST_PROPERTY_ITEM(x) ((x)->ism_PropertyList_NotWB)
#define SREQ_LAST_PROPERTY_ITEM(x) ((x)->ism_PropertyList_RefreshRate)

/*****************************************************************************************/

/* structure for passing arguments to tag parsing hooks */
struct ParseTagArgs
{
    struct IntReq	*pta_IntReq;
    APTR		pta_Req;
    struct TagItem	*pta_Tags;
};

struct AslReqInfo
{
    ULONG 	IntReqSize;
    ULONG 	ReqSize;
    APTR  	DefaultReq;
    /* Size of userdata for GadgetryHook and EventHook */
    ULONG 	UserDataSize;
    struct Hook ParseTagsHook;
    struct Hook GadgetryHook;
};

/*****************************************************************************************/

/* Flags */

#define IF_PRIVATEIDCMP  (1 << 0)
#define IF_SLEEPWINDOW	 (1 << 1)
#define IF_USER_POSTEXT  (1 << 2)
#define IF_USER_NEGTEXT  (1 << 3)
#define IF_POPTOFRONT	 (1 << 4)
#define IF_POPPEDTOFRONT (1 << 5)
#define IF_OPENINACTIVE  (1 << 6)

#define GetIR(ir) ((struct IntReq *)ir)

struct AslBase_intern
{
    struct LibHeader            lh;

    struct MinList		ReqList;
    struct SignalSemaphore	ReqListSem;
    struct AslReqInfo		ReqInfo[3];
    Class 			*aslpropclass;
    Class			*aslarrowclass;
    Class			*asllistviewclass;
    Class			*aslbuttonclass;
    Class			*aslstringclass;
    Class			*aslcycleclass;
    Class   	    	    	*aslfontpreviewclass;
    Class   	    	    	*aslfontstyleclass;
    Class   	    	    	*aslcolorpickerclass;
};

/*****************************************************************************************/

/* Prototypes */

/* basicfuncs.c */

struct ReqNode *FindReqNode(APTR, struct AslBase_intern *);
VOID ParseCommonTags(struct IntReq *, struct TagItem *, struct AslBase_intern *);
UWORD BiggestTextLength(STRPTR *, UWORD, struct RastPort *, struct AslBase_intern *);
VOID StripRequester(APTR, UWORD, struct AslBase_intern *AslBase);

WORD CountNodes(struct List *list, WORD flag);
struct Node *FindListNode(struct List *list, WORD which);
void SortInNode(APTR req, struct List *list, struct Node *node,
		WORD (*compare)(APTR, APTR, APTR, struct AslBase_intern *),
		struct AslBase_intern *AslBase);

APTR MyAllocVecPooled(APTR pool, IPTR size, struct AslBase_intern *AslBase);
void MyFreeVecPooled(APTR mem, struct AslBase_intern *AslBase);
char *PooledCloneString(const char *name1, const char *name2, APTR pool,
			struct AslBase_intern *AslBase);
char *PooledCloneStringLen(const char *name1, ULONG len1, const char *name2, ULONG len2, APTR pool,
			   struct AslBase_intern *AslBase);
char *VecCloneString(const char *name1, const char *name2, struct AslBase_intern *AslBase);
char *VecPooledCloneString(const char *name1, const char *name2, APTR pool,
			   struct AslBase_intern *AslBase);
char *PooledUIntegerToString(IPTR value, APTR pool, struct AslBase_intern *AslBase);
void CloseWindowSafely(struct Window *window, struct AslBase_intern *AslBase);

AROS_UFP3(ULONG, StringEditFunc,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(struct SGWork *,		sgw,		A2),
    AROS_UFPA(ULONG *, 			command,	A1));

/* classes.c */

Class *makeaslpropclass(struct AslBase_intern *AslBase);
Class *makeaslarrowclass(struct AslBase_intern *AslBase);
Class *makeasllistviewclass(struct AslBase_intern *AslBase);
Class *makeaslbuttonclass(struct AslBase_intern *AslBase);
Class *makeaslstringclass(struct AslBase_intern *AslBase);
Class *makeaslcycleclass(struct AslBase_intern *AslBase);
Class *makeaslfontpreviewclass(struct AslBase_intern *AslBase);
Class *makeaslfontstyleclass(struct AslBase_intern *AslBase);
Class *makeaslcolorpickerclass(struct AslBase_intern *AslBase);

/* gadgets.c */

BOOL makescrollergadget(struct ScrollerGadget *scrollergad, struct LayoutData *ld, 
			struct TagItem *tags, struct AslBase_intern *AslBase);
void killscrollergadget(struct ScrollerGadget *scrollergad, struct AslBase_intern *AslBase);
void getgadgetcoords(struct Gadget *gad, struct GadgetInfo *gi, WORD *x, WORD *y, WORD *w, WORD *h);
void connectscrollerandlistview(struct ScrollerGadget *scrollergad, Object *listview,
				struct AslBase_intern *AslBase);
void FreeObjects(Object **first, Object **last, struct AslBase_intern *AslBase);

/* locale.c */

STRPTR GetString(LONG id, struct Catalog *catalog, struct AslBase_intern *AslBase);
void LocalizeMenus(struct NewMenu *nm, struct Catalog *catalog, struct AslBase_intern *AslBase);

/*****************************************************************************************/


AROS_UFP3(VOID, FRTagHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFPA(struct AslBase_intern *,  AslBase,        A1)
);

AROS_UFP3(ULONG, FRGadgetryHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(struct LayoutData *,      ld,             A2),
    AROS_UFPA(struct AslBase_intern *,  AslBase,        A1)
);

AROS_UFP3(VOID, FOTagHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFPA(struct AslBase_intern *,  AslBase,        A1)
);

AROS_UFP3(ULONG, FOGadgetryHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(struct LayoutData *,      ld,             A2),
    AROS_UFPA(struct AslBase_intern *,  AslBase,        A1)
);

AROS_UFP3(VOID, SMTagHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFPA(struct AslBase_intern *,  AslBase,        A1)
);

AROS_UFP3(ULONG, SMGadgetryHook,
    AROS_UFPA(struct Hook *,            hook,           A0),
    AROS_UFPA(struct LayoutData *,      ld,             A2),
    AROS_UFPA(struct AslBase_intern *,  AslBase,        A1)
);

/*****************************************************************************************/

/* Return values for the gadgetry hooks */
#define GHRET_FINISHED_OK   2
#define GHRET_OK	    1
#define GHRET_FAIL	    0

/* Handy macros */
#define SETFLAG(flagvar, boolvar, flag) \
    if (boolvar)                        \
	flagvar |= flag;		\
    else				\
	flagvar &= ~flag;


/*****************************************************************************************/

#undef ASLB
#define ASLB(b) ((struct AslBase_intern *)b)

#endif /* ASL_INTERN_H */
