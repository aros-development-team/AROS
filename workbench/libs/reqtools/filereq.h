/**************************************************************
*                                                             *
*      File/Font/Screenmode requester                         *
*                                                             *
*                                 (c) Nico François 1991-1994 *
**************************************************************/

/****************************************************************************************/

/* INCLUDES */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <libraries/gadtools.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfxmacros.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <proto/layers.h>
#include <proto/wb.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <clib/macros.h>
#include <string.h>

#include <libraries/reqtools.h>
#include <proto/reqtools.h>

#if !defined(__AROS__) && !defined(__GNUC__)
#include <pragmas/reqtools_pragmas.h>
#endif

#ifdef __AROS__
#include <aros/asmcall.h>
#include "reqtools_intern.h"
#endif

/****************************************************************************************/

#include "general.h"
#include "gadstub.h"
#include "boopsigads.h"
#include "mem.h"
#include "rtlocale.h"

/****************************************************************************************/

#define ThisProcess()		( ( struct Process * ) FindTask( NULL ) )

#define DoPkt1(port, action, arg1) DoPkt(port, action, arg1, 0, 0, 0, 0)

#define D_S(type,name)	UBYTE a_##name[ sizeof( type ) + 3 ]; \
			type *name = ( type * )( ( IPTR ) ( a_##name + 3 ) & ~3 );

/****************************************************************************************/

extern APTR ASM Dofmt (ASM_REGPARAM(a3, char *,), ASM_REGPARAM(a0, char *,), ASM_REGPARAM(a1, APTR,));
extern APTR STDARGS DofmtArgs (char *, char *,...);
extern void SetWinTitleFlash (ASM_REGPARAM(a0, struct Window *,), ASM_REGPARAM(a1, char *,));
extern IPTR ASM LoopReqHandler (ASM_REGPARAM(a1, struct rtHandlerInfo *,));
/*extern ULONG CallHook (struct Hook *, APTR,...);*/
extern void ASM StrCat (ASM_REGPARAM(a0, char *,), ASM_REGPARAM(a1, char *,));
extern void ShortDelay (void);

/****************************************************************************************/

#define FPROP		1
#define FILES		2

#define INFO		3
#define ALL		4
#define PATTERN		5
#define CLR		6
#define OK		7
#define DISKS		8
#define PARENT		9
#define CANCEL		10

#define FILESTR		11
#define DRAWERSTR	12
#define PATSTR		13
#define FONTSIZE	14

#define BOLD		15
#define ITALIC		16
#define UNDERLINE	17
#define GETDIR		18

#define DEPTH		19
#define SCRWIDTH	20
#define SCRHEIGHT	21
#define DEFWIDTH	22
#define DEFHEIGHT	23
#define AUTOSCR		24
#define OVERSCN		25

/****************************************************************************************/

extern struct Library		*GadToolsBase;
extern struct DosLibrary	*DOSBase;
extern struct IntuitionBase	*IntuitionBase;
extern struct GfxBase		*GfxBase;
extern struct Library		*LayersBase;
extern struct ReqToolsBase	*ReqToolsBase;
#if defined(__AROS__) || defined(__GNUC__)
extern struct UtilityBase	*UtilityBase;
#else
extern struct Library		*UtilityBase;
#endif
extern struct Library		*WorkbenchBase;

extern char TOPAZSTR[];
#define DOTFONTSTR      &TOPAZSTR[5]

extern char DOTINFOSTR[];

/****************************************************************************************/

/* PRIVATE */
struct ReqEntry
{
    struct Node	re_Node;
    LONG	re_Size;
    UBYTE	re_Flags,
		re_Style;
    UBYTE	re_SizeLenPix,
		re_EntryLen;
};

#define re_Type			re_Node.ln_Type
#define re_Next			re_Node.ln_Succ
#define re_VolLen		re_Node.ln_Pri
#define re_Name			re_Node.ln_Name

/* entry types */
#define FILE			0x0
#define DIRECTORY		0x1
#define FONT			0x2
#define VOLUME			0x3
#define ASSIGN			0x4
#define SCRMODE			0x10

#define MAX_FILE_DIRECTORY	1

/* entry flags */
#define ENTRYF_SELECTED		0x1
#define ENTRYF_HIDDEN		0x2
#define ENTRYF_HIGHLIGHTED	0x4
#define ENTRYF_GHOSTED		0x8

/* PRIVATE */
struct BufferData
{
    struct ReqEntry	*firstname;
    APTR		pool;
    LONG		pos, gotopos, numfiles, currentnum, sorted;
    LONG		file_id, directory_id, dirsmixed;
};

struct RealFileRequester
{
    ULONG		ReqPos;
    UWORD		LeftOffset;
    UWORD		TopOffset;
    ULONG		Flags;
    struct Hook		*Hook;
    STRPTR		Dir;		/* READ ONLY! Change with rtChangeReqAttrA()! */
    STRPTR		MatchPat;	/* READ ONLY! Change with rtChangeReqAttrA()! */
    struct TextFont	*DefaultFont;
    ULONG		WaitPointer;
    ULONG		LockWindow;
    ULONG		ShareIDCMP;
    struct Hook		*IntuiMsgFunc;
    UWORD		ReqLeft;
    UWORD		ReqTop;
    UWORD		ReqWidth;
    UWORD		ReqHeight;

    /** PRIVATE **/
    struct BufferData	buff;
    TEXT		dirname[256], patstr[124], *filename;
    LONG		hideinfo;
};

struct RealFontRequester
{
    ULONG		ReqPos;
    UWORD		LeftOffset;
    UWORD		TopOffset;
    ULONG		Flags;
    struct Hook		*Hook;
    struct TextAttr	Attr;
    struct TextFont	*DefaultFont;
    ULONG		WaitPointer;
    ULONG		LockWindow;
    ULONG		ShareIDCMP;
    struct Hook		*IntuiMsgFunc;
    UWORD		ReqLeft;
    UWORD		ReqTop;
    UWORD		ReqWidth;
    UWORD		ReqHeight;

    /** PRIVATE **/
    struct BufferData	buff;
    TEXT		fontname[108];
};

struct RealScreenModeRequester
{
    ULONG		ReqPos;
    UWORD		LeftOffset;
    UWORD		TopOffset;
    ULONG		Flags;
    ULONG		private1;
    ULONG		DisplayID;
    UWORD		DisplayWidth;
    UWORD		DisplayHeight;
    struct TextFont	*DefaultFont;
    ULONG		WaitPointer;
    ULONG		LockWindow;
    ULONG		ShareIDCMP;
    struct Hook		*IntuiMsgFunc;
    UWORD		ReqLeft;
    UWORD		ReqTop;
    UWORD		ReqWidth;
    UWORD		ReqHeight;
    UWORD		DisplayDepth;
    UWORD		OverscanType;
    ULONG		AutoScroll;

    /** PRIVATE **/
    struct BufferData	buff;
};

typedef struct RealHandlerInfo	GlobData;

#define CHECKBOX_BOLD		0
#define CHECKBOX_ITALIC		1
#define CHECKBOX_UNDERLINE	2
#define CHECKBOX_AUTOSCROLL	3

#define NUMCHECKBOXGADS		5

struct RealHandlerInfo
{
    IPTR				(*func)();        /* private */
    ULONG				WaitMask;
    ULONG				DoNotWait;

    /* PRIVATE */
    APTR				req;
    struct RealFileRequester		*freq;
    struct RealFontRequester 		*fontreq;
    struct RealScreenModeRequester 	*scrmodereq;
    struct BufferData *buff;
    /* fib *MUST* be aligned to a longword boundary! */
    ALIGNED struct FileInfoBlock 	fib;
    ALIGNED struct FileInfoBlock 	linkfib;		/* Soft link support */
    struct NewWindow			newreqwin;
    struct KeyButtonInfo 		buttoninfo;
    struct Image 			labelimages;
    struct TextAttr 			font;
    struct Screen 			*scr, *frontscr;
    struct ViewPort 			*vp;
    struct Window 			*reqwin, *prwin, **winaddr, *oldwinptr;
    struct RastPort 			*reqrp;
    struct Gadget 			*drawergad, *filegad, *scrollergad, *mainstrgad, *patgad;
    struct Gadget 			*okgad, *cancelgad, *activegadget, *numselectedgad;
    struct ReqEntry 			*firstentry, *displaylist[50];
    struct TextFont 			*reqfont;
    struct DrawInfo 			*drinfo;
    struct Hook 			*imsghook, *filterhook, intuihook, backfillhook;
    struct Catalog 			*catalog;
    struct StrGadUserData 		strgaduserdata, fnamegaduserdata;
    struct IntuiText 			itxt;
    char 				*title, *filestr, *drawerstr, *gadtxt[8], tempfname[108], *tempdir;
    int 				boxtop, boxleft, boxright, boxheight, numentries;
    int 				reqheight, reqwidth, fontheight, fontwidth, fontbase, reqtype;
    int 				reqpos, leftedge, topedge, waitpointer, shareidcmp, lockwindow;
    int 				downgadget, clicked, lastclicked, bufferentry, sec, mic;
    int 				entryheight, noscreenpop, underchar, lastdisplaylistnum, os30;
    APTR 				visinfo, winlock;
    UWORD 				*pens;
    ULONG 				flags, rpmask, entrymask, winmask;
    WORD				zoom[4];

    /* file requester */
    struct IntuiText 			selitxt;
    struct AppWindow 			*appwindow;
    struct MsgPort 			*appwinport;
    struct timerequest 			timereq;
    char 				*patgadstr, matchpat[248], selpattern[124];
    int 				nodir, newdir, exnext, disks, wilddotinfo, selectedpos;
    int 				selectcurrpos, allowempty, timerstarted, numselected, numselectedoff;
    int 				file_id, directory_id, quiet, firsttimer, maxvolwidth;
    int 				led_x, led_y, led_w, led_h, ledon;
    ULONG 				volumerequest;
    BPTR 				lock;
    char 				patkey;
    /* file requester, soft link support */
    TEXT 				linkbuf[ 364 ];

    /* font requester */
    int 				fontdisplayleft, fontdisplayright, fontdisplaytop;
    int 				sampleheight, fontstyle, minsize, maxsize, colcount;
    struct Gadget 			*checkboxgad[NUMCHECKBOXGADS];
    char 				gadkey[NUMCHECKBOXGADS];
    APTR 				colormap;
    APTR 				diskfontbase;

    /* screenmode requester */
    struct Gadget 			*depthgad, *widthgad, *heightgad, *maxcolgad, *currcolgad, *overscangad;
    struct Gadget 			*visgad, *nomgad, *defwgad, *defhgad, *modetxtgad;
    struct NameInfo 			nameinfo;
    struct DimensionInfo 		diminfo;
    struct DisplayInfo 			dispinfo;
    int 				usedefwidth, usedefheight, overscantype, autoscroll;
    int 				currmindepth, currmaxdepth, defwidth, defheight;
    int 				maxdepth, mindepth, minheight, maxheight, minwidth, maxwidth;
    UWORD 				width, height, depth;
    ULONG 				modeid, propertymask, propertyflags;
    char 				*oscanlabs[5], heightkey, widthkey, overscankey, depthkey;
    char 				maxcolstr[6], currcolstr[6];
};

/****************************************************************************************/

#define IEQUALIFIER_SHIFT	(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)
#define IEQUALIFIER_ALT		(IEQUALIFIER_LALT|IEQUALIFIER_RALT)

/****************************************************************************************/

struct ReqEntry *REGARGS FindEntry (struct BufferData *, char *, int, int, int *, ULONG);
BOOL REGARGS FindCurrentPos (GlobData *, char *, int, int);

#define FIND_EXACT	0x1
#define FIND_VOLUMENAME	0x2

/****************************************************************************************/

struct ReqEntry *REGARGS AddEntry (GlobData *, struct BufferData *, char *, int, int);
int REGARGS SetupReqWindow (GlobData *, int);
int REGARGS CountAllDeselect (GlobData *, int);
int REGARGS GetModeData (GlobData *, ULONG, int *);
int REGARGS EndsInDotInfo (char *, int);
void REGARGS RenderLED (GlobData *);
void REGARGS RenderReqWindow (GlobData *, int, int);
void REGARGS UpdateDepthDisplay (GlobData *, int, ULONG);
void REGARGS UpdateDepthGad (GlobData *);
void REGARGS UpdateNumSelGad (GlobData *);
void REGARGS GetModeDimensions (GlobData *);
void REGARGS DisplayModeAttrs (GlobData *);
void REGARGS SetSizeGads (GlobData *);
void REGARGS SetTextGad (GlobData *, struct Gadget *, char *);
void REGARGS PrintEntry (GlobData *, int);
void REGARGS ShowDisks (GlobData *);
void REGARGS SelectAll (GlobData *, char *);
void REGARGS AddDiskNames (GlobData *, ULONG);
BOOL REGARGS FindVolume (GlobData *, UBYTE *, struct ReqEntry *);
void REGARGS ClearFilesRect (GlobData *);
void REGARGS AdjustScroller (GlobData *);
void REGARGS ClearDisplayList (GlobData *);
void REGARGS UpdateDisplayList (GlobData *);
void REGARGS PrintFiles (GlobData *);
void REGARGS ScrollerMoved (GlobData *, int);
void REGARGS ClearAndInitReqBuffer (GlobData *);
void REGARGS RethinkReqDisplay (GlobData *);
void REGARGS ShowFontSample (GlobData *, int, int);
void REGARGS UnLockReqLock (GlobData *);
void REGARGS NewDir (GlobData *);
LONG REGARGS IntGadgetBounds (GlobData *, struct Gadget *, LONG, LONG);
IPTR ASM SAVEDS PropReqHandler (REGPARAM(a1, struct RealHandlerInfo *,),
				 REGPARAM(d0, ULONG,),
				 REGPARAM(a0, struct TagItem *,));

void ASM SAVEDS FreeReqBuffer (REGPARAM(a1, APTR,));
void REGARGS SetFileDirMode (struct BufferData *, ULONG);
void BuildColStr (char *, LONG, ULONG);

void REGARGS SetDrawerAndFileFields (GlobData *);
void REGARGS ResetDrawerAndFileFields (GlobData *);
#ifdef __AROS__
AROS_UFP3(void, IntuiMsgFunc,
    AROS_UFPA(struct Hook *, hook, A0),
    AROS_UFPA(APTR, req, A2),
    AROS_UFPA(struct IntuiMessage *, imsg, A1));

#else
void ASM SAVEDS IntuiMsgFunc (
	REGPARAM(a0, struct Hook *,),
	REGPARAM(a2, APTR,),
	REGPARAM(a1, struct IntuiMessage *,));
#endif

int REGARGS FindEntryPos (GlobData *, char *, int);
void REGARGS DeselectFiles (GlobData *, int, int);
int REGARGS ClickDown (GlobData *, int, struct IntuiMessage *, int);
IPTR REGARGS LeaveReq (GlobData *, char *);
void REGARGS FreeAll (GlobData *);
void REGARGS FreeAllCheckBuffer (GlobData *);
struct rtFileList *REGARGS AllocSelectedFiles (GlobData *);
int REGARGS FindEntryPos (GlobData *, char *, int);
void REGARGS StopTimer (GlobData *);
void REGARGS StartTimer (GlobData *, int);
void REGARGS EndQuiet (GlobData *);
void STDARGS SAVEDS FreeReqToolsFonts (void);
int REGARGS CalcClicked (GlobData *, struct IntuiMessage *);
void REGARGS CompClicked (GlobData *);
struct IntuiMessage *REGARGS ProcessWin_Msg_Freq (GlobData *, struct IntuiMessage *);

/****************************************************************************************/

#define REQ_IDCMP		(ARROWIDCMP|CYCLEIDCMP|SCROLLERIDCMP|STRINGIDCMP|\
				IDCMP_CLOSEWINDOW|IDCMP_DISKINSERTED|IDCMP_DISKREMOVED|\
				IDCMP_NEWSIZE|IDCMP_REFRESHWINDOW)

#define REQTYPE(req)		(((ULONG *)req)[-1])

/****************************************************************************************/

extern struct TextAttr topaz80;

/****************************************************************************************/

#define FI_REQ			struct RealFileRequester *
#define FO_REQ			struct RealFontRequester *
#define SC_REQ			struct RealScreenModeRequester *

/****************************************************************************************/
