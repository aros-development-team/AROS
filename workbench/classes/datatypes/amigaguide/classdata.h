/*
** $PROJECT: amigaguide.datatype
**
** $VER: classdata.h 1.2 (14.06.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

/* ------------------------------- includes ------------------------------- */

#include "amigaguide_rev.h"

#include "navigator.h"

#ifdef __AROS__
#include <datatypes/amigaguideclass.h>
#else
#include "amigaguideclass.h"
#endif

#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <datatypes/textclass.h>

#include <libraries/amigaguide.h>

#include <rexx/rxslib.h>
#include <rexx/errors.h>

/* -------------------------- some misc defines --------------------------- */

/* buffer size for file scanning per line */
#define AG_BUFSIZE_LINE   1024
/* buffer size for memory pool puddle size */
#define AG_PUDDLE_SIZE    4096

/* distance in pixel between navigator gadget and AG document */
#define AG_NAV_DISTANCE   4

/* define help trigger command */
#ifndef STM_HELP
#define STM_HELP   17
#endif

/* SendRexxCommand() modes */
#define AGRX_RX   0
#define AGRX_RXS  1

/* ------------------------ AmigaGuide definitions ------------------------ */

enum
{
   CMD_AUTHOR,
   CMD_COPYRIGHT,
   CMD_DATABASE,
   CMD_DNODE,
   CMD_ENDNODE,
   CMD_KEYWORDS,
   CMD_FONT,
   CMD_HEIGHT,
   CMD_HELP,
   CMD_INDEX,
   CMD_MACRO,
   CMD_MASTER,
   CMD_NEXT,
   CMD_NODE,
   CMD_ONCLOSE,
   CMD_ONOPEN,
   CMD_PREV,
   CMD_REMARK,
   CMD_SMARTWRAP,
   CMD_TAB,
   CMD_TITLE,
   CMD_TOC,
   CMD_VERSION,
   CMD_WIDTH,
   CMD_WORDWRAP,
   CMD_WORDDELIM,  /* new for V50. */

   CMD_MAX
};

enum
{
   ATTR_AMIGAGUIDE,
   ATTR_APEN,
   ATTR_B,
   ATTR_BG,
   ATTR_BODY,
   ATTR_BPEN,
   ATTR_CLEARTABS,
   ATTR_CODE,
   ATTR_FG,
   ATTR_I,
   ATTR_JCENTER,
   ATTR_JLEFT,
   ATTR_JRIGHT,
   ATTR_LINDENT,
   ATTR_LINE,
   ATTR_PAR,
   ATTR_PARD,
   ATTR_PARI,
   ATTR_PLAIN,
   ATTR_SETTABS,
   ATTR_TAB,
   ATTR_U,
   ATTR_UB,
   ATTR_UI,
   ATTR_UU,

   ATTR_MAX
};

enum
{
   CMDTYPE_UNKNOWN,
   CMDTYPE_LINK,
   CMDTYPE_SYSTEM,
   CMDTYPE_RXS,
   CMDTYPE_RX,

   CMDTYPE_MAX
};

struct AmigaGuideCmd
{
   const STRPTR agc_Name;
   const LONG agc_Id;
};

struct AmigaGuideAttr
{
   const STRPTR aga_Name;
   const LONG aga_Id;
};

struct AmigaGuideMacro
{
   struct Node agm_Node;
   STRPTR agm_Macro;
};

struct AmigaGuideFile
{
   struct Node agf_Node;

   BPTR agf_Handle;           /* cache file handle for actual AG database */
   BPTR agf_Lock;             /* lock on the file for SameLock() checking */

   struct List agf_Nodes;     /* list of all nodes for this AG document */
   struct List agf_Macros;    /* list of all AG macros for this AG document */

   UWORD agf_NodeCount;

   struct
   {
      /* internal state flags */
      ULONG CloseHandle : 1;  /* close agf_Handle on om_dispose() */

      /* document flags */
      ULONG SmartWrap : 1;    /* global smartwrap */
      ULONG WordWrap  : 1;    /* global wordwrap */
   } agf_Flags;

   /* global layout attributes */
   UWORD agf_TabWidth;
   UWORD agf_Width;
   UWORD agf_Height;

   /* global strings */
   STRPTR agf_Name;
   STRPTR agf_Help;
   STRPTR agf_Index;
   STRPTR agf_TOC;
   STRPTR agf_Copyright;
   STRPTR agf_Version;
   STRPTR agf_Author;
   STRPTR agf_Font;

   STRPTR agf_WordDelim;

   STRPTR agf_OnOpen;
   STRPTR agf_OnClose;
};


struct AmigaGuideNode
{
   struct Node agn_Node;
   ULONG agn_Pos;
   ULONG agn_Length;

   STRPTR agn_Keywords;
   STRPTR agn_Font;
   STRPTR agn_OnOpen;
   STRPTR agn_OnClose;

   struct AmigaGuideFile *agn_File;
};

struct AmigaGuideObject
{
   struct Node ago_Node;
   Object *ago_Object;

