/**********************************************************
**                                                       **
**      $VER: ClipLoc.c 1.2 (10 dec 1999)                **
**      RKM Locale library and clipboard support.        **
**                                                       **
**      © T.Pierron, C.Guilaume. Free software under     **
**      terms of GNU public license.                     **
**                                                       **
**********************************************************/

#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/locale.h>
#include <exec/memory.h>
#include <exec/io.h>
#include "Memory.h"
#include "ClipLoc.h"
#include "Version.h"
#include "Cursor.h"
#include "Utility.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS		/* We will need the string id */
#define  CATCOMP_STRINGS		/* and the english string corresponding to the id */
#include "strings.h"

#define ID_FTXT	MAKE_ID('F','T','X','T')
#define ID_CHRS	MAKE_ID('C','H','R','S')

/** Main clipboard handle manage through iffparse.library **/
struct IFFHandle * clip = NULL;

/** All messages of JanoEditor **/
STRPTR JanoMessages[] = {

	/* Global error messages */
	ERR_BADOS_STR,         ERR_NOASLREQ_STR,
	ERR_NOMEM_STR,         ERR_NOGUI_STR,
	ERR_NONAME_STR,        ERR_WRITECLIP_STR,
	ERR_OPENCLIP_STR,      ERR_LOADFILE_STR,
	ERR_NOTXTINCLIP_STR,   ERR_WRONG_TYPE_STR,
	ERR_READCLIP_STR,      ERR_NOBRACKET_STR,
	ERR_NOT_FOUND_STR,     ERR_LOADFONT_STR,
	ERR_NOPREFEDITOR_STR,  ERR_BADPREFSFILE_STR,
	ERR_FILEMODIFIED_STR,  ERR_SLC_STR,
	ERR_NOSEARCHTEXT_STR,  ERR_FILEEXISTS_STR,
	ERR_OC_STR,            WARN_RECORD_STR,
	WARN_RECORDED_STR,     WARN_REC_STR,

	/* About messages */
	MSG_ABOUT_STR,         MSG_FORMATINFO_STR,
	MSG_CONTINUE_STR,      MSG_BYTE_STR,
	MSG_BYTES_STR,         MSG_LINE_STR,
	MSG_LINES_STR,

	/* Search window messages */
	MSG_SEARCHWINDOW_STR,  MSG_REPLACEWINDOW_STR, MSG_SEARCHSTRING_STR,
	MSG_REPLACESTRING_STR, MSG_OPTCASE_STR,       MSG_OPTWORDS_STR,
	MSG_BUTTONREPLACE_STR, MSG_BUTTONREPALL_STR,  MSG_BUTTONSEARCH_STR,
	MSG_NEXTSEARCH_STR,    MSG_USESEARCH_STR,     MSG_CANCELSEARCH_STR, NULL,
	MSG_REPLACEALL_STR,    MSG_OCCURENCY_STR,     MSG_OCCURENCIES_STR
};

/*** Open the clipboard using iffparse library***/
static BOOL CBOpen(ULONG unit)
{
	extern struct Library * IFFParseBase;

	if( IFFParseBase != NULL && (clip = (APTR) AllocIFF() ) )
	{
		if( (clip->iff_Stream = (IPTR) OpenClipboard(unit)) )
		{
			InitIFFasClip(clip);

			return TRUE;
		}
		FreeIFF( clip );
	}
	ThrowError(Wnd, ErrMsg(ERR_OPENCLIP));
	return FALSE;
}

/*** Close clipboard ***/
void CBClose( void )
{
	if( clip )
	{
		CloseClipboard( (APTR) clip->iff_Stream );
		FreeIFF(clip);
	}
}

