/***************************************************************
**** prefs.c: Preference file and Inter-Process Communica-  ****
****          tion with JanoPrefs. © T.Pierron, C.Guillaume ****
**** Free software under GNU license, started on 16/4/2000  ****
***************************************************************/

#define	ASL_V38_NAMES_ONLY
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/intuitionbase.h>
#include <libraries/asl.h>
#include <libraries/iffparse.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <stddef.h>				/* offsetof() */
#include "Jed.h"
#include "IPC_Prefs.h"
#include "Utility.h"
#include "Events.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS
#include "strings.h"

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *      GfxBase;
extern struct Library *      AslBase;
extern struct Library *      IFFParseBase;

PREFS prefs, tmpprefs;
UBYTE File[]   = APPNAME ".prefs";
UBYTE ENV[]    = "ENVARC:";
UBYTE Path[100];

/** Special table for word separation **/
UBYTE WordsSep[MAX_SPLIT] = "!-/:-?[-]^`{-¿×÷";
UBYTE SpaceType[]         = "\t\n\r \177";
UBYTE TypeChar[256];

/** Default pens number **/
struct pens DefaultPens = {
	BACKGROUNDPEN, TEXTPEN, FILLPEN, HIGHLIGHTTEXTPEN, SHINEPEN, TEXTPEN,
	SHINEPEN, SHADOWPEN, BACKGROUNDPEN, TEXTPEN, -4, BACKGROUNDPEN
};

/** Which part of editor to modify according to color changes **/
UBYTE Modif[] = {
	EDIT_AREA, EDIT_AREA, EDIT_AREA, EDIT_AREA, 0, 0, EDIT_GUI, EDIT_GUI,
	EDIT_GUI, EDIT_GUI, EDIT_GUI, EDIT_GUI
};

/** Little wrapper **/
#define	OFFS(x)		(UBYTE)(offsetof(PREFS,x))

/** Offset of structure PREFS **/
UBYTE offsets[] = {
	OFFS(use_pub), OFFS(wordssep),         OFFS(attrtxt), OFFS(attrtxt.ta_YSize),
	OFFS(attrscr), OFFS(attrscr.ta_YSize), OFFS(left),    OFFS(scrw),
	OFFS(scrd),    OFFS(modeid),           OFFS(vmd),     OFFS(pen)
};
/** And correspond size (0=null-terminated string) **/
UBYTE sizefields[] = {
	12*sizeof(char),  0, 0, sizeof(prefs.attrtxt)-sizeof(STRPTR), 0,
	sizeof(prefs.attrscr)-sizeof(STRPTR), 4*sizeof(prefs.left), sizeof(prefs.scrw),
	sizeof(prefs.scrd), sizeof(prefs.modeid), sizeof(prefs.vmd), sizeof(prefs.pen)
};

UBYTE FontName[60];

/*** Convert a TextFont struct into a TextAttr ***/
void text_to_attr(struct TextFont *src, struct TextAttr *dest)
{
	dest->ta_Name  = src->tf_Message.mn_Node.ln_Name;
	dest->ta_YSize = src->tf_YSize;
	dest->ta_Style = FS_NORMAL;
	dest->ta_Flags = src->tf_Flags;
}

/*** Extract some information from a Screen structure ***/
void info_screen(PREFS *prefs, struct Screen *Scr)
{
	prefs->parent = Scr;
	prefs->scrw   = Scr->Width;
	prefs->scrh   = Scr->Height;
	prefs->vmd    = GetVPModeID( &Scr->ViewPort );
	prefs->scrd   = Scr->RastPort.BitMap->Depth;
}

/*** Unpack separators description string ***/
void unpack_separators(UBYTE *Fmt)
{
	UBYTE *src,a,b;
	memset(TypeChar,ALPHA,sizeof(TypeChar));
	for(src=Fmt; *src; src++)
	{
		/* Avoid special character meaning */
		if(*src == '\\' && src[1]) src++;
		TypeChar[*src] = SEPARATOR;
		/* Range of char specifier? */
		if(src[1] == '-' && *src<src[2])
			for(a=*src,b=src[2]; a<=b; a++)
				TypeChar[a] = SEPARATOR;
	}
	/* Force space char type */
	for(src=SpaceType; *src; src++)
		TypeChar[*src] = SPACE;
}

