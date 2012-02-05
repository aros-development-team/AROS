/**************************************************************
**** diskio.c : procedures for accessing/writing on disk   ****
**** Free software under GNU license, started on 25/2/2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include "Project.h"
#include "DiskIO.h"
#include "Gui.h"
#include "Utility.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS			/* Error msg use strings id */
#include "strings.h"

#define	MEM_CHUNK		8192		/* To save file */

extern struct Library *AslBase;

extern struct Screen *Scr;
struct FileRequester *fr=NULL;

/* ASL load or save requester tags */
static IPTR tags[] = {
	ASLFR_InitialLeftEdge,        0,
	ASLFR_InitialTopEdge,         0,
	ASLFR_InitialWidth,           320,
	ASLFR_InitialHeight,          256,
	ASLFR_Window,          (IPTR) NULL,
	ASLFR_Screen,          (IPTR) NULL,
	ASLFR_Flags1,                 0,
	ASLFR_SleepWindow,            TRUE,
	ASLFR_InitialDrawer,   (IPTR) NULL,
	ASLFR_InitialFile,     (IPTR) NULL,
	ASLFR_InitialPattern,  (IPTR) NULL,
	ASLFR_TitleText,       (IPTR) NULL,
	TAG_DONE
};

/*** Find the correctly spelled path ***/
BYTE get_full_path( STRPTR filename, STRPTR *dest )
{
	STRPTR file = NULL, path;
	BPTR   lock;
	WORD   len;

	len  = sizeof(STR_MODIF) + 1;
	lock = (BPTR) Lock(filename, SHARED_LOCK);

	/* Can't get lock, surely because file does not exist */
	if( lock == BNULL ) {
		file = (STRPTR) FilePart(filename); len += strlen(file);
		if(file != filename) {
			char svg = *file; *file = 0;
			lock = (BPTR) Lock(filename, SHARED_LOCK); *file = svg;
		}
	}

	/* Be sure the locked object is of the desired type */
	if(lock != BNULL)
	{
		struct FileInfoBlock *fib;
		if((fib = (void *) AllocVec(sizeof(*fib), MEMF_PUBLIC)) && Examine(lock, fib))
		{
			/* That's particularly nasty, but prevent user of doing mistakes */
			if(file == NULL ? fib->fib_EntryType > 0 : fib->fib_EntryType < 0)
				/* Filename sucks, get a generic one */
				UnLock(lock), lock = BNULL;
		}
		if( fib ) FreeVec( fib );
		else { UnLock(lock); return ERR_NOMEM; }
		/* User has given a directory as file to edit! */
		if(file == NULL && lock == BNULL) return ERR_WRONG_TYPE;
	}

	/* The directory/file exists, Get its correctly spelled name */
	for(len += 256; ;)
	{
		if( ( path = (STRPTR) AllocVec(len, MEMF_PUBLIC) ) ) {
			/* If no directory provide, try to get the current wd */
			if( lock ? NameFromLock(lock, path, len) :
			           GetCurrentDirName(path,  len) )
			{
				/* At last if there was a file name, add it to the buffer */
				if( file ) AddPart(path, file, len);
				*dest = path;
				break;
			}
		} else return ERR_NOMEM;
		/* Fucking name! reallocate a longer buffer! */
		FreeVec(path); len += 64;
	}
	if( lock ) UnLock(lock);

	return 0;
}

/*** Extract path and file into 2 separate pointers (for ASL) ***/
void split_path(AskArgs *src, STRPTR *dir, STRPTR *file)
{
	extern UBYTE BufTxt[256];
	if( src->dir != NULL )
	{
		/* TODO cleanup */
		UWORD Len = src->file - src->dir;
		if(dir != NULL) {
			if(Len >= sizeof(BufTxt)) Len = sizeof(BufTxt)-1;
			CopyMem(src->dir, BufTxt, Len); BufTxt[Len] = '\0';
			*dir = BufTxt;
		} else Len = 0;

		/* Copy file name */
		*file = BufTxt+Len+1; Len = strlen( src->file );
		CopyMem(src->file, *file, Len); (*file)[Len] = '\0';
	}
	else
	{
		*file = src->file;
		if ( dir != NULL ) *dir = NULL;
	}
}

/*** Encapsulated cleaning procedure of allocated things ***/
void free_diskio_alloc(void)
{
	if(fr) FreeFileRequest(fr);
}

/*** Save width and height of an ASL requester ***/
void save_asl_dim(struct FileRequester *fr)
{
	tags[5] = fr->fr_Width;
	tags[7] = fr->fr_Height;
}

