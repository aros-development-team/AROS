/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Asl initialization code.
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE /* For NEWLIST() macro */

#include <stddef.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/boopsi.h>

#include "initstruct.h"
#include "asl_intern.h"
#include "libdefs.h"

#include <gadgets/aroslist.h>
#include <gadgets/aroslistview.h>

#define INIT AROS_SLIB_ENTRY(init, Asl)

#define DEBUG 0
#include <aros/debug.h>

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct AslBase_intern *INIT();
extern struct AslBase_intern *AROS_SLIB_ENTRY(open,Asl)();
extern BPTR AROS_SLIB_ENTRY(close,Asl)();
extern BPTR AROS_SLIB_ENTRY(expunge,Asl)();
extern int AROS_SLIB_ENTRY(null,Asl)();
extern const char LIBEND;

/* FIXME: egcs 1.1b and possibly other incarnations of gcc have
 * two nasty problems with entry() that prevents using the
 * C version of this function on AROS 68k native.
 *
 * First of all, if inlining is active (-O3), the optimizer will decide
 * that entry() is simple enough to be inlined, and it doesn't generate
 * its code until all other functions have been compiled. Delaying asm
 * output for a global (non static) function is probably silly because
 * the optimizer can't eliminate its stand alone istance anyway.
 *
 * The second problem is that even without inlining, the code generator
 * adds a nop instruction immediately after rts. This is probably done
 * to help the 68040/60 pipelines, but it adds two more bytes before the
 * library resident tag, which causes all kinds of problems on native
 * AmigaOS.
 *
 * The workaround is to embed the required assembler instructions
 * (moveq #-1,d0 ; rts) in a constant variable.
 */
#if (defined(__mc68000__) && (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE))
const LONG entry = 0x70FF4E75;
#else
int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}
#endif

static const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    0,	/* priority */
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]=NAME_STRING;

const char version[]=VERSION_STRING;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct AslBase_intern),
    (APTR)LIBFUNCTABLE,
    (APTR)&datatable,
    &INIT
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (LIBEND);
};

#define O(n) offsetof(struct AslBase_intern,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(library.lib_Version     )), { VERSION_NUMBER } } },
    { { I_CPYO(1,W,O(library.lib_Revision    )), { REVISION_NUMBER } } },
    { { I_CPYO(1,L,O(library.lib_IdString    )), { (IPTR)&version[6] } } },
  I_END ()
};


/* #undef O
#undef SysBase */


/* Requester type specific default data */
const struct IntFileReq def_filereq =
{
    {
	ASL_FileRequest,
	NULL,	/* Window		*/
	NULL,	/* Screen		*/
	NULL,	/* PubScreenName	*/
	NULL,	/* IntuiMsgFunc 	*/
	NULL,	/* TextAttr		*/
	NULL,	/* Locale		*/
	NULL,	/* MemPool		*/
	2048,	/* MemPoolPuddle	*/
	2048,	/* MemPoolThresh	*/
	"Select File",
	"Ok",
	"Cancel",
	-1, -1,	 /* --> center on screen */
	300, 300
    },

    "",			/* File 	 */
    "",     		/* Drawer        */
    "#?",       	/* Pattern       */
    NULL,		/* AcceptPattern */ /* def. = "#?", but must be ParsePatternNoCase'ed */
    NULL,		/* RejectPattern */ /* def. = "~(#?)", but must be ParsePatternNoCase'ed */
    0,			/* Flags1	 */
    FRF_REJECTICONS,	/* Flags2	 */
    NULL,		/* FilterFunc	 */
    NULL,		/* HookFunc	 */
    "Volumes", 	 	/* VolumesText   */
    "Parent",  		/* CancelText    */
    "Pattern",		/* PatternText   */
    "Drawer",		/* DrawerText    */
    "File",		/* FileText	 */
    "Drawer",		/* LVDrawerText  */
    "Assign",		/* LVAssignText */
    
    /* Menus */
    
    "Control",

    "L\0Last Name",
    "N\0Next Name",
    "R\0Restore",
    "P\0Parent",
    "V\0Volumes",
    "U\0Update",
    "D\0Delete",
    "T\0Create new drawer...",
    "E\0Rename...",
    "#\0Select...",
    "O\0Ok",
    "C\0Cancel",
    
    "File list",
    
    "1\0Sort by name",
    "2\0Sort by date",
    "3\0Sort by size",
    "+\0Ascending order",
    "-\0Descending order",
    "4\0Show drawers first",
    "5\0Show drawers with files",
    "6\0Show drawers last"
    
};

