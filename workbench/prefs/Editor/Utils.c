/********************************************************************
**** Utils.c: Some useful functions that doesn't change too much ****
**** Free software under GNU license, started on 11/11/2000      ****
**** © T.Pierron, C.Guillaume.                                   ****
********************************************************************/


#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/gadtools.h>

#include <string.h>
#include <stdlib.h>

#include "Jed.h"
#include "JanoPrefs.h"
#include "Sample.h"

#define CATCOMP_NUMBERS
#include "../../tools/Edit/strings.h"

extern struct Window *Wnd;
struct FileRequester *fr=NULL;
static PREFS oldprefs;

void ThrowError(struct Window *W, UBYTE *Msg);
void ThrowDOSError(struct Window *W, STRPTR Prefix, UBYTE err);

#define	GetSI(gad)			((struct StringInfo *)gad->SpecialInfo)

/*** Convert number to dec ***/
WORD AddNum(ULONG nb, STRPTR buf)
{
	static UBYTE temp[10];
	UBYTE *dst;
	for(dst=temp+9; nb>=10; nb/=10)		/* Mouais :-\ */
		*dst--= nb%10+'0';
	*dst=nb+'0';
	CopyMem(dst,buf,temp+10-dst);
	return (WORD)(temp+10-dst);
}


/*** Measure the maximal lenght of a NULL-terminated array of string: ***/
WORD meas_table(UBYTE **strings)
{
	extern struct RastPort RPT;
	register UBYTE **p;
	register WORD  maxlen,len;

	for(p=strings, maxlen=0; *p; p++)
		if(maxlen < (len=TextLength(&RPT,*p,strlen(*p)))) maxlen = len;

	return maxlen;
}

/*** Extract some information of a TextFont struct ***/
void font_info(UBYTE *buf, struct TextFont *fnt)
{
	UBYTE *name = fnt->tf_Message.mn_Node.ln_Name;
	UWORD size;

	/* Fontname / Height */
	size = strlen(name)-5;		/* -".font" */
	CopyMem(name,buf,size); buf+=size;
	*buf++='/'; buf += AddNum(fnt->tf_YSize, buf);
	*buf = 0;
}

/*** Same job with a (struct Screen *) ***/
void scr_info(UBYTE *buf, WORD Width, WORD Height, WORD Depth)
{
	/* Width x Height x Depth */
	buf += AddNum(Width,  buf); *buf++='x';
	buf += AddNum(Height, buf); *buf++='x';
	buf += AddNum(Depth,  buf); *buf=0;
}

/*** Try to load an already loaded font ***/
struct TextFont *get_old_font( STRPTR fmt )
{
	static UBYTE FontName[50];
	STRPTR p;
	for(p=fmt; *p && *p!='/'; p++);
	if(*p == '/')
	{
		struct TextAttr font;
		CopyMem(fmt,FontName,p-fmt); strcpy(FontName+(p-fmt),".font");
		font.ta_Name  = FontName;
		font.ta_YSize = atoi(p+1);
		font.ta_Style = FS_NORMAL;
		font.ta_Flags = FPF_DISKFONT;
		return (struct TextFont *)OpenDiskFont(&font);
	}
	return NULL;
}

/*** Be sure a window fits in a screen ***/
void fit_in_screen(struct NewWindow *wnd, struct Screen *scr)
{
	/* Adjust left edge and width of window */
	if(wnd->LeftEdge + wnd->Width > scr->Width)
		wnd->LeftEdge = scr->Width - wnd->Width;

	if(wnd->LeftEdge < 0) wnd->LeftEdge=0, wnd->Width=scr->Width;
	
	/* Adjust top edge and height */
	if(wnd->TopEdge + wnd->Height > scr->Height)
		wnd->TopEdge = scr->Height - wnd->Height;

	if(wnd->TopEdge < 0) wnd->TopEdge=0, wnd->Height=scr->Height;
}