   BPTR ago_TmpHandle;
   struct AmigaGuideNode *ago_AGNode;
   STRPTR ago_Buffer;
   ULONG ago_BufferLen;

   LONG ago_TopVert;
   BOOL ago_NoDispose;
};

#define AGMT_TRIGGER   1
#define AGMT_LAYOUT    2

/* ---------------------------- instance data ----------------------------- */

/* Instance data structure for your subclass. see INSTANCESIZE define */
struct AmigaGuideData
{
   /* run time data */
   APTR ag_Pool;                /* memory pool for this AG object */

   struct NavigatorButton *ag_Buttons;
   struct Gadget *ag_Gadget;    /* navigator gadget */
   UWORD ag_NavHeight;          /* navigator height */
   struct IBox ag_SubObjDomain; /* domain to use for AG sub objects */

   STRPTR ag_InitialNode;       /* node to load initially */
   struct AmigaGuideFile *ag_File; /* structure to actual AG File */

   Object *ag_Actual;           /* actual object to display (short cut) */
   struct AmigaGuideObject *ag_ActualObject;
   struct List ag_Files;        /* list of opened AmigaGuide files */
   struct List ag_Visited;      /* list of visited nodes */

   struct Process *ag_Process;  /* external process */
   struct MsgPort *ag_ProcPort; /* message port to send async messages */
   struct Task *ag_Parent;      /* used for handshaking during termination. */

   STRPTR ag_ARexxPortName;     /* stem name for creating arexx port */
   struct MsgPort *ag_RexxPort; /* ARexx port */
   LONG ag_RexxOutstanding;     /* outstanding rexx messages */

   /* task which created this object to be used by arexx quit and close cmds */
   struct Task *ag_Creator;
   /* window this object is attached to. */
   struct Window *ag_Window;

   Object *ag_ICTarget;
   struct TagItem *ag_ICMap;

   /* flags */
   struct
   {
      ULONG Redraw     : 1; /* redraw object */
      ULONG InNavInput : 1; /* in navigator input state */
      ULONG InDocInput : 1; /* in document input state */
      ULONG InAsyncLayout : 1; /* currently in async layout method */
      ULONG InitialLayout : 1; /* layout sub-object initially */
      ULONG Secure : 1;     /* In secure mode don't launch any program or script. */
      ULONG GotoLine : 1;
   } ag_Flags;

   struct SignalSemaphore ag_ASyncLayout;

   UBYTE ag_RexxName[64];
   UBYTE ag_Message[128];   /* buffer for notify text messages to target */
};


/* ---------------------- AmigaGuide node attributes ---------------------- */

#define AGNA_Command    (TAG_USER + 0x107001)
#define AGNA_Help       (TAG_USER + 0x107002)
#define AGNA_Contents   (TAG_USER + 0x107003)
#define AGNA_Index      (TAG_USER + 0x107004)
#define AGNA_Previous   (TAG_USER + 0x107005)
#define AGNA_Next       (TAG_USER + 0x107006)

#define AGNA_RootObject (TAG_USER + 0x107007)
#define AGNA_AGFile     (TAG_USER + 0x107008)

/* ------------------------- function prototypes -------------------------- */

struct ClassBase;
struct Rectangle;

LONG GetGlobalCommand(struct ClassBase *cb,
		      STRPTR cmd, STRPTR *args);
LONG GetNodeCommand(struct ClassBase *cb,
		    STRPTR cmd, STRPTR *args);

APTR AllocAGMem(Class *cl, Object *obj, ULONG size);
void FreeAGMem(Class *cl, Object *obj, APTR mem, ULONG size);

APTR AllocAGVec(Class *cl, Object *obj, ULONG size);
void FreeAGVec(Class *cl, Object *obj, APTR mem);

STRPTR CopyAGString(Class *cl, Object *obj, STRPTR args);
STRPTR CopyString(Class *cl, Object *obj, STRPTR args);

struct AmigaGuideFile *AllocAGFile(Class *cl, Object *obj);
struct AmigaGuideNode *GetAGNode(Class *cl, Object *obj,
                                 struct AmigaGuideFile *agf,
                                 STRPTR name);
struct AmigaGuideObject *AllocAGObject(Class *cl, Object *obj);
struct AmigaGuideObject *AllocAGObjectNode(Class *cl, Object *obj,
                                           struct AmigaGuideFile *agf,
                                           struct AmigaGuideNode *agnode);
void FreeAGObject(Class *cl, Object *obj, struct AmigaGuideObject *agobj);

void ScanFile(Class *cl, Object *obj, struct AmigaGuideFile *agf);

/* navigator.c functions */
Class *MakeNavigatorClass(struct ClassBase *cb);
BOOL FreeNavigatorClass(struct ClassBase *cb, Class *cl);

/* nodeclass.c functions */
Class *MakeNodeClass(struct ClassBase *cb);
BOOL FreeNodeClass(struct ClassBase *cb, Class *cl);


/* util.c functions */
struct Region *InstallClipRegionSafe(struct ClassBase *cb, struct GadgetInfo *ginfo,
				     struct Region *reg, struct Rectangle *rect);
