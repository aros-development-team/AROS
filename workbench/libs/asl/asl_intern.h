#ifndef ASL_INTERN_H
#define ASL_INTERN_H

#undef	AROS_ALMOST_COMPATIBLE
#define AROS_ALMOST_COMPATIBLE


#ifndef EXEC_TYPES_H
#    include <exec/types.h>
#endif

#ifndef EXEC_LIBRARIES_H
#    include <exec/libraries.h>
#endif

#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif

#ifndef LIBRARIES_ASL_H
#    include <libraries/asl.h>
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

/*****************************************************************************************/

// #define TURN_OFF_DEBUG

/* Predeclaration */
struct AslBase_intern;

#define GLOBAL_INTUIBASE
#define GLOBAL_SYSBASE
#define GLOBAL_DOSBASE

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
    APTR		ir_MemPool;
    ULONG		ir_MemPoolPuddle;  /* if 0, no pool is created */
    ULONG		ir_MemPoolThresh;
    STRPTR		ir_TitleText;
    STRPTR		ir_PositiveText;
    STRPTR		ir_NegativeText;
    WORD		ir_LeftEdge;
    WORD		ir_TopEdge;
    WORD		ir_Width;
    WORD		ir_Height;
    UBYTE		ir_Flags;

};

/* Nodes in the ReqList */
struct ReqNode
{
    struct MinNode	rn_Node;
    APTR		rn_Req;
    struct IntReq	*rn_IntReq;
};

/*****************************************************************************************/

/* Internal filerequester struct */
struct IntFileReq
{
	struct IntReq	ifr_IntReq;

    STRPTR	ifr_File;
    STRPTR	ifr_Drawer;
    STRPTR	ifr_Pattern;
    STRPTR	ifr_AcceptPattern;
    STRPTR	ifr_RejectPattern;
    UBYTE	ifr_Flags1;
    UBYTE	ifr_Flags2;
    struct Hook *ifr_FilterFunc;
    ULONG 	(*ifr_HookFunc)(ULONG mask, APTR object, struct FileRequester *fr);
    		/* ASLFR_HookFunc = Combined callback function */

    /* Some gadgettext specific for the file requester */
    STRPTR	ifr_VolumesText;
    STRPTR	ifr_ParentText;
    STRPTR	ifr_PatternText;
    STRPTR	ifr_DrawerText;
    STRPTR	ifr_FileText;
    STRPTR	ifr_LVDrawerText;
    STRPTR	ifr_LVAssignText;

    STRPTR	ifr_Menu_Control;
    STRPTR	ifr_Item_Control_LastName;
    STRPTR	ifr_Item_Control_NextName;
    STRPTR	ifr_Item_Control_Restore;
    STRPTR	ifr_Item_Control_Parent;
    STRPTR	ifr_Item_Control_Volumes;
    STRPTR	ifr_Item_Control_Update;
    STRPTR	ifr_Item_Control_Delete;
    STRPTR	ifr_Item_Control_CreateNewDrawer;
    STRPTR	ifr_Item_Control_Rename;
    STRPTR	ifr_Item_Control_Select;
    STRPTR	ifr_Item_Control_OK;
    STRPTR	ifr_Item_Control_Cancel;
    STRPTR	ifr_Menu_FileList;
    STRPTR	ifr_Item_FileList_SortByName;
    STRPTR	ifr_Item_FileList_SortByDate;
    STRPTR	ifr_Item_FileList_SortBySize;
    STRPTR	ifr_Item_FileList_AscendingOrder;
    STRPTR	ifr_Item_FileList_DescendingOrder;
    STRPTR	ifr_Item_FileList_ShowDrawersFirst;
    STRPTR	ifr_Item_FileList_ShowDrawerWithFiles;
    STRPTR	ifr_Item_FileList_ShowDrawersLast;
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
    struct Hook		*ifo_HookFunc;
    UBYTE		ifo_MaxFrontPen;
    UBYTE		ifo_MaxBackPen;

    STRPTR		*ifo_ModeList;
    UBYTE		*ifo_FrontPens;
    UBYTE		*ifo_BackPens;

};

#define ISMF_DOAUTOSCROLL 1
#define ISMF_DODEPTH      2
#define ISMF_DOHEIGHT     4
#define ISMF_DOWIDTH      8
#define ISMF_DOOVERSCAN   16

/*****************************************************************************************/

