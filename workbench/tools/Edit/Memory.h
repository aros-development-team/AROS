/**************************************************************
**** memory.h: main data-types for storing file in memory  ****
**** Free software under GNU license, written in 15/2/2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#ifndef	MEMORY_H
#define	MEMORY_H

#include <exec/types.h>
#include <exec/memory.h>

/*** Internal representation of a line of text: ***/
typedef	struct _Line
{
	struct _Line *next,*prev;	/* linked list */
	ULONG size, max;				/* actual size and max before overflow */
	STRPTR stream;					/* Start of buffer */
	UBYTE flags;					/* See below */
}	LINE;

#include "UndoRedo.h"

/*** Valid flags for the previous field ***/
#define	FIRSTSEL			1		/* Line begins a selection */
#define	LASTSEL			2		/* Line ends a selection */
#define	WHOLESEL			4		/* Entire line is selected */

/*** Granularity of mem allocation (power of 2) ***/
#define	GRANULARITY			16

/*** Special code ***/
#define	KERNEL_PANIC	(ULONG)-1

LINE *create_line (STRPTR,ULONG size);						/* Create a new line of size bytes */
LINE *new_line    (LINE *);									/* Simplified line creation */
void free_line    (LINE *);
void trash_file   (LINE *);									/* Trash all lines of a file */
BOOL add_char     (JBuf,LINE *,ULONG,UBYTE);				/* Add a char to a line at pos */
BOOL insert_str	(JBuf,LINE *,ULONG,STRPTR,ULONG);	/* Like add_string, but doesn't care of \n */
BOOL rem_chars    (JBuf,LINE *,ULONG s, ULONG e);		/* Remove chars between and incl. s, e (s<=e) */
BOOL del_line     (JBuf,LINE **,LINE *);					/* Remove an line */
BOOL join_lines   (JBuf,LINE *, LINE *);					/* Join to lines */
BOOL split_line   (JBuf,LINE *,ULONG,ULONG *,BYTE);	/* Split line in two parts with indent */
BOOL replace_char (LINE *,ULONG,UBYTE);					/* Change char at pos */
ULONG size_count  (LINE *,BYTE);								/* Count the bytes of a file */

BOOL replace_chars(LINE *,ULONG s,ULONG e,UBYTE (*change)(UBYTE));


BOOL add_string   (JBuf, LINE *, ULONG pos, STRPTR, ULONG len, LONG *buf);
BOOL add_block    (JBuf, LINE *, ULONG pos, STRPTR, ULONG len, LONG *buf);
/* buf will be set to the following values at the exit:
** buf[0]: Horizontal position for the cursor
** buf[1]: Number of lines modified in the display
** buf[2]: Number of lines added to the buffer (rq: for add_string buf[1] == buf[2])
*/

#endif
