/**************************************************************
**** Utility.c: some useful functions, by T.Pierron        ****
**** Free software under GNU license, started on 17/2/2000 ****
**************************************************************/

#define  UTILITY_C
#include <intuition/intuitionbase.h>
#include <workbench/startup.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/io.h>
#include "ClipLoc.h"
#include "Project.h"
#include "Gui.h"
#include "Utility.h"
#include "DiskIO.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS
#include "strings.h"

extern struct IntuitionBase *IntuitionBase;
extern ULONG  err_time;
static UBYTE  SPrintfBuf[80], *savea3;

/** SPrintf like routine **/


#ifdef __AROS__
#include <aros/asmcall.h>

AROS_UFH2(void, PutChProc,
    AROS_UFHA(UBYTE,    data, D0),
    AROS_UFHA(STRPTR *, p,   A3))
{
    AROS_USERFUNC_INIT

#elif defined( __GNUC__ )

void PutChProc( void )        /* Register based-argument passing with gcc */
{
	register UBYTE data __asm("d0");

#else                         /* Same proc with SAS/C */

void __asm PutChProc(register __d0 UBYTE data, register __a3 STRPTR out)
{
#endif
	/* Can't use a3 ; compiler will restore register content on exit */
	if( savea3 < SPrintfBuf + sizeof(SPrintfBuf) - 1 )
		*savea3++ = data;
	else *savea3 = 0;

#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/** This is a very simplified routine, but takes only a few hundreed of bytes **/
STRPTR my_SPrintf(STRPTR fmt, APTR data)
{
	savea3 = SPrintfBuf;
	RawDoFmt(fmt, data, (void *)PutChProc, 0);
	return SPrintfBuf;
}

STRPTR InfoTmpl = "%4ld, %5ld ";

/** Write column/line in top of window **/
void draw_info(Project p)
{
	extern WORD fginfo, bginfo;
	struct { ULONG x; ULONG y; } coord;
	struct RastPort *RP;

	coord.x = p->nbrc+1; coord.y = p->nbl+1; savea3 = SPrintfBuf;
	RawDoFmt(InfoTmpl, &coord, (void *)PutChProc, 0);

	RP = (prefs.use_pub ? &Scr->RastPort : &RPT);

	SetABPenDrMd(RP,fginfo,bginfo,JAM2);
	Move(RP,gui.xinfo,gui.yinfo);
	Text(RP, SPrintfBuf, savea3-SPrintfBuf-1);
}

/** Convert argv table into a WBArg one **/
void ParseArgs(StartUpArgs *res, int nb, char **argv)
{
	res->sa_Free   = 0;
	res->sa_NbArgs = 0;
	if( nb == 0 ) {
		/* Program has been started from Workbench */
		res->sa_NbArgs =        ((struct WBStartup *)argv)->sm_NumArgs-1;
		res->sa_ArgLst = (APTR)(((struct WBStartup *)argv)->sm_ArgList+1);
	} else if( nb > 1 ) {
		/* From CLI */
		struct WBArg *new;
		if((new = (void *) AllocVec(sizeof(*new)*(--nb), MEMF_PUBLIC | MEMF_CLEAR)))
		{
			BPTR cwd = (BPTR) CurrentDir( NULL ); /* No need to UnLock so */
			res->sa_ArgLst = (APTR) new;
			res->sa_NbArgs = nb;
			res->sa_Free   = 1;
			for(argv++; nb; new->wa_Name = *argv++, new->wa_Lock = cwd, new++, nb--);
			CurrentDir( cwd );
		}
	}
}

/** Get the filename inside #include directive **/
STRPTR GetIncludeFile(Project prj, LINE * ln)
{
	STRPTR p = ln->stream;
	LONG   i = ln->size;

	while(i && TypeChar[ *p ] == SPACE) p++, i--;
	if( i > 0 && *p == '#' )
	{
		for(p++, i--; i && TypeChar[*p] == SPACE; p++, i--);
		if( i > 7 && 0 == strncmp(p, "include", 7) )
		{
			for(p+=7, i-=7; i && TypeChar[*p] == SPACE; p++, i--);
			if(i > 2)
			{
				extern UBYTE BufTxt[];
				STRPTR dest = BufTxt;
				UBYTE  end  = *p;
				if(*p == '<') strcpy(BufTxt, "INCLUDE:"), dest+=8, end = '>';
				else if(prj->path == NULL) BufTxt[0] = 0;
				else {
					CopyMem(prj->path, BufTxt, prj->name-prj->path);
					dest += prj->name-prj->path;
				}
				for(p++, i--; i && *p != end; *dest++ = *p++, i--);
				*dest=0;
				if(RETURN_OK == get_full_path(BufTxt, &dest))
					return dest;
			}
		}
	}
	return NULL;
}

/* Generic list used ONLY as pointer */
typedef	struct _list
{
	struct _list *next, *prev;
}	*list;

/*** Insert node Src after the node It ***/
void InsertAfter( list It,list Src )
{
	register list L, Lp;
	if(It)
	{
		Lp=It; L=Lp->next;
		Src->next = L; Src->prev = Lp;
		if( L  ) L->prev = Src;
		if( Lp ) Lp->next= Src;
	}	else
		Src->next = Src->prev = NULL;
}

/*** Remove a node from a list ***/
void Destroy( list *First, list p )
{
	if(p->next) p->next->prev = p->prev;
	if(p->prev) p->prev->next = p->next;
	else *First = p->next;
}

/*** Catenate two path part ***/
STRPTR CatPath(STRPTR dir, STRPTR file)
{
	STRPTR dst;
	UWORD  len;
	if( ( dst = (STRPTR) AllocVec(len = strlen(dir) + strlen(file) + 2, MEMF_PUBLIC) ) )
		strcpy(dst, dir), AddPart(dst, file, len);
	return dst;
}

/*** MemMove: copy overlapping chunk of mem ***/
void MemMove(UBYTE *Src, UWORD Offset, LONG sz)
{
	register UBYTE *src, *dst;
	for(src=Src+sz-1, dst=src+Offset; sz>0; sz--, *dst-- = *src--);
}

static UBYTE TabStop[256], tab=255;

/*** Pre-computes tabstop ***/
void init_tabstop(UBYTE ts)
{
	if(ts != tab) {
		int i;
		for(i=0, tab=ts; i<sizeof(TabStop); i++)
		{
			TabStop[i] = tab;
			if(tab==1) tab=ts; else tab--;
		}
		tab=ts;
	}
}

/*** Returns increment up to the next tabstop ***/
UBYTE tabstop(ULONG nb)
{
	/* Almost all tabulations are situated before the 256th character */
	if(nb<=sizeof(TabStop)) return TabStop[nb];
	else return (UBYTE)(tab - (nb % tab));
}

/*** Display an error message in the window's title ***/
void ThrowError(struct Window *W, STRPTR Msg)
{
	if( W ) {
		if(Msg[0] != 127) DisplayBeep(W->WScreen); else Msg++;

		/* If window is backdrop'ed, change screen's title instead of window */
		if(W->Flags & WFLG_BACKDROP) SetWindowTitles(W,(UBYTE *)-1,Msg);
		else SetWindowTitles(W,Msg,(UBYTE *)-1);

		err_time = 0;
		/* To be sure that mesage will be disappear one day */
		ModifyIDCMP(W,W->IDCMPFlags | IDCMP_INTUITICKS);
	}	else puts(Msg);
}

/*** Show messages associated with IoErr() number ***/
void ThrowDOSError(struct Window *W, STRPTR Prefix)
{
	static UBYTE Message[100];

	/* Get standard DOS error message */
	Fault(IoErr(), Prefix, Message, sizeof(Message));

	ThrowError(W, Message);
}

/*** Set title of window/screen properly ***/
void SetTitle(struct Window *W, STRPTR Msg)
{
	/* If there is a pending msg, change hidden title */
	if( W->IDCMPFlags & IDCMP_INTUITICKS )
		ModifyIDCMP(W,W->IDCMPFlags & ~IDCMP_INTUITICKS);

	/* Modify visible title */
	if(W->Flags & WFLG_BACKDROP) SetWindowTitles(W,(UBYTE *)-1,Msg);
	else SetWindowTitles(W,Msg,(UBYTE *)-1);
	W->UserData = Msg;
}

/*** Reset the old title ***/
void StopError(struct Window *W)
{
	if(W->Flags & WFLG_BACKDROP) SetWindowTitles(W,(UBYTE *)-1,W->UserData);
	else SetWindowTitles(W,W->UserData,(UBYTE *)-1);

	/* INTUITICKS aren't required anymore */
	ModifyIDCMP(W,W->IDCMPFlags & ~IDCMP_INTUITICKS);
}

/** Getting standard busy pointer **/
ULONG IDCMPFlags;
ULONG busy_pointer_tags[] =
{
	WA_BusyPointer,TRUE,
	TAG_END
};

/*** Shutdown window IDCMP port ***/
void BusyWindow(struct Window *W)
{
	if( W )
	{
		/* Store IDCMP flags and shutdown port */
		IDCMPFlags = W->IDCMPFlags;
		ModifyIDCMP(W,0);
		/* Change window's pointer (OS 3.0+ only) */
		if(IntuitionBase->LibNode.lib_Version >= 39)
			SetWindowPointerA(W,(struct TagItem *)busy_pointer_tags);
	}
}

/*** Reset IDCMP port ***/
void WakeUp(struct Window *W)
{
	if( W )
		ModifyIDCMP(W,IDCMPFlags),
		ClearPointer(W);
}

/* Information window about current project */
struct EasyStruct request;

/*** Don't know where to put this one... ***/
void show_info(Project p)
{
	extern UBYTE Version[], WinTitle[], szEOL[];
	STRPTR file;
	ULONG  bytes;

	BusyWindow(Wnd);
	request.es_StructSize = sizeof(struct EasyStruct);
	CopyMem(MsgAbout, &request.es_Title, 3*sizeof(STRPTR));
	bytes = size_count(p->the_line, szEOL[ p->eol ]);
	split_path((AskArgs *)&p->path, NULL, &file);

	EasyRequest(Wnd,&request,0,(ULONG)WinTitle,(ULONG)(Version+sizeof(SVER)-sizeof(TARGET)),(ULONG)file,
	            p->max_lines,(ULONG)MsgAbout[ p->max_lines>1 ? 6:5 ],bytes,(ULONG)MsgAbout[ bytes>1 ? 4:3 ]);
	WakeUp(Wnd);
}

/*** Avert user that its file has been modified ***/
char warn_modif(Project p)
{
	STRPTR file;
	if( p->state & MODIFIED )
	{
		request.es_StructSize   = sizeof(struct EasyStruct);
		request.es_Title        = MsgAbout[0];
		request.es_TextFormat   = ErrMsg(ERR_FILEMODIFIED);
		request.es_GadgetFormat = ErrMsg(ERR_SLC);
		split_path((AskArgs *)&p->path, NULL, &file);
		switch( EasyRequest(Wnd,&request,0,(ULONG)file) )
		{
			case 0:  return 0;
			case 1:  return save_project(p, FALSE, FALSE);
		}
	}
	/* User want to close this file */
	return 1;
}

/*** Avert user that he is going to overwrite a file ***/
char warn_overwrite( STRPTR path )
{
	APTR lock;
	if(NULL != (lock = (APTR) Lock( path, SHARED_LOCK )))
	{
		UnLock( (BPTR) lock );
		/* Fuck'n shit, the file exists */
		request.es_StructSize   = sizeof(struct EasyStruct);
		request.es_Title        = MsgAbout[0];
		request.es_TextFormat   = ErrMsg(ERR_FILEEXISTS);
		request.es_GadgetFormat = ErrMsg(ERR_OC);

		return (char) EasyRequest(Wnd,&request,0,(IPTR)NULL);
	}
	return 1;
}

/*** Simple requester to ask user for a number ***/
int get_number( Project p, CONST_STRPTR title, LONG * result )
{
	struct Window *win;
	static UBYTE  LineNum[10];
	static struct StringInfo SI = {LineNum,NULL,0,sizeof(LineNum),0,0,0,0,0,0,NULL,0,NULL};
	static struct Gadget StrGad = {
		NULL,0,0,0,0,GFLG_GADGHCOMP,GACT_IMMEDIATE | GACT_RELVERIFY | GACT_LONGINT | GACT_STRINGCENTER,
		GTYP_STRGADGET,NULL,NULL,NULL,0,(APTR) &SI,0,NULL
	};

	/* Open our window */
	if((win = (void *) OpenWindowTags( NULL,
			WA_Width,       160,
			WA_InnerHeight, prefs.scrfont->tf_YSize+2,
			WA_Left,        Wnd->LeftEdge + (Wnd->Width  - 160) / 2,
			WA_Top,         Wnd->TopEdge  + (Wnd->Height - 30)  / 2,
			WA_Title,       (ULONG) title,
			WA_IDCMP,       IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
			WA_Flags,       WFLG_CLOSEGADGET  | WFLG_ACTIVATE | WFLG_RMBTRAP | WFLG_DRAGBAR,
			WA_PubScreen,   (ULONG) Scr,
			TAG_DONE)))
	{
		extern struct IntuiMessage msgbuf,*msg;

		BusyWindow(Wnd);
		/* Attach the simple OS1.3 compliant string gadget */
		*LineNum = 0;
		StrGad.Width    = 160 - win->BorderRight - (
		StrGad.LeftEdge = win->BorderLeft);
		StrGad.TopEdge  = win->BorderTop+1;
		StrGad.Height   = prefs.scrfont->tf_YSize;
		AddGList(win, &StrGad, 0, 1, NULL);
		ActivateGadget(&StrGad, win, NULL);

		/* Quickly collects events */
		for(;;) {

			WaitPort( win->UserPort );

			while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
			{
				CopyMemQuick(msg, &msgbuf, sizeof(msgbuf));
				ReplyMsg((struct Message *)msg);

				switch( msgbuf.Class )
				{
					case IDCMP_GADGETUP:
					case IDCMP_CLOSEWINDOW: goto the_end;
				}
			}
		}
		/* Cleanup everything */
		the_end: WakeUp(Wnd);
		CloseWindow(win);
		*result = SI.LongInt;

		if( LineNum[0] != 0 ) return 1;

	}	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
	return 0;
}