/*** Init the ASL tags table ***/
struct TagItem *init_tags(struct Window *wnd, ULONG flags)
{
	/* Init LeftEdge */
	tags[1] = wnd->LeftEdge + (wnd->Width-tags[5]) / 2;
	/* TopEdge */
	tags[3] = wnd->TopEdge + wnd->BorderTop;
	/* Window, Screen, Flags */
	tags[9]  = (IPTR) wnd;
	tags[11] = (IPTR) wnd->WScreen;
	tags[13] = flags;
	tags[21] = (IPTR) "#?";
	return (struct TagItem *)tags;
}

/*** Popup a file requester to choose a save file ***/
STRPTR ask_save(struct Window *wnd, AskArgs *path, CONST_STRPTR title)
{
	split_path(path, (STRPTR *)&tags[17], (STRPTR *)&tags[19]);
	tags[23] = (IPTR)title;

	/* If requester hasn't been allocated yet */
	if(fr==NULL && !(fr = (void *)AllocAslRequest(ASL_FileRequest,NULL)))
		ThrowError(Wnd, ErrMsg(ERR_NOASLREQ));
	else
		/* Display the file requester */
		if(AslRequest(fr,init_tags(wnd, FRF_DOSAVEMODE | FRF_DOPATTERNS)))
		{
			STRPTR path;
			save_asl_dim(fr);
			/* User may choose strange path like "CPROG://docs/prog.c" **
			** get_full_path will search for the correct pathname      */
			if( ( path = CatPath(fr->fr_Drawer, fr->fr_File) ) )
			{
			    	STRPTR fullpath;
				WORD err = get_full_path(path, &fullpath);
				/* get_full_path, allocate a new buffer since size may vary */
				FreeVec( path );
				if( err == 0 ) return fullpath;
				ThrowError(Wnd, ErrMsg(err));
			} else
				ThrowError(Wnd, ErrMsg(ERR_NOMEM));
		}
	return NULL;
}

/** Original FWrite has a too small buffer **/
static STRPTR buffer = NULL;
static ULONG  usage  = 0;
char myFWrite(BPTR file, CONST_STRPTR buf, ULONG size)
{
	if(buffer == NULL && NULL == (buffer = (STRPTR) AllocVec(MEM_CHUNK, MEMF_PUBLIC)))
		goto emergency;

	if(size+usage > MEM_CHUNK) {
		if( Write(file, buffer, usage) != usage ) return 0;
		else usage = 0;
	}
	if(size > MEM_CHUNK)
		emergency: return (char) (Write(file, buf, size) == size ? 1 : 0);
	else
		CopyMem(buf, buffer+usage, size), usage+=size;
	return 1;
}

char myFClose( BPTR file )
{
	char retcode = 1;
	/* Flush buffer before closing */
	if(buffer) {
		/* Everything must be controlled */
		if(usage > 0 && Write(file, buffer, usage) != usage)
			retcode = 0;
		FreeVec(buffer); buffer=NULL; usage=0;
	}
	Close(file);
	return retcode;
}

/** Redirect functions **/
#ifdef FWrite
#undef FWrite
#endif

#define	FWrite(x,y,z,t)	myFWrite(x,y,z)

#ifdef FClose
#undef FClose
#endif

#define	FClose(x)			myFClose(x)

/*** Write the lines svg in the file `name', returns 1 if all's OK ***/
BYTE save_file(STRPTR name, LINE *svg, unsigned char eol, LONG protection)
{
	STRPTR buf;
	LONG   i;
	BYTE   szeol = szEOL[eol];
	LINE *ln; BPTR fh=BNULL;
	BYTE retval = 0;
	BOOL newfile=FALSE;

	BusyWindow(Wnd);

	/* first try if file already exist */
	if( (fh = Open(name, MODE_OLDFILE)) )
	{
		SetFileSize(fh, 0, OFFSET_BEGINNING);
	}
	else
	{
		/* No,open new file */
		fh = Open(name, MODE_NEWFILE);
		newfile=TRUE;
	}

	if( fh )
	{
		for(ln=svg, buf=NULL, i=0; ln; ln=ln->next)
		{
			if(i == 0) buf = ln->stream;
			/* An unmodified line (2nd cond. is for deleted lines) */
			if(ln->max == 0 && ln->stream-buf == i) i+=ln->size+szeol;

			else {
				/* Flush preceding unmodified buffer */
				i -= szeol;
				if( i>=0 && (FWrite(fh, buf,       i,     1) != 1 ||
					FWrite(fh, chEOL+eol, szeol, 1) != 1 ) )
				{
					wrterr: ThrowDOSError(Wnd, name);
					i=0;    break;
				}

				/* Writes the modified line */
				if( FWrite(fh, ln->stream, ln->size, 1) != 1 || (ln->next != NULL &&
				    FWrite(fh, chEOL+eol,  szeol,    1) != 1 ) )
					goto wrterr;
				i=0;
			}
		}
		/* Flush buffer */
		if( i>szeol && FWrite(fh,buf,i-szeol,1)!=1 ) goto wrterr;

		FClose( fh );
		/* set protection flags, if file is new. */ 
		if( newfile ) SetProtection(name, protection);

		retval = 1;

	} else if(IoErr() == ERROR_OBJECT_EXISTS)
		ThrowError(Wnd, ErrMsg(ERR_WRONG_TYPE));
	else
		ThrowDOSError(Wnd, name);

	WakeUp(Wnd);
	return retval;
}