#include <intuition/screens.h> /* Needed for pen constants */
const struct IntFontReq def_fontreq =
{
    {
	ASL_FontRequest,
	NULL,	/* Window		*/
	NULL,	/* Screen		*/
	NULL,	/* PubScreenName	*/
	NULL,	/* IntuiMsgFunc 	*/
	NULL,	/* TextAttr		*/
	NULL,	/* Locale		*/
	NULL,	/* MemPool		*/
	0,	/* MemPoolPuddle	*/
	0,	/* MemPoolThresh	*/
	"Open font",
	"OK",
	"Cancel",
	0, 0,
	500, 300
    },
    {"topaz", 8, 0, 0}, /* Default textattr */
    TEXTPEN,		/* FrontPen	*/
    BACKGROUNDPEN,	/* BackPen	*/
    JAM1,		/* DrawMode	*/
    0,		/* Flags	*/

    2,		/* Minheight	*/
    100,	/* MaxHeight	*/
    NULL,	/* FilterFunc	*/
    NULL,	/* HookFunc	*/
    32, 	/* MaxFrontPen	*/
    32, 	/* MaxBackPen	*/

    NULL,	/* ModeList	*/
    NULL,	/* FrontPens	*/
    NULL	/* BackPens	*/

};



VOID InitReqInfo(struct AslBase_intern *);

/* I have to make IntuitionBase global to make use of NewObject() */
#ifdef GLOBAL_INTUIBASE
struct IntuitionBase * IntuitionBase;
#endif

/* snprintf() need global SysBase & DOSBase */
#ifdef GLOBAL_SYSBASE
struct ExecBase *SysBase;
#endif

#ifdef GLOBAL_DOSBASE
struct DosLibrary *DOSBase;
#endif