/*** Write strings to the clipboard.device ***/
BOOL CBWriteFTXT( LINE *stsel, struct cutcopypast *ccp )
{
	LINE *ln;
	long  length;

	/* If clipboard not already allocated, makes it now */
	if( clip == NULL && !CBOpen(STD_CLIP_UNIT) ) return FALSE;

	/* Compute how many chars are selected */
	for(length=0, ln=stsel; ln && ln->flags; ln=ln->next)
	{
		length += ln->size;
		/* In block selection mode, always add a newline */
		if(ccp->select == COLUMN_TYPE) length++;
		if(ln->flags & FIRSTSEL) length -= find_nbc(ln, ccp->startsel);
		if(ln->flags & LASTSEL)  length += find_nbc(ln, ccp->endsel)-ln->size;
		else length++; /* New line */
	}
	if(length == 0) return FALSE;

	if( !OpenIFF(clip, IFFF_WRITE) )
	{
		/* Let iffparse manage main chunk size */
		if( !PushChunk(clip, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN) )
		{
			if( !PushChunk(clip, ID_FTXT, ID_CHRS, length) )
			{
				/* In columnar selection, use '\r' as eol */
				char eol = (ccp->select == COLUMN_TYPE ? '\r':'\n'), add_eol;

				/* Write the content of strings */
				for(ln = stsel; ln && ln->flags; ln = ln->next)
				{
					STRPTR stream;
					ULONG  length;
					stream = (STRPTR)ln->stream;
					length = ln->size;
					if(ln->flags & FIRSTSEL) {
						register long rc = find_nbc(ln, ccp->startsel);
						length -= rc; stream += rc;
					}
					if(ln->flags & LASTSEL) {
						length += find_nbc(ln, ccp->endsel) - ln->size;
						/* Add automatically a newline */
						add_eol = (ccp->select == COLUMN_TYPE);
					}
					else add_eol = 1;

					WriteChunkBytes(clip, stream, length);
					if(add_eol)
						WriteChunkBytes(clip, &eol, 1);
				}
				PopChunk(clip);
			}
			else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
			PopChunk(clip);
		}
		else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
		CloseIFF(clip);
	}
	return TRUE;
}

/*** Reads the next CHRS chunk from clipboard ***/
BOOL CBReadCHRS( void *jbuf, LINE *st, ULONG pos, LONG *nbl )
{
	struct ContextNode * cn;
	BOOL   ret = FALSE;

	/* If clipboard not already allocated, makes it now */
	if( clip == NULL && !CBOpen(STD_CLIP_UNIT) ) return FALSE;

	if( !OpenIFF(clip, IFFF_READ) )
	{
		if( !StopChunk(clip, ID_FTXT, ID_CHRS) )
		{
			if( !ParseIFF(clip, IFFPARSE_SCAN) )
			{
				cn = CurrentChunk(clip);
				if( cn->cn_Type == ID_FTXT && cn->cn_ID == ID_CHRS && cn->cn_Size > 0 )
				{
					STRPTR buf;
					ULONG  size = cn->cn_Size;

					if( (buf = (STRPTR) AllocVec(size, MEMF_PUBLIC)) )
					{
						UBYTE eol;
						ReadChunkBytes(clip, buf, size);

						/* What's kind of paste method shall we used? */
						{	register STRPTR s; register ULONG n;
							for(s=buf,n=size; --n && *s!='\n' && *s!='\r'; s++);
							eol = *s;
						}
						/* Add string to the buffer */
						reg_group_by(jbuf);
						if(eol == '\n') add_string(jbuf,st,pos,buf,size,nbl) ;
						else            add_block (jbuf,st,pos,buf,size,nbl) ;
						reg_group_by(jbuf);
						FreeVec(buf);
						ret = TRUE;
					}
					else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
				}
				else ThrowError(Wnd, ErrMsg(ERR_NOTXTINCLIP));
			}
			else ThrowError(Wnd, ErrMsg(ERR_NOTXTINCLIP));
		}
		else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
		CloseIFF(clip);
	}
	/* ThrowError(Wnd, ErrMsg(ERR_READCLIP)); */
	return ret;
}

/************************************************
**** locale.library support by C.Guillaume.  ****
************************************************/

static APTR catalog = NULL;

/*** Localise all strings of the program ***/
void InitLocale(void)
{
	if( (catalog = (APTR) OpenCatalogA(NULL, "System/Tools/Editor.catalog", NULL)) )
	{
		WORD n;
		/* Translate menu strings */
		{	register struct NewMenu * nm;
			register STRPTR           str;
			extern   struct NewMenu   newmenu[];

			for(nm = newmenu, n = MSG_PROJECTTITLE; nm->nm_Type != NM_END; nm++)
			{
				if (nm->nm_Label != NM_BARLABEL)
				{
					str = (STRPTR) GetCatalogStr( catalog, n++, nm->nm_Label );
					/* Change shortcut to user prefered one */
					if (*str == 127) {
						nm->nm_CommKey  = str+1; str += 2;
						nm->nm_Flags   &= ~NM_COMMANDSTRING;
					}
					nm->nm_Label = str;
				}
			}
		}
		/* Translate all other messages */
		{	register STRPTR * str;
			for(n = 0, str = JanoMessages; n < sizeof(JanoMessages)/sizeof(STRPTR); n++, str++)
				if (*str != NULL)
					*str = (STRPTR) GetCatalogStr( catalog, n + ERR_BADOS, *str );
		}
	}
}

/*** Free allocated ressource ***/
void CleanupLocale(void)
{
	if (catalog != NULL) CloseCatalog(catalog);
}