struct IntModeReq
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
    
    /* Some gadgettext specific for the screenmode requester */
    
    STRPTR		ism_OverscanText;
    STRPTR		ism_Overscan1Text;
    STRPTR		ism_Overscan2Text;
    STRPTR		ism_Overscan3Text;
    STRPTR		ism_Overscan4Text;
    STRPTR		ism_OverscanNullText;
    STRPTR		ism_WidthText;
    STRPTR		ism_HeightText;
    STRPTR		ism_ColorsText;
    STRPTR		ism_AutoScrollText;
    STRPTR		ism_AutoScrollOFFText;
    STRPTR		ism_AutoScrollONText;
    STRPTR		ism_AutoScroll0Text;
    
    STRPTR		ism_Menu_Control;
    STRPTR		ism_Item_Control_LastMode;
    STRPTR		ism_Item_Control_NextMode;
    STRPTR		ism_Item_Control_PropertyList;
    STRPTR		ism_Item_Control_Restore;
    STRPTR		ism_Item_Control_OK;
    STRPTR		ism_Item_Control_Cancel;
    
    STRPTR		ism_PropertyList_Title;
    
};

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

#define GetIR(ir) ((struct IntReq *)ir)

struct AslBase_intern
{
    struct Library		library;
    struct ExecBase		*sysbase;
    BPTR			seglist;

#ifndef GLOBAL_DOSBASE
    struct Library		*dosbase;
#endif
#ifndef GLOBAL_INTUIBASE
    struct IntuitionBase 	*intuitionbase;
#endif

    struct GfxBase		*gfxbase;
    struct Library		*cybergfxbase;
    struct Library		*boopsibase;
    struct Library		*utilitybase;
    struct Library		*gadtoolsbase;
    struct Library		*aroslistviewbase;
    struct Library		*aroslistbase;

    struct MinList		ReqList;
    struct SignalSemaphore	ReqListSem;
    struct AslReqInfo		ReqInfo[3];
    Class 			*aslpropclass;
    Class			*aslarrowclass;
    Class			*asllistviewclass;
    Class			*aslbuttonclass;
    Class			*aslstringclass;
    Class			*aslcycleclass;
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
void SortInNode(struct List *list, struct Node *node, WORD (*getpri)(APTR));

APTR AllocVecPooled(APTR pool, IPTR size);
void FreeVecPooled(APTR mem);
char *PooledCloneString(const char *name1, const char *name2, APTR pool,
			struct AslBase_intern *AslBase);
char *VecCloneString(const char *name1, const char *name2, struct AslBase_intern *AslBase);
char *VecPooledCloneString(const char *name1, const char *name2, APTR pool,
			   struct AslBase_intern *AslBase);
char *PooledIntegerToString(IPTR value, APTR pool, struct AslBase_intern *AslBase);

/* classes.c */

Class *makeaslpropclass(struct AslBase_intern *AslBase);
Class *makeaslarrowclass(struct AslBase_intern *AslBase);
Class *makeasllistviewclass(struct AslBase_intern *AslBase);
Class *makeaslbuttonclass(struct AslBase_intern *AslBase);
Class *makeaslstringclass(struct AslBase_intern *AslBase);
Class *makeaslcycleclass(struct AslBase_intern *AslBase);

/* gadgets.c */

BOOL makescrollergadget(struct ScrollerGadget *scrollergad, struct LayoutData *ld, 
			struct TagItem *tags, struct AslBase_intern *AslBase);
void killscrollergadget(struct ScrollerGadget *scrollergad, struct AslBase_intern *AslBase);
void getgadgetcoords(struct Gadget *gad, struct GadgetInfo *gi, WORD *x, WORD *y, WORD *w, WORD *h);
void connectscrollerandlistview(struct ScrollerGadget *scrollergad, Object *listview,
				struct AslBase_intern *AslBase);
void FreeObjects(Object **first, Object **last, struct AslBase_intern *AslBase);

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

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct GfxBase GraphicsBase;
typedef struct IntuitionBase IntuiBase;

#undef ASLB
#define ASLB(b) ((struct AslBase_intern *)b)
#undef UtilityBase
#define UtilityBase	ASLB(AslBase)->utilitybase

#define GadToolsBase	ASLB(AslBase)->gadtoolsbase

#define CyberGfxBase	ASLB(AslBase)->cybergfxbase

#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase	ASLB(AslBase)->intuitionbase
#endif

#undef GfxBase
#define GfxBase 	ASLB(AslBase)->gfxbase

#undef BOOPSIBase
#define BOOPSIBase	   ASLB(AslBase)->boopsibase

#ifndef GLOBAL_DOSBASE
#undef DOSBase
#define DOSBase 	ASLB(AslBase)->dosbase
#endif

#ifndef GLOBAL_SYSBASE
#undef SysBase
#define SysBase 	ASLB(AslBase)->sysbase
#endif


#define expunge() \
AROS_LC0(BPTR, expunge, struct AslBase_intern *, AslBase, 3, Asl)

#endif /* ASL_INTERN_H */
