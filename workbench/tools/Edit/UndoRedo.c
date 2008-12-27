/**********************************************************
**                                                       **
**      $VER: UndoRedo.c 1.2 (3.2.2001)                  **
**      Implementation of journalized buffer             **
**                                                       **
**      © T.Pierron, C.Guilaume. Free software under     **
**      terms of GNU public license.                     **
**                                                       **
**********************************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include "Jed.h"
#include "Utility.h"
#include "ProtoTypes.h"
#define	DEBUG_UNDO_STUFF		/* Only activated if DEBUG macro is defined */
#include "Debug.h"

/** Sizeof each struct **/
UBYTE SizeOf[] = {0,
	sizeof(struct _AddChar),
	sizeof(struct _AddChar),
	sizeof(struct _RemLine),
	sizeof(struct _JoinLine),
	sizeof(struct _GroupBy)
};

#define	PRE_CHECK(jbuf)	{ \
	register Project prj = PRJ(jbuf); \
	if((prj->state & DONT_FLUSH) == 0) { \
		flush_undo_buf(&prj->redo); \
		if((prj->state & MODIFIED) == 0) set_modif_mark( prj ); \
	}}


/*** Get last operation done ***/
static void *last_operation( JBuf jb )
{
	STRPTR ptr = jb->ops->data + jb->size;

	return ptr - SizeOf[ LAST_TYPE(ptr) ];
}

/*** Alloc a new buffer for operations only ***/
static void *new_undo_buf(JBuf jb, UWORD size)
{
	RBOps  new;
	void  *ret;
	/* Enough place? */
	if(jb->ops==NULL || jb->size + size >= UNDO_CHUNK)
	{
		/* It may have some bytes left, but don't care splitting op over 2 buf */
		if(NULL == (new = (RBOps) AllocVec(sizeof(*new), MEMF_PUBLIC)))
			return NULL;

		new->prev = jb->ops;
		new->size = jb->size; /* Save old usage */
		jb->ops   = new;
		jb->size  = size;
		ret = new->data;
	} else {
		ret = jb->ops->data + jb->size;
		jb->size += size;
	}
	IS_COMMIT((STRPTR)ret+size) = 0;
	return ret;
}

/*** Alloc a new rollback segment (data from file) ***/
static char new_rbseg(JBuf jb, ULONG size)
{
	RBSeg new;
	if( jb->rbseg == NULL )
	{
		alloc: /* Adjust size */
		size &= ~(UNDO_CHUNK-1); size += UNDO_CHUNK;

		if( ( new = (RBSeg) AllocVec(size+sizeof(*new)-1, MEMF_PUBLIC) ) )
		{
			new->max  = size;
			new->prev = jb->rbseg;
			new->next = NULL;
			if(jb->rbseg) jb->rbseg->next = new;
			else          jb->rbseg       = new;
		}
		else return 0;
	}
	else if( jb->rbsz + size > jb->rbseg->max )
	{
		/* This time, fully use segment capacity */
		size -= jb->rbseg->max - jb->rbsz;
		goto alloc;
	}
	return 1;
}

/*** Do a savepoint ***/
void commit_work( Project p )
{
	/* Remove last savepoint, if any */
	if(p->savepoint != NULL && p->savepoint != OUT_OF_DATE)
		IS_COMMIT(p->savepoint) = 0;
	/* Commit last operation done */
	if(p->undo.ops != NULL)
		IS_COMMIT(p->savepoint = p->undo.ops->data + p->undo.size) = 1;
	else
		p->savepoint = NULL;
}

/*** Free memory used for one project's undo buf ***/
void flush_undo_buf( JBuf jb )
{
	RBOps jd, prev;
	UWORD size;

	for(jd=jb->ops, size=jb->size; jd; size=jd->size, prev=jd->prev, FreeVec(jd), jd=prev)
	{
		register UBYTE  type;
		register STRPTR op;
		/* Some operations contain non-freed data */
		while(size != 0)
		{
			op    = jd->data + size;
			type  = LAST_TYPE(op);
			size -= SizeOf[ type ];
			if( IS_COMMIT(op) ) PRJ(jb)->savepoint = OUT_OF_DATE;
			op    = jd->data + size;
			switch ( type )
			{
				case REM_LINE:  free_line(  ((RemLine)op)->line ); break;
				case JOIN_LINE: free_line( ((JoinLine)op)->old  ); break;
			}
		}
	}
	/* Free the rollback segments */
	for(jd = (RBOps)jb->rbseg; jd; prev = jd->prev, FreeVec(jd), jd = prev);
	/* Reset struct fields to 0 */
	memset(jb, 0, (ULONG) &((JBuf)0L)->rbtype);
}