/*** Set preference to default settings ***/
void set_default_prefs( PREFS *prefs, struct Screen *def )
{
	memset(prefs, 0, sizeof(*prefs));
	CopyMem(&DefaultPens, &prefs->pen, sizeof(prefs->pen));
	prefs->auto_indent = 1;
	prefs->matchcase   = 1;
	prefs->tabsize = 8;
	prefs->scrfont = def->RastPort.Font;
	prefs->width   = def->Width-(prefs->left<<1);
	prefs->top     = (def->Height - (
	prefs->height  = 27 * (
	prefs->txtfont = GfxBase->DefaultFont)->tf_YSize)) >> 1;
	prefs->modeid  = PAL_MONITOR_ID | HIRES_KEY;

	/* Convert dynamic struct to static one */
	text_to_attr(prefs->scrfont, &prefs->attrscr);
	text_to_attr(prefs->txtfont, &prefs->attrtxt);
	unpack_separators(prefs->wordssep = WordsSep);
	info_screen(prefs, def);
	init_tabstop(8);
}

/* Open preference file according to `mode' */
APTR open_prefs(STRPTR file, UBYTE mode)
{
	struct IFFHandle * pref;

	if( IFFParseBase != NULL && (pref = (APTR) AllocIFF() ) )
	{
		BPTR fh = 0;
		switch( mode )
		{
			case MODE_USE:
				if(file == NULL)
				{
					/* First: search in local directory */
					CopyMem(File, Path, sizeof(File)-1);
					if(NULL == (fh = Open( Path, MODE_OLDFILE )))
					{
						/* Otherwise, look in directory ENVARC */
						CopyMem(ENV, Path,sizeof(ENV)-1);
						CopyMem(File,Path+sizeof(ENV)-1,sizeof(File)-1);
						fh = Open( Path, MODE_OLDFILE );
					}
				} else if(NULL != (fh = Open( file, MODE_OLDFILE )))
					strcpy(Path, file);
				break;
			case MODE_SAVE:
				fh = Open(Path, MODE_NEWFILE);
		}
		/* Did we have a opened file? */
		if( fh )
		{
			pref->iff_Stream = (IPTR) fh;
			/* Use DOS function for accessing it */
			InitIFFasDOS( pref );
			/* Open it through iffparse */
			if( !OpenIFF( pref, mode == MODE_SAVE ? IFFF_WRITE : IFFF_READ) )
				return pref;
		}
		FreeIFF( pref );
	}
	return NULL;
}

/*** Close properly IFF handle ***/
void close_prefs( struct IFFHandle * file )
{
	CloseIFF( file );
	if( file->iff_Stream ) Close((BPTR) file->iff_Stream );
	FreeIFF( file );
}