/*** Performs some checks on what user has enterred ***/
void check_tab(struct Gadget *str)
{
	UBYTE *buf = GetSI(str)->Buffer, *start;

	for(start=buf; *buf; buf++)
		if(*buf < '0' || *buf > '9')
		{
			/* Wrong char, avert user */
			GetSI(str)->BufferPos = buf-start;
			DisplayBeep(NULL);
			ActivateGadget(str, Wnd, NULL);
			break;
		}
}

/*** Update gui components ***/
void show_changes(PREFS *old, PREFS *new)
{
	extern struct Gadget *gads[];
	extern struct Screen *Scr;
	extern UBYTE StrInfo[60],Modif[];

	/* Tabulation */
	if(old->tabsize != new->tabsize)
	{
		StrInfo[ AddNum(new->tabsize, StrInfo) ] = 0;
		GT_SetGadgetAttrs(gads[0], Wnd, NULL, GTST_String, StrInfo, TAG_DONE);
	}
	GT_SetGadgetAttrs(gads[1], Wnd, NULL, GTST_String, new->wordssep, TAG_DONE);
	{
		extern struct TagItem TextFontTags[], ScrFontTags[], ScrMdTags[];
		extern ULONG extended;

		ScrFontTags[1].ti_Data = 0;
		ScrMdTags[1].ti_Data = 0;
		TextFontTags[1].ti_Data = 0; extended=0;
		if( new->use_txtfont )
			font_info(StrInfo, new->txtfont),
			TextFontTags[0].ti_Data = (ULONG) FTCycTxt, extended |= 1;
		else
			TextFontTags[0].ti_Data = (ULONG) (FTCycTxt+1);

		if( new->use_scrfont )
			font_info(StrInfo+20, new->scrfont),
			ScrFontTags[0].ti_Data = (ULONG) FSCycTxt, extended |= 2;
		else
			ScrFontTags[0].ti_Data = (ULONG) (FSCycTxt+1);

		if( new->use_pub==1 )
			scr_info(StrInfo+40,Scr->Width, Scr->Height, Scr->RastPort.BitMap->Depth),
			ScrMdTags[0].ti_Data = (ULONG) ScrCycTxt, extended |= 4;
		else {
			ScrMdTags[0].ti_Data = (ULONG) (ScrCycTxt+1);
			if(new->use_pub!=2) ScrMdTags[1].ti_Data = 1; /* Clone */
		}

		/* Show changes */
		GT_SetGadgetAttrsA(gads[2],Wnd,NULL,TextFontTags);
		GT_SetGadgetAttrsA(gads[3],Wnd,NULL,ScrFontTags);
		GT_SetGadgetAttrsA(gads[4],Wnd,NULL,ScrMdTags);
	}
	{
		register int i, col;
		for(i=0; i<CBS; i++)
			GT_SetGadgetAttrs(gads[CGS+i], Wnd, NULL, GTCB_Checked, (&new->backdrop)[i], TAG_DONE);

		for(col=i=0; i<sizeof(new->pen); i++)
			if( (&new->pen.bg)[i] != (&old->pen.bg)[i] ) col |= Modif[i];

		if(old->scrfont != new->scrfont) col |= EDIT_GUI;
		if(old->txtfont != new->txtfont) col |= EDIT_AREA;
		render_sample(Wnd, col);
	}
}

/* ASL load or save requester tags: */
static IPTR tags[] = {
	ASLFR_InitialLeftEdge,50,
	ASLFR_InitialTopEdge,50,
	ASLFR_InitialWidth,320,
	ASLFR_InitialHeight,256,
	ASLFR_Window,0,
	ASLFR_Flags1,0,
	ASLFR_SleepWindow,TRUE,
	ASLFR_InitialDrawer,(IPTR)"ENVARC:",
	ASLFR_InitialFile,0,
	ASLFR_InitialPattern,(IPTR)"#?",
	TAG_DONE
};