/*** Register a character added in the buffer ***/
void reg_add_chars(JBuf jb, LINE *ln, ULONG pos, UWORD nb)
{
	AddChar buf;

	PRE_CHECK( jb );

	/* Reuse old registered operation? */
	if(jb->op == ADD_CHAR && jb->line == ln)
	{
		buf = (AddChar) last_operation( jb );
		/* If character position is just after the previous one && **
		** If it wasn't a committed operation (must be unchanged)  */
		if(pos == buf->pos + buf->nbc && !buf->commit)
			/* Yes! */
			jb->nbc = pos+1, buf->nbc += nb;
		else
			goto new_op;
	}
	else /* Register the new operation */
	{
		new_op:
		if((buf = new_undo_buf(jb, sizeof(*buf))))
		{
			jb->nbc   = (buf->pos = pos)+1;
			buf->line = jb->line  = ln;
			buf->type = jb->op    = ADD_CHAR;
			buf->nbc  = nb;
		}
	}
}

/*** Copy a string into rollback segment ***/
void copy_rbseg(JBuf jb, STRPTR str, ULONG size)
{
	RBSeg  rbs = jb->rbseg;
	STRPTR dst = rbs->data + jb->rbsz;

	if(jb->rbsz + size > rbs->max)
	{
		/* The string can never be spread over more than 2 buffers */
		CopyMem(str, dst, jb->rbsz = rbs->max-jb->rbsz); str += jb->rbsz;
		CopyMem(str, (jb->rbseg = rbs->next)->data, jb->rbsz = size-jb->rbsz);
	}
	else CopyMem(str, dst, size), jb->rbsz+=size;
}

/*** Move a string inside rollback segment ***/
void move_rbseg(JBuf jb, STRPTR ins, UWORD size, ULONG off)
{
	STRPTR dst,  src;
	RBSeg  dmin, smin;

	src = (smin = dmin = jb->rbseg)->data + jb->rbsz - 1;
	{	/* Compute search final position: current + off bytes in rb segment */
		register ULONG nb = jb->rbsz + off;
		while( nb > dmin->max )
			nb -= dmin->max, dmin = dmin->next;

		dst = dmin->data + (jb->rbsz = nb) - 1;
	}	jb->rbseg = dmin;

	/* Move size bytes from src to dst */
	while( size-- )
	{
		if(src < smin->data) smin=smin->prev, src=smin->data+smin->max-1;
		if(dst < dmin->data) dmin=dmin->prev, dst=dmin->data+dmin->max-1;
		*dst-- = *src--;
	}
	/* Then insert string "ins" of "off" bytes */
	for(src=ins+off-1; off--; )
	{
		if(dst < dmin->data) dmin=dmin->prev, dst=dmin->data+dmin->max-1;
		*dst-- = *src--;
	}
}

