/**************************************************************
**** memory.c: memory allocation of edited file. Critical! ****
**** Free software under GNU license, written in 15/2/2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#include "Memory.h"
#include "Project.h"
#include "Utility.h"
#include "Cursor.h"
#include "ProtoTypes.h"

/*** Allocate a new line filled with "bytes" of "size" length ***/
LINE *create_line(STRPTR bytes, ULONG size)
{
	/* Round the size to the wanted granularity */
	register ULONG rsize = (size & ~(GRANULARITY-1)) + GRANULARITY;
	LINE *new;

	/* Allocate struct & buf */
	if(NULL != (new = (LINE *) AllocMem(sizeof(*new), MEMF_PUBLIC)))
	{
		if(NULL != (new->stream = (UBYTE *) AllocMem(rsize, MEMF_PUBLIC)))
		{
			/* The line has been fully allocated! */
			if(bytes) CopyMem(bytes, new->stream, size);
			new->size=size; new->max=rsize;
			new->flags=0;
		}	else { FreeMem(new, sizeof(*new)); return NULL; }
	}
	return new;
}

/*** Simplified line creation ***/
LINE *new_line( LINE *prev )
{
	LINE *new;
	if(NULL != (new = (LINE *) AllocMem(sizeof(*new), MEMF_PUBLIC)))
	{
		new->max   = 0;
		new->flags = 0;
		InsertAfter(prev, new);
	}
	return new;
}

/*** Try to resize the memory chunk of text, PRI-VA-TE! ***/
static ULONG realloc_stream(LINE *ln, ULONG size)
{
	UBYTE *new; ULONG rsize;
	/* Always adjust size to the granularity of mem */
	rsize = (size & ~(GRANULARITY-1)) + GRANULARITY;

	/* Don't free old stream, it's still needed */
	if(NULL != (new = (UBYTE *) AllocMem(rsize, MEMF_PUBLIC)))
		ln->stream = new;
	else
		/* Try to reduce disaster */
		if( size <= ln->max ) rsize=ln->max;
		else return KERNEL_PANIC;

	ln->size=size; size=ln->max; ln->max=rsize;
	/* Return the size of old chunk that is going to be deallocated */
	return size;
}

/*** Low-level deallocation function ***/
void free_line(LINE *ln)
{
	if(ln->max) FreeMem(ln->stream, ln->max);
	FreeMem(ln, sizeof(*ln));
}

/*** Free all allocated memory ***/
void trash_file(LINE *next)
{
	register LINE *ln;
	for(ln=next; ln; ln=next)
	{
		next=ln->next;
		/* free_line(ln); */
		if(ln->max) FreeMem(ln->stream, ln->max);
		FreeMem(ln, sizeof(*ln));
	}
}

/*** Add a char in a line ***/
BOOL add_char(JBuf jb, LINE *ln, ULONG pos, UBYTE newchar)
{
	/* Is there enough place to store the new char? */
	if(ln->size >= ln->max)
	{
		/* A reallocation is needed */
		register STRPTR oldbuf = ln->stream;
		register ULONG  oldsz;

		if(KERNEL_PANIC != (oldsz = realloc_stream(ln, ln->size+1)))
		{
			if(pos) CopyMem(oldbuf, ln->stream, pos);
			CopyMem(oldbuf+pos, ln->stream+pos+1, ln->size-pos-1);
			if(oldsz > 0) FreeMem(oldbuf, oldsz);
		}
		else return FALSE;
	} else
		MemMove(ln->stream+pos,1,ln->size-pos), ln->size++;

	if( jb ) reg_add_chars(jb, ln, pos, 1);
	ln->stream[pos] = newchar;
	return TRUE;
}

/*** Like previous but a whole byte stream this time ***/
BOOL insert_str(JBuf jb, LINE *ln, ULONG pos, STRPTR str, ULONG lg)
{
	/* Is there enough place to store the string? */
	if(ln->size+lg>ln->max)
	{
		register STRPTR oldbuf = ln->stream;
		register ULONG  oldsz;

		/* If realloc failed, don't change anything */
		if(KERNEL_PANIC != (oldsz = realloc_stream(ln, ln->size+lg)))
		{
			if(pos) CopyMem(oldbuf, ln->stream, pos);
			CopyMem(oldbuf+pos, ln->stream+pos+lg, ln->size-pos-lg);
			if(oldsz > 0) FreeMem(oldbuf, oldsz);
		} else
			return FALSE;		/* Nothing change */

	} else
		/* Just shifts the buffer */
		MemMove(ln->stream+pos,lg,ln->size-pos), ln->size+=lg;

	if( jb ) reg_add_chars(jb, ln, pos, lg);
	CopyMem(str,ln->stream+pos,lg);
	return TRUE;
}

