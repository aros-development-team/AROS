/*********************************************************
** cliploc.c: Contains clipboard related procedures and **
** locale library support. Adapted from a RKM example.  **
** Written by T.Pierron, 10-dec-1999                    **
*********************************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/clipboard.h>
#include "Memory.h"
#include "ClipLoc.h"
#include "Version.h"
#include "Cursor.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS		/* We will need the string id */
#define  CATCOMP_STRINGS		/* and the english string corresponding to the id */
#include "Jed_Strings.h"

#ifdef MAKE_ID
#undef MAKE_ID
#endif

#define MAKE_ID(a,b,c,d) ((a<<24L) | (b<<16L) | (c<<8L) | d)

#define ID_FORM	MAKE_ID('F','O','R','M')
#define ID_FTXT	MAKE_ID('F','T','X','T')
#define ID_CHRS	MAKE_ID('C','H','R','S')

/** Union to access ULONG table as an UBYTE one **/
typedef union
{
	char  *ptr8;
	long  *ptr32;
}	PTR;

#define	WriteLONG(Buf,Val)		*(Buf.ptr32)++=Val
#define	ReadLONG(Buf,n)			((ULONG *)Buf)[n]

/* Global error messages */
STRPTR Errors[] = {
	ERR_BADOS_STR,				ERR_NOASLREQ_STR,
	ERR_NOWRTACCESS_STR,		ERR_NOMEM_STR,
	ERR_NOGUI_STR,				ERR_NONAME_STR,
	ERR_WRITECLIP_STR,		ERR_OPENCLIP_STR,
	ERR_LOADFILE_STR,			ERR_NOTXTINCLIP_STR,
	ERR_WRONG_TYPE_STR,		ERR_READCLIP_STR,
	ERR_NOBRACKET_STR,		ERR_NOT_FOUND_STR,
	ERR_LOADFONT_STR,			ERR_NOPREFEDITOR_STR,
	ERR_BADPREFSFILE_STR,	ERR_FILEMODIFIED_STR,
	ERR_SLC_STR,				ERR_NOSEARCHTEXT_STR,
	ERR_FILEEXISTS_STR,		ERR_OC_STR,
	WARN_RECORD_STR,			WARN_RECORDED_STR,
	WARN_REC_STR
};

/* About messages */
STRPTR MsgAbout[] = {
	MSG_ABOUT_STR,				MSG_FORMATINFO_STR,
	MSG_CONTINUE_STR,			MSG_BYTE_STR,
	MSG_BYTES_STR,				MSG_LINE_STR,
	MSG_LINES_STR
};

/*** Try to open the clipboard ***/
struct IOClipReq *CBOpen(ULONG unit)
{
	struct MsgPort   *mp;
	struct IOClipReq *ior;

	if( ( mp = (void *) CreateMsgPort() ) )
	{
		if( ( ior = (void *) CreateIORequest(mp,sizeof(*ior)) ) )
		{
			if( OpenDevice("clipboard.device",unit,(struct IORequest *)ior,0L) == 0 )
				return ior;

			DeleteIORequest((struct IORequest *)ior);
		}
		DeleteMsgPort(mp);
	}
	return NULL;
}

/*** Try to close it ***/
void CBClose(struct IOClipReq *ior)
{
	if( ior )
	{
		struct MsgPort *mp = ior->io_Message.mn_ReplyPort;

		CloseDevice((struct IORequest *)ior);
		DeleteIORequest((struct IORequest *)ior);
		DeleteMsgPort(mp);
	}
}

static UBYTE Buffer[20];