/*** Register some characters removed from the buffer ***/
void reg_rem_chars(JBuf jb, LINE *ln, ULONG s, ULONG e)
{
	RemChar buf;

	PRE_CHECK( jb );

	/* Previous operation was the converse one */
	if(jb->op == ADD_CHAR && jb->line == ln && s == e)
	{
		AddChar buf = (AddChar)last_operation( jb );
		/* Remove the last enterred char is very frequent */
		if(buf->pos <= e && e < buf->pos+buf->nbc && !buf->commit)
		{
			if(buf->nbc == 1) {
				if( pop_last_op(jb, ADD_CHAR, 0) == PRJ(jb)->savepoint )
					unset_modif_mark(PRJ(jb), TRUE);
			} else buf->nbc--;
			return;
		}
	}
	/* Optimize if previous operation was the same */
	if(jb->op == REM_CHAR && jb->line == ln)
	{
		buf = (RemChar) last_operation( jb );
		/* Ranges don't match */
		if((LONG)e < (LONG)buf->pos-1 || s > buf->pos || buf->commit)
			goto new_op;
		else
			if( new_rbseg(jb, e-s+1) )
			{
				if(s < buf->pos) {
					s = buf->pos-s; buf->pos -= s; e -= s;
					move_rbseg(jb, ln->stream+buf->pos, buf->nbc, s);
					buf->nbc += s;
				} else s = 0;

				if(e >= buf->pos)
					copy_rbseg(jb, ln->stream+buf->pos+s, e-buf->pos+1),
					buf->nbc += e-buf->pos+1;
				jb->nbc = buf->pos;
			}
	}
	else /* New operation */
	{
		new_op:
		if(NULL != (buf = new_undo_buf(jb, sizeof(*buf))) &&
		   0    != new_rbseg(jb, buf->nbc = e-s+1))
		{
			copy_rbseg(jb, ln->stream+s, buf->nbc);
			buf->line = jb->line = ln;
			buf->pos  = jb->nbc  = s;
			buf->type = jb->op   = REM_CHAR;
		}
	}
}

/*** Register a entire line removed/added ***/
void reg_rem_line(JBuf jb, LINE *ln)
{
	RemLine buf;

	PRE_CHECK( jb );

	if( NULL != (buf = new_undo_buf(jb, sizeof(*buf))) )
	{
		buf->line  = ln;
		buf->type  = jb->op = REM_LINE;
		buf->after = (ln->prev ? ln->prev : NULL);
		jb->line   = (ln->next ? ln->next : ln);
		jb->nbc    = 0;
	}
}

/*** Register two lines joined ***/
void reg_join_lines(JBuf jb, LINE *ln, LINE *ln2)
{
	JoinLine buf;

	PRE_CHECK( jb );

	if( NULL != (buf = new_undo_buf(jb, sizeof(*buf))) )
	{
		/* Keep original pointers since further modif. may reference them */
		buf->line = ln;
		buf->old  = ln2;
		buf->type = jb->op  = JOIN_LINE;
		buf->pos  = jb->nbc = ln->size;
		jb->line  = ln;
	}
}

/*** Group of modifications ***/
void reg_group_by( JBuf jb )
{
	GroupBy buf;

	/* No PRE_CHECK since GROUP_BY type doesn't imply any modif. */
	if( NULL != (buf = new_undo_buf(jb, sizeof(*buf))) )
	{
		jb->op = GROUP_BY;
		buf->type = GROUP_BY;
	}
}

/*** References may not be synchronized anymore ***/
void adjust_ccp( Project p )
{
	/* The problem when user undoes or redoes something is that **
	** line references may not be valid anymore, so readjust    */
	register LINE  *ln;
	register ULONG  nb;

	for(ln=p->the_line,nb=0; ln && ln->flags == 0; ln=ln->next, nb++);
	if(ln->next != NULL)
	{
		char YPinfYC = p->ccp.yp < p->ccp.yc;
		if(YPinfYC) p->ccp.yp=nb, p->ccp.line =ln;
		else        p->ccp.yc=nb, p->ccp.cline=ln;
		{	register LINE  *tmp;
			register ULONG  nbl;
			for(tmp=ln,nbl=nb; tmp; tmp=tmp->next,nbl++)
				if( tmp->flags != 0 ) ln = tmp, nb = nbl;
		}
		if(YPinfYC) p->ccp.yc=nb, p->ccp.cline=ln;
		else        p->ccp.yp=nb, p->ccp.line =ln;
	}
	else p->ccp.select = 0;
}