/*** Like previous, but what's left after pos in ln is stored in a new line ***/
BOOL break_line(JBuf jb, LINE *ln, ULONG pos, STRPTR str, ULONG lg)
{
	STRPTR oldbuf = ln->stream;
	ULONG  rsize  = ((pos+lg) & ~(GRANULARITY-1)) + GRANULARITY,
	       size   = ln->size-pos;
	LINE  *ln2;

	/* Add rest of line in a new string */
	if(NULL != (ln2 = create_line(oldbuf + pos, size)))
	{
		if( rsize != ln->max )
			/* If realloc failed, don't change anything */
			if(KERNEL_PANIC != (rsize = realloc_stream(ln, pos+lg)))
			{
				if(pos) CopyMem(oldbuf, ln->stream, pos);
			}	else {
				free_line(ln2);
				return FALSE;		/* Nothing change */
			}
		else ln->size = pos + lg;
		InsertAfter(ln, ln2);
		if( jb ) reg_add_chars(jb, ln, pos, lg+1);
		CopyMem(str, ln->stream+pos, lg);
	}	else return FALSE;

	if(oldbuf != ln->stream && rsize>0) FreeMem(oldbuf, rsize);
	return (BOOL)(str!=NULL);
}

/*** Add a whole string, taking care of newline ***/
BOOL add_string(JBuf jb, LINE *ln, ULONG pos, STRPTR string, ULONG lg, LONG *buf)
{
	ULONG i; STRPTR str;
	for(str=string, i=0, buf[0]=pos, buf[1]=0; i<lg; str++, i++)
		if(*str=='\n')
			/* Add string at pos and copy rest of line to a new one */
			if(break_line(jb,ln,pos,string,str-string)) string=str+1,pos=0,ln=ln->next,buf[1]++;
			/* Fucking memory failure! */
			else return FALSE;

	/* Some bytes left */
	if(string < str)
		if( !insert_str(jb, ln,pos,string,str-string) ) return FALSE;
		else buf[0] = str-string;

	if( 0 == (buf[2] = buf[1]) ) buf[0] += pos;
	return TRUE;
}

/*** Add a block, taking care of newline ***/
BOOL add_block(JBuf jb, LINE *ln, ULONG pos, STRPTR string, ULONG lg, LONG *buf)
{
	ULONG i; STRPTR str; char isf;
	pos = x2pos(ln,pos);
	for(str=string, i=lg, buf[1]=buf[2]=0, isf=1; i--; str++)
		if(*str=='\r') { flush_ln:
			/* First text insertion, must be done on current line */
			if( isf ) goto insln;
			if(ln->next) {
				ln=ln->next; insln: lg = find_nbc(ln, pos);
				if( !insert_str(jb, ln,lg,string,str-string) ) return FALSE;
				if( isf ) buf[1]--,isf=0; buf[0]=lg+str-string;
			}	else {
				/* Not enough lines, creates one */
				LINE *new;
				if(NULL != (new = create_line(string,buf[0]=str-string))) {
					if( jb ) reg_add_chars(jb, new, 0, buf[0]+1);
					InsertAfter(ln, new);
					ln=new; buf[2]++;
				}
				else return FALSE;
			}
			string=str+1; buf[1]++;
		}

	/* Some bytes left (might not happen) */
	if(string < str) { i=0; goto flush_ln; }

	return TRUE;
}

/*** Modification of characters ***/
BOOL replace_char(LINE *ln, ULONG pos, UBYTE newchar)
{
	ln->stream[pos] = newchar;
	return TRUE;
}

/*** like previous, but with multiple replacement ***/
BOOL replace_chars(LINE *ln, ULONG s, ULONG e, UBYTE (*change)(UBYTE))
{
	register UBYTE *buf;
	for(buf=ln->stream+s; s < e; s++,buf++)
		*buf = change(*buf);
	return TRUE;
}

