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
    STRPTR		ir_TitleText;
    STRPTR		ir_PositiveText;
    STRPTR		ir_NegativeText;
    UWORD		ir_LeftEdge;
    UWORD		ir_TopEdge;
    UWORD		ir_Width;
    UWORD		ir_Height;
    UBYTE		ir_Flags;

};

/* Nodes in the ReqList */
struct ReqNode
{
    struct MinNode	rn_Node;
    APTR		rn_Req;
    struct	IntReq	*rn_IntReq;
};


/* Internal filerequester struct */
struct IntFileReq
{
	struct IntReq	ifr_IntReq;

    STRPTR	ifr_File;
    STRPTR	ifr_Drawer;
    STRPTR	ifr_Pattern;
    UBYTE	ifr_Flags1;
    UBYTE	ifr_Flags2;
    struct Hook *ifr_FilterFunc;
    struct Hook *ifr_HookFunc; /* Combined callback function */

    /* Some gadgettext specific for the file requester */
    STRPTR	ifr_VolumesText;
    STRPTR	ifr_ParentText;
};

struct IntFontReq
{
    struct IntReq	ifo_IntReq;
    struct TextAttr	ifo_TextAttr;
    UBYTE	ifo_FrontPen;
    UBYTE	ifo_BackPen;
    UBYTE	ifo_DrawMode;

    UBYTE	ifo_Flags;
    UWORD	ifo_MinHeight;
    UWORD	ifo_MaxHeight;
    struct Hook *ifo_FilterFunc;
    struct Hook *ifo_HookFunc;
    UBYTE	ifo_MaxFrontPen;
    UBYTE	ifo_MaxBackPen;

    STRPTR	*ifo_ModeList;
    UBYTE	*ifo_FrontPens;
    UBYTE	*ifo_BackPens;

};

struct IntModeReq
{
    struct IntReq	ism_IntReq;
};


/* structure for passing arguments to tag parsing hooks */
struct ParseTagArgs
{
    struct IntReq	*pta_IntReq;
    APTR		pta_Req;
    struct TagItem	*pta_Tags;
};

struct AslReqInfo
{
    ULONG IntReqSize;
    ULONG ReqSize;
    APTR  DefaultReq;
    /* Size of userdata for GadgetryHook and EventHook */
    ULONG UserDataSize;
    struct Hook ParseTagsHook;
    struct Hook GadgetryHook;
};



/* Flags */

#define IF_PRIVATEIDCMP (1 << 0)
#define IF_SLEEPWINDOW	(1 << 1)

#define GetIR(ir) ((struct IntReq *)ir)

/* Prototypes */

struct ReqNode *FindReqNode(APTR, struct AslBase_intern *);
VOID ParseCommonTags(struct IntReq *, struct TagItem *, struct AslBase_intern *);
UWORD BiggestTextLength(STRPTR *, UWORD, struct RastPort *, struct AslBase_intern *);
VOID StripRequester(APTR, UWORD, struct AslBase_intern *AslBase);

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


struct AslBase_intern
{
    struct Library	library;
    struct ExecBase	*sysbase;
    BPTR		seglist;

#ifndef GLOBAL_DOSBASE
    struct Library	*dosbase;
#endif
#ifndef GLOBAL_INTUIBASE
    struct IntuitionBase *intuitionbase;
#endif

    struct GfxBase	*gfxbase;
    struct Library	*boopsibase;
    struct Library	*utilitybase;
    struct Library	*aroslistviewbase;
    struct Library	*aroslistbase;

    struct MinList		ReqList;
    struct SignalSemaphore	ReqListSem;
    struct AslReqInfo		ReqInfo[3];
};

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