/*** Jump to the last modified line ***/
void last_modif( JBuf jb, char dirtiness )
{
	LINE *ln; ULONG nb; Project p;

	if(jb->line == NULL) return;

	/* Get line number of the last modified line */
	for(nb=0, p=PRJ(jb), ln=p->the_line; ln && ln!=jb->line; ln=ln->next, nb++);

/*	if(ln == NULL) {
		printf("Line not found! (0x%08x - 0x%08x)", jb->line, p->the_line);
		print_n(jb->line->stream, jb->line->size);
		return;
	} */

	p->nbrwc = x2pos(ln, jb->nbc);

	if(dirtiness == LINES_DIRTY)
	{
		register ULONG nbl = nb;
		/* Be sure "show" is the right pointer to the top_line */
		if( p->top_line >= p->max_lines ) p->top_line = p->max_lines-1;
		if( p->top_line >= nbl )
		{
			while(nbl < p->top_line) nbl++, ln=ln->next;
			p->show = ln;
		}
		/* and edited to nbl field */
		if( p->nbl >= p->max_lines ) p->nbl = p->max_lines-1;
		if( p->nbl >= nbl )
		{
			while(nbl < p->nbl) nbl++, ln=ln->next;
			p->edited = ln;
		}
		/* References may not be valid */
		if(p->ccp.select) adjust_ccp(p);
	}
	move_to_line(p, nb, dirtiness);
}

/*** Free last operation ***/
STRPTR pop_last_op( JBuf jb, char type, char isgrouped )
{
	STRPTR ptr = NULL;
	jb->size -= SizeOf[ (unsigned char) type ];

	/* If chunk is empty, it can be freed now */
	if( jb->size == 0 )
	{
		RBOps tmp = jb->ops;

		jb->ops = tmp->prev;
		if( jb->ops ) jb->size = tmp->size;
		else          jb->op   = 0;

		FreeVec( tmp );
	}
	/* Get previous operation */
	if( jb->size != 0 )
	{
		ptr = jb->ops->data + jb->size;
		if( isgrouped == 0 ) {
			register STRPTR op = ptr - SizeOf[ jb->op = LAST_TYPE(ptr) ];
			switch( jb->op )
			{
				case ADD_CHAR:  jb->nbc  = ((AddChar) op)->pos + ((AddChar)op)->nbc; goto after;
				case REM_CHAR:  jb->nbc  = ((RemChar) op)->pos;   after:
				case JOIN_LINE: jb->line = ((JoinLine)op)->line;  break;
				case REM_LINE:  jb->line = ((RemLine) op)->after; break;
			}
		}
	}
	return ptr;
}