/*** Try to load a preference file ***/
UBYTE load_prefs(PREFS *prefs, STRPTR filename)
{
	APTR  file;
	UBYTE err = RETURN_OK;

	/* Locate preference file */
	if( (file = open_prefs(filename, MODE_USE)) )
	{
		/* Search for PREF/JANO chunk in this file */
		if( !StopChunk(file, ID_PREF, ID_JANO) )
		{
			if( !ParseIFF(file, IFFPARSE_SCAN) )
			{
				struct ContextNode * cn = CurrentChunk(file);
				STRPTR buffer   = NULL;
				UWORD  ByteRead = 0;

				if( cn->cn_Type == ID_PREF && cn->cn_ID == ID_JANO           &&
				   (buffer = (STRPTR) AllocVec(cn->cn_Size, MEMF_PUBLIC))    &&
				    ReadChunkBytes(file, buffer, cn->cn_Size) == cn->cn_Size )
				{
					/* He have read the file, converts it into PREFS struct */
					memset(prefs, 0, sizeof(*prefs));
					prefs->wordssep = WordsSep;
					prefs->attrtxt.ta_Name = FontName;
					prefs->attrscr.ta_Name = FontName+30;
					while(ByteRead < cn->cn_Size)
					{
						register STRPTR src;
						src = buffer + ByteRead;
						if(src[0] < MAX_NUMFIELD) {
							register STRPTR dest = (STRPTR)prefs+offsets[*src];
							if(sizefields[ *src ] == 0) dest = *(STRPTR *)dest;
							CopyMem(src+2, dest, src[1]);
						}
						ByteRead += src[1]+2;
					}
				} else err = RETURN_FAIL;
				if(buffer != NULL) FreeVec( buffer );
			} else err = RETURN_FAIL;
		} else err = RETURN_FAIL;
		close_prefs(file);
	} else err = RETURN_FAIL;

	if(err == RETURN_OK)
	{
		info_screen(prefs, IntuitionBase->ActiveScreen);

		/* If user wants to use a custom font for its interface, try lo **
		** load it, otherwise use default screen font of parent screen: */
		if(!prefs->use_scrfont ||
		   !(prefs->scrfont = (void *) OpenDiskFont( &prefs->attrscr )) )
			prefs->scrfont = prefs->parent->RastPort.Font;
		/* Ditto with text font */
		if(!prefs->use_txtfont ||
		   !(prefs->txtfont = (void *) OpenDiskFont( &prefs->attrtxt )) )
			prefs->txtfont = GfxBase->DefaultFont;
		/* Makes valid pointers */
		text_to_attr(prefs->scrfont, &prefs->attrscr);
		text_to_attr(prefs->txtfont, &prefs->attrtxt);
		/* Special characters that separate words */
		unpack_separators(prefs->wordssep);
	}
	else set_default_prefs(prefs, IntuitionBase->ActiveScreen);
	/* All done */
	return err;
}

/*** Save a file where we found it, otherwise in ENVARC: ***/
UBYTE save_prefs(PREFS *prefs)
{
	APTR  file;
	UBYTE num, size;
	UBYTE NumField[2];
	if( (file = open_prefs(Path, MODE_SAVE)) )
	{
		if( !PushChunk(file, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN) )
		{
			if( !PushChunk(file, ID_PREF, ID_JANO, IFFSIZE_UNKNOWN) )
			{	 
				/* Save window dimension */
				CopyMem(&Wnd->LeftEdge, &prefs->left, 4*sizeof(WORD));
				/* Write configuration file */
				for(num=0; num < MAX_NUMFIELD; num++) {
					register STRPTR src;
					size = sizefields[ num ];
					src  = (STRPTR)prefs + offsets[ num ];
					if(size == 0) src = *(STRPTR *)src, size = strlen(src)+1;

					NumField[0] = num; NumField[1] = size;
					if( WriteChunkBytes(file, NumField, 2) != 2   ||
					    WriteChunkBytes(file, src,   size) != size ) break;
				}
				PopChunk( file );
			}
			PopChunk( file );
		}
		close_prefs(file);
	}
	return 0;
}

/*** Ask for a new font, fixed or not ***/
struct TextFont *change_fonts(struct TextAttr *buf, void *Wnd, BOOL fixed)
{
	struct FontRequester *fr;
	struct TextFont *newfont = NULL;

	if((fr = (void *) AllocAslRequestTags(ASL_FontRequest,
				ASLFO_FixedWidthOnly, fixed,
				ASLFO_SleepWindow,    TRUE,
				ASLFO_InitialName,    (ULONG)buf->ta_Name,
				ASLFO_InitialSize,    buf->ta_YSize,
				ASLFO_Window,         (ULONG)Wnd,
				TAG_DONE)))
	{
		if( AslRequest(fr, NULL) )
		{
			/* User may hit cancel! */
			newfont = (void *) OpenDiskFont( &fr->fo_Attr );

			if( newfont )
			{
				CopyMem(&fr->fo_Attr, buf, sizeof(*buf));
				/* The ta_Name field will be freed with FreeAslRequest call ! */
				buf->ta_Name = newfont->tf_Message.mn_Node.ln_Name;
			}
			else
				ThrowError(Wnd, ErrMsg(ERR_LOADFONT));
		}
		FreeAslRequest(fr);
		/* Window will be reinitiated later... */
	}
	return newfont;
}