/*** Write strings to the clipboard.device ***/
int CBWriteFTXT(struct IOClipReq *ior, LINE *stsel, struct cutcopypast *ccp)
{
	PTR p; LINE *ln;
	long slen;
	char odd;

	/* Compute how many chars are selected */
	for(slen=0, ln=stsel; ln && ln->flags; ln=ln->next)
	{
		slen += ln->size;
		/* In block selection mode, always add a newline */
		if(ccp->select == COLUMN_TYPE) slen++;
		if(ln->flags & FIRSTSEL) slen -= find_nbc(ln, ccp->startsel);
		if(ln->flags & LASTSEL)  slen += find_nbc(ln, ccp->endsel)-ln->size;
		else slen++; /* New line */
	}
	if(slen == 0)	return FALSE;
	odd = slen&1;	/* pad byte */

	/* Initial set-up for Offset, Error, and ClipID */
	ior->io_Offset = 0;
	ior->io_Error  = 0;
	ior->io_ClipID = 0;
	p.ptr8 = Buffer;

	/* Create the IFF header information */
	WriteLONG(p,ID_FORM);			/* "FORM"             */
	WriteLONG(p,slen+12+odd);		/* + "[size]FTXTCHRS" */
	WriteLONG(p,ID_FTXT);			/* "FTXT"             */
	WriteLONG(p,ID_CHRS);			/* "CHRS"             */
	WriteLONG(p,slen);				/* string length      */

	/* Write header to clipboard */
	ior->io_Data    = (STRPTR)Buffer;
	ior->io_Length  = (STRPTR)p.ptr8-Buffer;
	ior->io_Command = CMD_WRITE;
	DoIO( (struct IORequest *) ior);

	/* In columnar selection, use '\r' at eol */
	*Buffer = (ccp->select == COLUMN_TYPE ? '\r':'\n');

	/* Write the content of strings */
	for(ln=stsel, odd=0; ln && ln->flags; ln=ln->next)
	{
		ior->io_Data = (STRPTR)ln->stream;
		slen         = ln->size;
		if(ln->flags & FIRSTSEL) {
			register long rc = find_nbc(ln, ccp->startsel);
			slen-=rc; ior->io_Data+=rc;
		}
		if(ln->flags & LASTSEL) {
			slen += find_nbc(ln, ccp->endsel) - ln->size;
			/* Add automatically a newline */
			if(ccp->select == COLUMN_TYPE) odd=1;
		}
		else if(ln->size < ln->max)
			slen++, ln->stream[ ln->size ]=*Buffer;
		else odd=1;

		ior->io_Length = slen;
		DoIO( (struct IORequest *) ior);

		/* New line must be send after first IO */
		if(odd)
			ior->io_Data = Buffer, ior->io_Length = 1, odd=0,
			DoIO( (struct IORequest *) ior);
	}

	/* Tell the clipboard that we are done writing */
	ior->io_Command=CMD_UPDATE;
	DoIO( (struct IORequest *) ior);

	/* Check if io_Error was set by any of the preceding IO requests */
	return ior->io_Error ? FALSE : TRUE;
}

/*** Check if there is TXT in the clipboard ***/
int CBQueryFTXT(struct IOClipReq *ior)
{
	/* Initial set-up for Offset, Error, and ClipID */
	ior->io_Offset = 0;
	ior->io_Error  = 0;
	ior->io_ClipID = 0;

	/* Look for "FORM[size]FTXT" */
	ior->io_Command = CMD_READ;
	ior->io_Data    = (STRPTR)Buffer;
	ior->io_Length  = 12;
	DoIO( (struct IORequest *) ior);

	if( (ior->io_Actual == 12L) &&				/* Do we have at least 12 bytes ? */
	    (ReadLONG(Buffer,0) == ID_FORM) &&		/* Does it starts with "FORM" ? */
	    (ReadLONG(Buffer,2) == ID_FTXT) )		/* Is it a "FTXT" chunk ? */
		return TRUE;

	/* It's not "FORM[size]FTXT", so tell clipboard we are done */
	CBReadDone(ior);
	return FALSE;
}