/*** Undo/redo the last operation ***/
void rollback( JBuf jb )
{
	static LINE *lastline;
	static ULONG lastpos;
	STRPTR ptr;
	char   group = 0, dirty = LINE_DIRTY, commit;
	JBuf   redolog;

	if(jb->ops == NULL) return;
	ptr      = jb->ops->data + jb->size;
	redolog  = jb->rbtype == 0 ? &PRJ(jb)->redo : &PRJ(jb)->undo;
	commit   = IS_COMMIT(ptr);
	jb->line = NULL;
	PRJ(jb)->state |= DONT_FLUSH;

	do {
		/* Cancel last operation */
		switch( LAST_TYPE(ptr) )
		{
			case ADD_CHAR:			/* Characters added to the buffer */
			{
				AddChar buf = (AddChar) (ptr - sizeof(*buf));

				/* Operations spread over several lines must be refreshed */
				if(group && jb->line != buf->line) dirty = LINES_DIRTY;

				/* Committed op must not be joined with the others */
				if(commit) redolog->op = 0;

		 		lastpos = buf->pos; lastline = buf->line;

				if((buf->nbc += buf->pos) > buf->line->size)
				{
					PRJ(jb)->max_lines--; dirty = LINES_DIRTY;
					if(buf->pos == 0 && jb->rbtype != 0) {
						/* Whole line added => Remove it entirely */
						if( lastline == jb->line ) jb->line = NULL;
						if( del_line(redolog, &PRJ(jb)->the_line, buf->line) )
							lastline = lastline->next;
						break;
					} else
						/* The last character keyed in this line was a '\n' */
						buf->nbc--, join_lines(redolog, buf->line, buf->line->next);
				}

				/* Remove the last enterred word if it's a standalone modif */
				if( buf->nbc > buf->pos )
				{
					if( group == 0 && buf->nbc > 1 && jb->rbtype == 0 )
					{
						register ULONG pos = backward_word(buf->line, buf->nbc-2);

						if(pos < buf->pos) pos = buf->pos;
						rem_chars(redolog, buf->line, pos, buf->nbc-1);
						/* If the operation is non-empty, do not pops it */
						if( ( buf->nbc = pos - buf->pos ) )
							IS_COMMIT(ptr) = 0, ptr = NO_MEANING_VAL, lastpos += buf->nbc;
					}
					else rem_chars(redolog, buf->line, buf->pos, buf->nbc-1);
				}
			}	break;
			case REM_CHAR:			/* Characters remove from the buffer */
			{
				RemChar op = (RemChar) (ptr - sizeof(*op));
				ULONG   sz = op->nbc;

				/* Operations spread over several lines must be refreshed */
				if(group && jb->line != op->line) dirty = LINES_DIRTY;

				/* Committed op must not be joined with the others */
				if(commit) redolog->op = 0;

				lastpos = op->pos; lastline = op->line;

				for(;;) {
					/* The line in the rollback segment may be spread over several chunk */
					if( jb->rbsz >= sz ) {
						insert_str(redolog, op->line, op->pos, jb->rbseg->data + jb->rbsz - sz, sz);
						jb->rbsz -= sz;
						break;
					} else {
						insert_str(redolog, op->line, op->pos, jb->rbseg->data, jb->rbsz);
						sz -= jb->rbsz;
						jb->rbsz = (jb->rbseg = jb->rbseg->prev)->max;
						FreeVec(jb->rbseg->next);
						jb->rbseg->next = NULL;
					}
				}
			}	break;
			case REM_LINE:			/* Cancel line removed */
			{
				RemLine buf = (RemLine) (ptr - sizeof(*buf));
				Project p   = PRJ(jb);
				lastpos = 0; lastline = buf->line;

				dirty = LINES_DIRTY;
				/* Restore line pointer */
				InsertAfter(buf->after, buf->line); p->max_lines++;
				reg_add_chars(redolog, buf->line, 0, buf->line->size+1);
				
				if( buf->after == NULL )				/* :-/ */
					buf->line->next = p->the_line,
					p->the_line->prev = buf->line,
					p->the_line = buf->line;

			}	break;
			case JOIN_LINE:		/* Two lines joined */
			{
				JoinLine buf = (JoinLine) (ptr - sizeof(*buf));

				buf->old->size = 0;
				dirty          = LINES_DIRTY;

				/* Insert characters added at the end of the first line into the second */
				insert_str(NULL, buf->old, 0, buf->line->stream+buf->pos, buf->line->size-buf->pos);

				if(buf->pos < buf->line->size)
					rem_chars(NULL, buf->line, buf->pos+1, buf->line->size);
				reg_add_chars(redolog, buf->line, buf->line->size, 1);
				lastpos = (lastline = buf->line)->size;

				InsertAfter(buf->line, buf->old); PRJ(jb)->max_lines++;
			}	break;
			case GROUP_BY:			/* Group of operations */
				reg_group_by(redolog);
				if( ( group = !group ) ) goto pop_it;
		}
		/* Get the first modified line */
		if(jb->line == NULL || (jb->rbtype == 0 ? lastline->prev != jb->line :
		                                          lastline->next == jb->line ))
			jb->line = lastline, jb->nbc = lastpos;

		/* Move cursor to the last modification */
		if(group == 0) last_modif( jb, dirty );

		/* Partial op cancelled : do not remove from rollback seg now */
		if( ptr == NO_MEANING_VAL ) break;

		/* Remove op from memory */
		pop_it: ptr = pop_last_op(jb, LAST_TYPE(ptr), group);

	}	while( group );

	/* Transfer commit flag and savepoint into redolog */
	if( commit )
		IS_COMMIT(PRJ(jb)->savepoint = redolog->ops->data + redolog->size) = 1;

	PRJ(jb)->state &= ~DONT_FLUSH;

	/* Last savepoint reached or left? */
	if((PRJ(jb)->state & MODIFIED) == 0)
		set_modif_mark(PRJ(jb));
	else if(jb->rbtype ? commit : PRJ(jb)->savepoint == ptr)
		unset_modif_mark(PRJ(jb), TRUE);
}