/*** Show a requester to choose a file to load ***/
STRPTR ask_load(struct Window *wnd, AskArgs *path, BYTE set_file, CONST_STRPTR title)
{
	split_path(path, (STRPTR *)&tags[17], (STRPTR *)&tags[19]);
	if(set_file == 0) tags[19] = 0;
	tags[23] = (IPTR)title;

	/* Alloc a file requester, if it hasn't been done yet */
	if(fr==NULL && !(fr = (void *)AllocAslRequest(ASL_FileRequest,NULL)))
		ThrowError(Wnd, ErrMsg(ERR_NOASLREQ));
	else
		/* Display the load requester */
		if(AslRequest(fr, init_tags(wnd, (set_file ? FRF_DOPATTERNS : FRF_DOMULTISELECT|FRF_DOPATTERNS))))
		{
			STRPTR path;
			save_asl_dim(fr);
			/* This is actually a (StartUpArgs *) */
			if( set_file == 0 ) return (STRPTR) &fr->fr_NumArgs;

			if( ( path = CatPath(fr->fr_Drawer, fr->fr_File) ) )
			{
			    	STRPTR fullpath;
				WORD err = get_full_path(path, &fullpath);
				FreeVec( path );
				if( err == 0 ) return fullpath;
				ThrowError(Wnd, ErrMsg(err));
			} else
				ThrowError(Wnd, ErrMsg(ERR_NOMEM));
		}
	return NULL;
}


/*** Simple proc to read a file ***/
WORD read_file( LoadFileArgs *args, ULONG *len )
{
	BPTR fh;
	WORD err = RETURN_OK;

	args->buffer = NULL;
	args->lines  = NULL;
	args->protection = 0;
	*len         = 0;

	if( ( fh = Open(args->filename, MODE_OLDFILE) ) )
	{
		/* First: Get file size */
		struct FileInfoBlock *fib;
		if( ( fib = (APTR) AllocDosObject( DOS_FIB, NULL ) ) )
		{
			if( ExamineFH(fh, fib) )
			{
				args->protection = fib->fib_Protection;
				/* Then reads it in one large buffer */
				if( (*len = fib->fib_Size) > 0 )
				{
					/* This is a critical call, with a high probability to fail ! */
					if( ( args->buffer = (STRPTR) AllocVec(*len, MEMF_PUBLIC) ) )
					{
						if( Read(fh, args->buffer, *len) != *len )
							err = ERR_LOADFILE;
					}
					else err = ERR_NOMEM;
				}
				/* This file is empty: we still need to allocate one line */
			}
			else err = ERR_LOADFILE;
			FreeDosObject( DOS_FIB, fib );
		}
		else err = ERR_NOMEM;

		Close( fh );
	}
	return err;
}

/*** load specified file from disk ***/
WORD load_file( LoadFileArgs *args )
{
	ULONG  len;
	WORD   err;
	LINE * new;

	BusyWindow(Wnd);

	/* We have read the file in a buffer: create the lines */
	if( RETURN_OK == (err = read_file( args, &len )) )
	{
		STRPTR p, q; char eol = '\n';
		ULONG  i;

		args->eol = AMIGA_EOL;
		/* Search in the very first line, the End Of Line type */
		for(args->nblines=1, i=0, q=p=args->buffer;
		    i<len && *p!='\r' && *p!='\n'; p++, i++);

		if( i < len && *p == '\r' )
			args->eol = (p[1] == '\n' ? MSDOS_EOL : MACOS_EOL),
			eol = '\r';

		/* Build lines list */
		if( ( args->lines = new = new_line(NULL) ) )
		{
			/* This loop is the most CPU consuming part */
			for(; i<len; p++, i++)
				if (*p == eol)
				{
					new->size   = p-q; 
					/* *p = '\n';   Removed to preserve MS-DOS/Windows CRLF end-of-line chars */ 
					new->stream = q;
					args->nblines++;
					if(args->eol == MSDOS_EOL && p[1]=='\n') p++,i++; q = p+1;
					if(NULL == (new = new_line(new))) {
						err = ERR_NOMEM; break;
					}
				}
			new->size   = p-q;
			new->stream = q;
		} else err = ERR_NOMEM;
	}
	/* If there was errors: cleanup all things */
	if( err != 0 && args->lines ) trash_file(args->lines);

	WakeUp(Wnd);
	return err;
}