/*** Ask for new screen information ***/
#ifndef	JANOPREF
ULONG change_screen_mode(UBYTE *Depth, ULONG ModeID)
#else
/*** JanoPrefs wants some additionnal information ***/
ULONG change_screen_mode(WORD *whd, ULONG ModeID)
#endif
{
	struct ScreenModeRequester *smr;

	if((smr = (void *) AllocAslRequestTags(ASL_ScreenModeRequest,
					ASLSM_DoWidth,          FALSE,
					ASLSM_DoHeight,         FALSE,
					ASLSM_DoAutoScroll,     FALSE,
					ASLSM_DoOverscanType,   FALSE,
					ASLSM_DoDepth,          TRUE,
					ASLSM_InitialDisplayID, ModeID,
					ASLFR_Screen,	        (ULONG)Scr,
					TAG_DONE) ))
	{
		if( AslRequest(smr,NULL) )
		{
			/* Extract some interresting information about screen */
#ifndef	JANOPREF
			*Depth = smr->sm_DisplayDepth;
#else
			whd[0] = smr->sm_DisplayWidth;
			whd[1] = smr->sm_DisplayHeight;
			whd[2] = smr->sm_DisplayDepth;
#endif
			ModeID = smr->sm_DisplayID;
		} else ModeID = INVALID_ID;
		FreeAslRequest(smr);
		return ModeID;
	}
	return INVALID_ID;
}

#ifndef	JANOPREF       /** Following functions are used only by the editor **/
#include "DiskIO.h"

/*** Ask where to load/save a preference file ***/
void ask_prefs(Project edit, char save, CONST_STRPTR title )
{
	STRPTR  new;
	AskArgs arg;

	arg.file = (STRPTR) FilePart( arg.dir = Path );
	arg.modifmark = 0;

	if((new = (STRPTR) (save ? ask_save(Wnd, &arg, title) : ask_load(Wnd, &arg, TRUE, title))))
	{
		CopyMemQuick(new, Path, sizeof(Path));
		FreeVec(new);
		if(save) save_prefs(&prefs);
		else if( load_prefs(&tmpprefs, Path) != RETURN_OK )
			ThrowError(Wnd, ErrMsg(ERR_BADPREFSFILE));
		else update_prefs(edit);
	}
}

/*** Ask user for a new font (shortcut of gui) ***/
void ask_new_font( void )
{
	struct TextFont *newfont;

	if((newfont = change_fonts(&prefs.attrtxt, Wnd, TRUE)))
	{
		if( prefs.txtfont ) CloseFont(prefs.txtfont);
		prefs.use_txtfont = TRUE;
		prefs.txtfont     = newfont;
		/* Redraw interface */
		SetFont(RP, prefs.txtfont);
		new_size(EDIT_AREA);
	}
}

/*** Change screenmode (shortcut of gui) ***/
void ask_new_screen( void )
{
	ULONG ModeID;
	if((ModeID = change_screen_mode(&prefs.depth, prefs.modeid)) != INVALID_ID )
	{
		prefs.modeid  = ModeID;
		prefs.use_pub = TRUE;
		/* Close everything */
		CloseMainWnd(1);
		if( setup() ) cleanup(ErrMsg(ERR_NOGUI), RETURN_FAIL);
		new_size(EDIT_ALL);
	}
}