void UnInstallClipRegionSafe(struct ClassBase *cb, struct GadgetInfo *ginfo,
			     struct Region *oldreg);

#ifdef __AROS__
ULONG NotifyAttrs(Object * obj, void * ginfo, ULONG flags, ...);
#else
ULONG NotifyAttrs(Object * obj, void * ginfo, ULONG flags, ...) __attribute__((varargs68k));
#endif

BOOL GetDTDomain(Class *cl, Object *obj, struct IBox *domain);

ULONG DoTrigger(Class *cl, Object *obj, struct GadgetInfo *ginfo, ULONG function, APTR data);

ULONG GotoObject(Class *cl, Object *obj, struct GadgetInfo *ginfo, STRPTR nodename, LONG line);
ULONG GotoObjectTag(Class *cl, Object *obj, struct GadgetInfo *ginfo, Tag tag);

void ParseFontLine(Class *cl, Object *obj, STRPTR args, struct TextAttr *ta);

BPTR GetFileLock(Class *cl, Object *obj,
                 STRPTR file, BOOL *nodetype);

ULONG SendRexxCommand(Class *cl, Object *obj, STRPTR command, ULONG mode);
ULONG SystemCommand(Class *cl, Object *obj, STRPTR command);

LONG mysprintf(struct ClassBase *cb, STRPTR buf, LONG len, STRPTR format,...);

BOOL GetFontDimension(Class *cl, Object *obj, STRPTR font, WORD *x, WORD *y);

/* process.c functions */
struct Process *CreateAGProcess(Class *cl, Object *obj);
void DeleteAGProcess(Class *cl, Object *obj);

BOOL SendAGTrigger(Class *cl, Object *obj, struct dtTrigger *dtt);
BOOL SendAGLayout(Class *cl, Object *obj, struct gpLayout *gpl);

/* ------------------------ some inline functions ------------------------- */

static inline
STRPTR eatws(STRPTR p)
{
   while(*p == ' ' || *p == '\t')
      p++;
   return p;
}

/* --------------------------- some cast macros --------------------------- */

#define CAST_GAD(x)       ((struct Gadget *) (x))
#define CAST_SET(x)       ((struct opSet *) (x))
#define CAST_GPL(x)       ((struct gpLayout *) (x))
#define CAST_OBJ(x)       ((Object *) (x))

/* -------------------- defines needed by classbase.c --------------------- */

#define INSTANCESIZE             sizeof(struct AmigaGuideData)
#define CLASSNAME                "amigaguide_mos.datatype"
#define SUPERCLASSNAME           "datatypesclass"

/* ------------------- optional defines for classbase.c ------------------- */

/* Define the following, if you derive a subclass from an existing
 * superclass library.
 * For example :
 * #define SUPERCLASSLIBRARY        "datatypes/hypertext.datatype"
 * #define SUPERCLASSLIBRARYVERSION 40
 */

#undef SUPERCLASSLIBRARY
#undef SUPERCLASSLIBRARYVERSION


/* The following define enables the usages of the functions :
 *
 * BOOL UserClassBaseInit(struct ClassBase *cb);
 * void UserClassBaseExpunge(struct ClassBase *cb);
 *
 * You have to provide these two functions if USERCLASSBASEINIT is defined.
 * Note within this context only exec.library calls are valid!
 */
#define USERCLASSBASEINIT 1
#undef USERCLASSBASEINIT

/* The following define is used to add fields to the ClassBase structure.
 * For example :
 */
#define USERCLASSBASEDATA        ULONG cb_Flags;   \
				 Class *cb_Navigator; \
				 struct Library *cb_TextDTBase; \
				 Class *cb_NodeClass;

/* With the following define its possible to add own
 * functions to the library function table.
 * Note: Prototypes are needed.
 * For example :
 */
#define USERCLASSBASEFUNCTABLE   LibMyFirstFunc,      \
				 LibMyLastFunc,
#undef USERCLASSBASEFUNCTABLE

/* The following define enables the usage of the functions :
 *
 * BOOL UserClassBaseOpen(struct ClassBase *cb);
 * void UserClassBaseClose(struct ClassBase *cb);
 *
 * You have to provide these two functions if USERCLASSBASEOPEN is defined.
 */
#define USERCLASSBASEOPEN 1

/* Define the following, if you want to include a normal version string
 * including $VER: string. This fixes problems with installer v43, which
 * can't find RomTag version information.
 */
#define USE_VERSTAG

/* ------------------------- supported libraries -------------------------- */

/* Define following defines to enable support for appropriate library.
 */
#define USE_GFXLIB
#define USE_DOSLIB
#define USE_LOCALELIB
#define USE_DATATYPESLIB
#define USE_LAYERSLIB
#define USE_REXXSYSLIB
#define USE_DISKFONTLIB

/* ---------------------------- useful defines ---------------------------- */

#define INSTDATA struct AmigaGuideData *data = (struct AmigaGuideData *) INST_DATA(cl,obj)