static UBYTE TempPath[100];
extern UBYTE Path[100];

/*** Save width and height of an ASL requester ***/
void save_asl_dim(struct FileRequester *fr)
{
	tags[1] = fr->fr_LeftEdge;
	tags[3] = fr->fr_TopEdge;
	tags[5] = fr->fr_Width;
	tags[7] = fr->fr_Height;
}

/*** Ask user for a new preference file to load ***/
void load_pref(PREFS *prefs)
{
	if(fr==NULL && !(fr = (void *)AllocAslRequest(ASL_FileRequest,NULL)))
		ThrowError(Wnd, ErrMsg(ERR_NOASLREQ));
	else
	{
		tags[11] = FILF_PATGAD;
		/* Save old preferences */
		CopyMem(prefs, &oldprefs, sizeof(oldprefs));
		if(AslRequest(fr, (struct TagItem *)tags))
		{
			UBYTE errcode;
			save_asl_dim(fr);
			CopyMem(Path,      TempPath, sizeof(Path));
			CopyMem(fr->fr_Drawer, Path, sizeof(Path));
			AddPart(Path,   fr->fr_File, sizeof(Path));
			switch( errcode = load_prefs(prefs, Path) )
			{
				case RETURN_OK:
					show_changes(&oldprefs, prefs),
					SetWindowTitles(Wnd, Path, (STRPTR)-1);
					return;
				case ERROR_OBJECT_WRONG_TYPE:
					ThrowError(Wnd, ErrMsg(ERR_BADPREFSFILE)); break;
				default: ThrowDOSError(Wnd, Path, errcode);
			}
			/* Restore previous path */
			CopyMem(TempPath, Path, sizeof(Path));
		}
	}
}

/*** Ask user for a place to save the preference file ***/
void save_pref_as(PREFS *prefs)
{
	if(fr==NULL && !(fr = (void *)AllocAslRequest(ASL_FileRequest,NULL)))
		ThrowError(Wnd, ErrMsg(ERR_NOASLREQ));
	else
	{
		tags[11] = FILF_SAVE | FILF_PATGAD;
		if(AslRequest(fr, (struct TagItem *)tags))
		{
			save_asl_dim(fr);
			CopyMem(fr->fr_Drawer, Path, sizeof(Path));
			AddPart(Path,   fr->fr_File, sizeof(Path));
			save_prefs( prefs );
			SetWindowTitles(Wnd, Path, (STRPTR)-1);
		}
	}
}

/*** Set default preference ***/
void default_prefs(PREFS *prefs)
{
	PREFS oldprefs;

	CopyMem(prefs, &oldprefs, sizeof(oldprefs));
	set_default_prefs(prefs, prefs->parent);
	show_changes(&oldprefs, prefs);
}

static char edit_file;
static char buffer[ MAX(sizeof(Path),sizeof(prefs)) ];

/*** Save current configuration to restore it later if desired ***/
void save_config( char ConfigFile )
{
#warning "CHECKME: = or ==?"

	if(edit_file = ConfigFile)
		CopyMem(Path, buffer, sizeof(Path));
	else
		CopyMem(&prefs, buffer, sizeof(prefs));
}

/*** Restore config ***/
void restore_config(PREFS *prefs)
{
	PREFS oldprefs;
	CopyMem(prefs, &oldprefs, sizeof(oldprefs));
	if( edit_file == 0 )
	{
		/* The preferences come from Jano */
		CopyMem(buffer, prefs, sizeof(*prefs));
	}
	else
	{
		/* We have edited a static file */
		if( load_prefs(prefs, buffer) == RETURN_OK )
			SetWindowTitles(Wnd, Path, (STRPTR)-1);
		else
			set_default_prefs(prefs, prefs->parent);
	}
	show_changes(&oldprefs, prefs);
}

void free_asl( void ) { FreeAslRequest( fr ); }