/*** Close the pref window, making or not changes effective ***/
void update_prefs( Project edit )
{
	/* Let's change the settings: */
	UBYTE flags = 0, col = 0, i;
	/* We are going to take in account changes wanted by the user. **
	** This can imply some deep changes in the interface, try to   **
	** limit them as possible (i.e:closing screen or window).      */

	/* First, be sure that "parent" screen already exists */
	if(prefs.use_pub == 0)
	{
		register struct Screen *list, *first;

		for(first = list = IntuitionBase->ActiveScreen; list && list != first; list=list->NextScreen)
			if(list == tmpprefs.parent) {
				first = NULL; break;
			}

		/** The screen hasn't been found! **/
		if(first && NULL != (list = (void *) LockPubScreen(NULL)))
			UnlockPubScreen(NULL, list);

		tmpprefs.parent = list;
	}

	/* Want to change the screen where the window is ? */
	if(prefs.use_pub != tmpprefs.use_pub ||
	  (tmpprefs.use_pub==1 && tmpprefs.modeid!=prefs.modeid) ||
	  (prefs.scrfont != tmpprefs.scrfont && prefs.use_pub))
		/* Close everything in this case */
		CloseMainWnd(1), flags = EDIT_ALL;
	else if(prefs.backdrop != tmpprefs.backdrop)
		/* Otherwise just the window */
		CloseMainWnd(0), flags = EDIT_ALL;
	else
		/* User has changed the screen font, but the window re- **
		** mains on a pubscreen. Therefore, it can't be closed, **
		** but change the font that our interface uses anyway:  */
		if(prefs.scrfont != tmpprefs.scrfont)
			/* Just compute new menu size */
			flags = EDIT_GUI;

	/* Look for color changes */
	for(i=0; i<sizeof(Modif); i++)
		if( (&prefs.pen.bg)[i] != (&tmpprefs.pen.bg)[i] ) col |= Modif[i];

	/* Text font has changed, no shutting required */
	if( prefs.txtfont != tmpprefs.txtfont ) flags |= EDIT_AREA;
	    
	/* Tabstop changed, need to recompute precomputed tab */
	if(edit->tabsize != tmpprefs.tabsize)
		init_tabstop(tmpprefs.tabsize), flags |= EDIT_AREA,
		edit->tabsize = tmpprefs.tabsize;

	/* Makes changes effective */
	CopyMem(&tmpprefs, &prefs, sizeof(prefs));
	unpack_separators((STRPTR)strcpy(prefs.wordssep = WordsSep, tmpprefs.wordssep));

	/* Requires to setup the main gui? */
	if(flags & EDIT_GUI)
	{
		if( setup() ) cleanup(ErrMsg(ERR_NOGUI), RETURN_FAIL);
		else          new_size(flags | col);
	} else {
		/* Pens number has changed? */
		if( col ) load_pens();
		/* It isn't required to "reboot" the programme! */
		SetFont(RP, prefs.txtfont);
		new_size(flags | col);
	}
}

/*** Find and launch JanoEditor preference tool ***/
void setup_winpref( void )
{
	static UBYTE JPPath[] = SYS_DIR PREF_DIR PREF_NAME " >NIL:";
	static IPTR  systags[] = {
		SYS_Input,   (IPTR) NULL,
		SYS_Output,  (IPTR) NULL,
		SYS_Asynch,         TRUE,
		TAG_DONE
	};
	struct FileLock *lock;
	UBYTE  *path;

	/* The pref may be already running */
	if( !send_pref(&prefs, CMD_SHOW) )
	{
		JPPath[ sizeof(SYS_DIR PREF_DIR PREF_NAME)-1 ] = 0;
		/* Search the preference editor in various places */
		if( !(lock = (void *) Lock(path = JPPath+sizeof(SYS_DIR PREF_DIR)-1, SHARED_LOCK)) &&
		    !(lock = (void *) Lock(path = JPPath+sizeof(SYS_DIR)-1,          SHARED_LOCK)) &&
		    !(lock = (void *) Lock(path = JPPath,                            SHARED_LOCK)) )
			/* But maybe it is in the path */
			path = JPPath+sizeof(SYS_DIR PREF_DIR)-1;

		if(lock) UnLock((BPTR)lock);
		JPPath[ sizeof(SYS_DIR PREF_DIR PREF_NAME)-1 ] = ' ';

		/* Let's spawn the new process, the pref will be clever enough to **
		** ask Jano for it's internal preference it is currently using.   */
		if(SystemTagList(path, (struct TagItem *)systags) != 0)
			ThrowError(Wnd, ErrMsg(ERR_NOPREFEDITOR));
	}
}
#endif