/*** Reads the next CHRS chunk from clipboard ***/
BOOL CBReadCHRS(struct IOClipReq *ior, void *jbuf, LINE *st, ULONG pos, LONG *nbl)
{
	ULONG size;

	/* Find next CHRS chunk */
	ior->io_Command = CMD_READ;
	ior->io_Data    = (STRPTR)Buffer;
	ior->io_Length  = 8L;
	memset(nbl, 0, sizeof(LONG)*3);
	for(;;)
	{
		/* Read the chunk ID and its length: */
		DoIO( (struct IORequest *) ior);

		/* Have get enough data ? */
		if (ior->io_Actual != 8) break;

		/* Get buffer size */
		size = ReadLONG(Buffer,1);

		/* Is it a CHRS chunk ? */
		if(size != 0 && ReadLONG(Buffer,0) == ID_CHRS)
		{
			UBYTE *buf;
			if( (buf = (UBYTE *)AllocVec(size, MEMF_PUBLIC)) )
			{
				ior->io_Length = size;
 				ior->io_Data   = buf;

				DoIO( (struct IORequest *) ior);
				/* What's kind of paste method shall we used? */
				{	register UBYTE *s; register ULONG n;
					for(s=buf,n=size; n-- && *s!='\n' && *s!='\r'; s++);
					if( *s == '\r' ) ior->io_Data = NULL;
				}
				/* Add string to the buffer */
				reg_group_by(jbuf);
				size = ior->io_Data ? add_string(jbuf,st,pos,buf,size,nbl) :
				                      add_block (jbuf,st,pos,buf,size,nbl) ;
				reg_group_by(jbuf);
				FreeVec(buf); if(size) break;
			}
			/* Error reading from clip! */
			CBReadDone(ior);
			return FALSE;
		}
		/* If not, skip to next chunk */
		else
			ior->io_Offset += size+(size&1);
	}
	CBReadDone(ior);
	return TRUE;
}

/*** Tell clipboard we are done reading ***/
void CBReadDone(struct IOClipReq *ior)
{
	ior->io_Command = CMD_READ;
	ior->io_Data    = (STRPTR)Buffer;
	ior->io_Length  = sizeof(Buffer)-2;

	/* falls through immediately if io_Actual == 0 */
	while (ior->io_Actual)
		if (DoIO( (struct IORequest *) ior)) break;
}

/************************************************
**** locale.library support by C.Guillaume.  ****
************************************************/

#include <libraries/gadtools.h>
#include <libraries/locale.h>

extern struct NewMenu newmenu[];
extern STRPTR SWinTxt[15];
static void *catalog = NULL;

struct Vars {
	UBYTE **msg;			/* Message to change */
	UBYTE nb;				/* Nb contiguous msg to change */
	WORD  size;				/* Size of contiguous memory */
	WORD	NumStr;			/* Id string from catalog */
} TabVars[]={
	{ Errors,   sizeof(Errors)/sizeof(STRPTR),   sizeof(STRPTR), ERR_BADOS },
	{ MsgAbout, sizeof(MsgAbout)/sizeof(STRPTR), sizeof(STRPTR), MSG_ABOUT },
	{ SWinTxt,  sizeof(SWinTxt)/sizeof(STRPTR),  sizeof(STRPTR), MSG_SEARCHWINDOW },
	{ &newmenu->nm_Label, 54, sizeof(struct NewMenu), MSG_PROJECTTITLE }
};

/*** Localise all strings of the program ***/
void jano_local(void)
{
	if( (catalog = (void *) OpenCatalogA(NULL,APPNAME ".catalog",NULL)) )
	{
		union { UBYTE **pptr8; UBYTE *ptr8; } msg;       /* :-( */
		struct Vars *p;
		WORD n;
		/* All necessary information is contained in our table */
		for(p=TabVars; p<TabVars+sizeof(TabVars)/sizeof(struct Vars); p++)
			for(n=0,msg.pptr8=p->msg; n<p->nb; msg.ptr8 += p->size )
				if(*msg.pptr8!=NM_BARLABEL && *msg.pptr8!=0)		/* Newmenus barlabel string */
					*msg.pptr8 = (UBYTE *) GetCatalogStr(catalog,p->NumStr++,*msg.pptr8), n++;
	}
}

/*** Free allocated ressource ***/
void free_locale(void)
{
	if(catalog) CloseCatalog(catalog);
}