/*** Remove chars from pos s to e, including limits (s>=e) ***/
BOOL rem_chars(JBuf jb, LINE *ln, ULONG s, ULONG e)
{
	ULONG size = ln->size-(e-s+1);
	UBYTE *old = ln->stream;

	if( jb ) reg_rem_chars(jb, ln, s, e);
	/* Adjust the buffer to not be too much large */
	if((ULONG) (ln->max-size) >= GRANULARITY)
		/* Reduces its size, to keep minimal mem usage */
		if(KERNEL_PANIC != (size = realloc_stream(ln, size)))
			/* Copy beginning of line */
			CopyMem(old, ln->stream, s);
		else return FALSE;
	else ln->size=size;

	/* Copy end of line */
	if(s < ln->size) CopyMem(old+e+1, ln->stream+s, ln->size-s);
	/* Free reallocated chunk */
	if(old != ln->stream && size>0) FreeMem(old, size);
	return TRUE;
}

/*** Split line ln at specified position ***/
BOOL split_line(JBuf jb, LINE *ln, ULONG pos, ULONG *nbrwc, BYTE indent)
{
	LINE *new;
	LONG spc=0;
	/* Count the blank chars starting the string */
	if( indent )
	{
		register UBYTE *str;
		for(str=ln->stream; spc<ln->size && (*str=='\t' || *str==' '); spc++, str++);
		if(spc>pos) spc=pos;
	}
	if(NULL != (new = create_line(NULL, ln->size-pos+spc)))
	{
		if( jb ) {
			if( spc ) reg_group_by( jb );
			reg_add_chars(jb, ln, pos, 1);
			if( spc ) reg_add_chars(jb, new, 0, spc), reg_group_by( jb );
		}
		/* Insert new line in buffer */
		InsertAfter(ln, new);
		/* Copy the buffer */
		if(indent && spc)  CopyMem(ln->stream, new->stream, spc);
		if(pos < ln->size) CopyMem(ln->stream+pos, new->stream+spc, ln->size-pos);
		ln->size = pos; *nbrwc = spc;
		return TRUE;
	}
	return FALSE;
}

/*** Remove an entire line ***/
BOOL del_line(JBuf jb, LINE **first, LINE *del)
{
	/* Don't delete the last line */
	if( del->next == NULL )
	{
		if(del->size && jb) reg_rem_chars(jb, del, 0, del->size-1);

		/* Keep a minimal edit buffer */
		if(del->max > GRANULARITY)
		{
			STRPTR old = del->stream;
			ULONG  sz;
			if( (sz = realloc_stream(del, 0)) && old!=del->stream )
				FreeMem(old, sz);
		}
		else del->size=0;
		/* Line wasn't entirely removed */
		return FALSE;
	} else {
		/* Remove item from linked list and deallocate it */
		Destroy(first, del);
		if( jb ) reg_rem_line(jb, del);
		else     free_line( del );
		return TRUE;
	}
}

/*** Join two lines ***/
BOOL join_lines(JBuf jb, LINE *ln1, LINE *ln2)
{
	ULONG size;
	if( jb ) reg_join_lines(jb, ln1, ln2);
	/* Is there enough place in ln1 to store ln2? */
	if( (size = ln1->size + ln2->size) <= ln1->max)
	{
		/* Yes, just store ln2 after ln1 */
		CopyMem(ln2->stream, ln1->stream+ln1->size, ln2->size);
		ln1->size = size;
	} else {
		STRPTR old = ln1->stream;
		ULONG  nbc = ln1->size;
		/* Need to realloc the stream */
		if(KERNEL_PANIC != (size = realloc_stream(ln1, size)))
		{
			CopyMem(old, ln1->stream, nbc);
			CopyMem(ln2->stream, ln1->stream+nbc, ln2->size);
			if(size > 0) FreeMem(old, size);
		}	else return FALSE;
	}
	/* Remove the line ln2 (ln2 is never the first line) */
	Destroy(NULL, ln2);
	if(jb == NULL) free_line( ln2 );
	return TRUE;
}

/*** Simple total lines and bytes count ***/
ULONG size_count( LINE *first, BYTE szEOL )
{
	register ULONG i;
	register LINE  *ln;
	for(i=0, ln=first; ln; i+=ln->size+szEOL, ln=ln->next);
	return i-szEOL;
}