AROS_LH2(struct AslBase_intern *, init,
    AROS_LHA(struct AslBase_intern *, LIBBASE, D0),
    AROS_LHA(BPTR,               segList,   A0),
    struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* This function is single-threaded by exec by calling Forbid. */

    SysBase=sysBase;

    D(bug("Inside initfunc\n"));

    LIBBASE->seglist=segList;

    /* You would return NULL here if the init failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}



AROS_LH1(struct AslBase_intern *, open,
    AROS_LHA(ULONG, version, D0),
    struct AslBase_intern *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */


    /* Keep compiler happy */
    version=0;

    D(bug("Inside openfunc\n"));


    if (!DOSBase)
	DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    if (!DOSBase)
	return(NULL);

    if (!GfxBase)
	GfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase)
	return(NULL);

    if (!CyberGfxBase)
        CyberGfxBase = OpenLibrary("cybergraphics.library",0);
    /* We can live without cybergraphics.library so don't abort if opening fails */
    	
    if (!UtilityBase)
	UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
	return(NULL);

    if (!GadToolsBase)
        GadToolsBase = OpenLibrary("gadtools.library", 37);
    if (!GadToolsBase)
        return (NULL);
	
    if (!BOOPSIBase)
	BOOPSIBase = OpenLibrary(BOOPSINAME, 37);
    if (!BOOPSIBase)
	return(NULL);

    if (!IntuitionBase)
	IntuitionBase = (IntuiBase *)OpenLibrary("intuition.library", 37);
    if (!IntuitionBase)
	return (NULL);

    if (!LIBBASE->aroslistviewbase)
	LIBBASE->aroslistviewbase = OpenLibrary(AROSLISTVIEWNAME, 37);
    if (!LIBBASE->aroslistviewbase)
	return (NULL);

    if (!LIBBASE->aroslistbase)
	LIBBASE->aroslistbase = OpenLibrary(AROSLISTNAME, 37);
    if (!LIBBASE->aroslistbase)
	return (NULL);

    if (!LIBBASE->aslpropclass)
        LIBBASE->aslpropclass = makeaslpropclass(LIBBASE);
    if (!LIBBASE->aslpropclass)
        return (NULL);

    if (!LIBBASE->aslarrowclass)
        LIBBASE->aslarrowclass = makeaslarrowclass(LIBBASE);
    if (!LIBBASE->aslarrowclass)
        return (NULL);
	
    if (!LIBBASE->asllistviewclass)
        LIBBASE->asllistviewclass = makeasllistviewclass(LIBBASE);
    if (!LIBBASE->asllistviewclass)
        return (NULL);

    if (!LIBBASE->aslbuttonclass)
        LIBBASE->aslbuttonclass = makeaslbuttonclass(LIBBASE);
    if (!LIBBASE->aslbuttonclass)
        return (NULL);

    if (!LIBBASE->aslstringclass)
        LIBBASE->aslstringclass = makeaslstringclass(LIBBASE);
    if (!LIBBASE->aslstringclass)
        return (NULL);
	
    /* ------------------------- */

    /* Asl specific initialization stuff */
    NEWLIST( &(ASLB(AslBase)->ReqList));

    InitSemaphore( &(ASLB(AslBase)->ReqListSem));

    InitReqInfo(ASLB(AslBase));

    /* ------------------------- */


    /* I have one more opener. */
    LIBBASE->library.lib_OpenCnt++;
    LIBBASE->library.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct AslBase_intern *, LIBBASE, 2, BASENAME)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--LIBBASE->library.lib_OpenCnt)
    {

	if (LIBBASE->aslpropclass)
	    FreeClass(LIBBASE->aslpropclass);
	LIBBASE->aslpropclass = NULL;

	if (LIBBASE->aslarrowclass)
	    FreeClass(LIBBASE->aslarrowclass);
	LIBBASE->aslarrowclass = NULL;
	
	if (LIBBASE->asllistviewclass)
	    FreeClass(LIBBASE->asllistviewclass);
	LIBBASE->asllistviewclass = NULL;
	
	if (LIBBASE->aslbuttonclass)
	    FreeClass(LIBBASE->aslbuttonclass);
	LIBBASE->aslbuttonclass = NULL;

	if (LIBBASE->aslstringclass)
	    FreeClass(LIBBASE->aslstringclass);
	LIBBASE->aslstringclass = NULL;

	if (GadToolsBase)
	    CloseLibrary(GadToolsBase);
	GadToolsBase = NULL;
	
	if (UtilityBase)
	    CloseLibrary(UtilityBase);
	UtilityBase = NULL;
	
	if (BOOPSIBase)
	    CloseLibrary(BOOPSIBase);
	BOOPSIBase = NULL;
	
	if (CyberGfxBase)
	    CloseLibrary(CyberGfxBase);
	CyberGfxBase = NULL;
	
	if (GfxBase)
	    CloseLibrary((struct Library *)GfxBase);
	GfxBase = NULL;
	
	if (DOSBase)
	    CloseLibrary((struct Library *)DOSBase);
	DOSBase = NULL;
	
	if (IntuitionBase)
	    CloseLibrary((struct Library *)IntuitionBase);
	IntuitionBase = NULL;
	
	if (LIBBASE->aroslistviewbase)
	    CloseLibrary(LIBBASE->aroslistviewbase);
	LIBBASE->aroslistviewbase = NULL;
	
	if (LIBBASE->aroslistbase)
	    CloseLibrary(LIBBASE->aroslistbase);
	LIBBASE->aroslistbase = NULL;

	/* Delayed expunge pending? */
	if(LIBBASE->library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct AslBase_intern *, LIBBASE, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(LIBBASE->library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	LIBBASE->library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=LIBBASE->seglist;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->library.lib_NegSize,
	LIBBASE->library.lib_NegSize+LIBBASE->library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct AslBase_intern *, LIBBASE, 4, BASENAME)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


#include <string.h>
#include "filereqhooks.h"
#include "fontreqhooks.h"


VOID InitReqInfo(struct AslBase_intern *AslBase)
{
    struct AslReqInfo *reqinfo;

    /* Set file requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_FileRequest]);
    reqinfo->IntReqSize 	= sizeof (struct IntFileReq);
    reqinfo->ReqSize		= sizeof (struct FileRequester);
    reqinfo->DefaultReq 	= (struct IntFileReq *)&def_filereq;
    reqinfo->UserDataSize	= sizeof (struct FRUserData);

    memset(&(reqinfo->ParseTagsHook), 0, sizeof (struct Hook));
    memset(&(reqinfo->GadgetryHook),  0, sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry	= (void *)FRTagHook;
    reqinfo->GadgetryHook.h_Entry	= (void *)FRGadgetryHook;

    /* Set font requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_FontRequest]);
    reqinfo->IntReqSize 	= sizeof (struct IntFontReq);
    reqinfo->ReqSize		= sizeof (struct FontRequester);
    reqinfo->DefaultReq 	= (struct IntFontReq *)&def_fontreq;
    reqinfo->UserDataSize	= sizeof (struct FOUserData);

    memset(&(reqinfo->ParseTagsHook), 0, sizeof (struct Hook));
    memset(&(reqinfo->GadgetryHook),  0, sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry	= (void *)FOTagHook;
    reqinfo->GadgetryHook.h_Entry	= (void *)FOGadgetryHook;

    /* Set screenmode requester info */

    reqinfo = &(ASLB(AslBase)->ReqInfo[ASL_ScreenModeRequest]);
    reqinfo->IntReqSize 	= sizeof (struct IntModeReq);
    reqinfo->ReqSize		= sizeof (struct ScreenModeRequester);
    reqinfo->DefaultReq 	= NULL;
    reqinfo->UserDataSize	= 0;

    memset(&(reqinfo->ParseTagsHook), 0, sizeof (struct Hook));
    memset(&(reqinfo->GadgetryHook),  0, sizeof (struct Hook));
    reqinfo->ParseTagsHook.h_Entry	= NULL;
    reqinfo->GadgetryHook.h_Entry	= NULL;

    return;
}
