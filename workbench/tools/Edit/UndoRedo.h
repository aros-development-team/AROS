/**********************************************************
**                                                       **
**      $VER: UndoRedo.h 1.2 (3.2.2001)                  **
**      Datatypes for journalized buffer                 **
**                                                       **
**      © T.Pierron, C.Guilaume. Free software under     **
**      terms of GNU public license.                     **
**                                                       **
**********************************************************/

#ifndef UNDOREDO_H
#define UNDOREDO_H

#ifndef	MEMORY_H
#include "Memory.h"
#endif

/* Size of one chunk of undo/redo buf (must be a power of 2) */
#define	UNDO_CHUNK			32

/** Structure holding modifications data (operations) **/
typedef struct _RBOps
{
	struct _RBOps *prev;
	UWORD          size;						/* Nb. of bytes in the array data */
	UBYTE          data[ UNDO_CHUNK ];	/* Buffer for operation data */
}	*RBOps;

/** Rollback segment (text) **/
typedef struct _RBSeg
{
	struct _RBSeg *prev, *next;
	ULONG          max;						/* Max data in data array */
	UBYTE          data[1];					/* Removed characters */
}	*RBSeg;

/** Internal values per Project **/
typedef struct _JBuf							/* Journalized buffer */
{
	LINE  *line;								/* Last line modified */
	ULONG  nbc;									/* Last character modified */
	RBOps  ops;									/* Stack of operations */
	RBSeg  rbseg;								/* Rollback segment */
	ULONG  size;								/* Nb. of bytes in modif buffer */
	ULONG  rbsz;								/* Nb. of bytes in rollback segment */
	UBYTE  op;									/* Type of last operation */
	UBYTE  rbtype;								/* 0:undo 1:redo */
	APTR   prj;									/* Link to project struct */
}	JBUF;

typedef JBUF *				JBuf;				/* Main type pointer */

/** Get Project struct **/
#define	PRJ(jbuf)		((Project)jbuf->prj)

/** Operations journalized **/
#define	ADD_CHAR			1					/* Add chars on a new line */
#define	REM_CHAR			2					/* Remove chars */
#define	REM_LINE			3					/* Remove whole content of a line */
#define	JOIN_LINE		4					/* Line joined */
#define	GROUP_BY			5					/* Group of operation */

#define	LAST_TYPE(x)	((STRPTR)(x))[-1]
#define	IS_COMMIT(x)	((STRPTR)(x))[-2]
#define	OUT_OF_DATE		((STRPTR)-1)
#define	NO_MEANING_VAL	((STRPTR)-2)

/** Datatypes specific to an operation (must be WORD aligned) **/
typedef struct _AddChar
{
	LINE *line;									/* Line where operation occured */
	UWORD pos;									/* Position in this line */
	UWORD nbc;									/* Nb. of char inserted */
#ifdef _AROS
	UWORD pad;									/* to get multiple-of-4 structure sizeof */
#endif
	UBYTE commit;								/* Savepoint */
	UBYTE type;									/* ADD_CHAR */
}	*AddChar;

/** Yes, no meaning changes, except type == REM_CHAR **/
typedef struct _AddChar						*RemChar;

typedef struct _RemLine
{
	LINE  *line;								/* Line removed */
	LINE  *after;								/* Insert the line after this one */
#ifdef _AROS
	UWORD pad;									/* to get multiple-of-4 structure sizeof */
#endif
	UBYTE  commit;								/* Savepoint */
	UBYTE  type;								/* REM_LINE */
}	*RemLine;

typedef struct _JoinLine
{
	LINE  *line;								/* First line concerned */
	LINE  *old;									/* Old line removed */
	ULONG  pos;									/* Joined position */
#ifdef _AROS
   UWORD pad;									/* to get multiple-of-4 structure sizeof */
#endif
	UBYTE  commit;								/* Savepoint */
	UBYTE  type;								/* JOIN_LINE */
}	*JoinLine;

typedef struct _GroupBy						/* Gather multiple operations */
{
#ifdef	_AROS
   UWORD pad;									/* to get multiple-of-4 structure sizeof */
#endif
	UBYTE  commit;								/* Savepoint */
	UBYTE  type;								/* GROUP_BY */
}  *GroupBy;

void flush_undo_buf ( JBuf );
void rollback       ( JBuf );
void last_modif     ( JBuf, char );
#ifdef	PROJECT_H
void commit_work    ( Project );
#endif

STRPTR pop_last_op  ( JBuf, char, char ); /* Update JBuf values */

void reg_add_chars  ( JBuf, LINE *, ULONG pos, UWORD nb );
void reg_rem_chars  ( JBuf, LINE *, ULONG s,   ULONG e  );
void reg_join_lines ( JBuf, LINE *, LINE * );
void reg_rem_line   ( JBuf, LINE * );
void reg_group_by   ( JBuf );

#endif
