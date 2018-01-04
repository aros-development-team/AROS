/*** Debug.h : Simple ressource tracking for Memory allocation
**** Written by T.Pierron  june 27, 2001
***/
// Changed from DEBUG to DEBUG_EDIT to avoid conflicts with <aros/debug.h>
#if	DEBUG_EDIT				/* Do not include this code if DEBUG isn't defined */

#if 0
#ifdef	DEBUG_STUFF		/* One arbitrary file must defined this macro */

	#ifdef	AllocVec			/* Disable recursiv call! */
	#undef	AllocVec
	#undef	FreeVec
	#endif

	/* Track calling times of Alloc & Free */
	static nballoc = 0, nbfree = 0;
	void * my_alloc(unsigned long size, unsigned long type)
	{
		void * result;
		nballoc ++;
		result = (void *)AllocVec(size, type);
		return result;
	}

	void my_free(void * ptr)
	{
		nbfree ++;
		FreeVec(ptr);
	}
#else /* simply redirect calls */

	void * my_alloc (unsigned long, unsigned long);
	void   my_free  (void *);

	/** Redirect original calls **/
	#define	AllocMem(x,y)		my_alloc(x,y)
	#define	FreeMem(x,y)		my_free(x)

#endif

/** Redirect original calls **/
#define	AllocVec(x,y)		my_alloc(x,y)
#define	FreeVec(x)			my_free(x)
#endif

#ifdef	DEBUG_UNDO_STUFF		/* For undo/redo tracking */
extern UBYTE SizeOf[];
RBSeg rbs;
ULONG rbsz;

void print_n(char *p, int n)
{
	printf("String: \"");
	for( ; n; n--, p++)
	{
		if(*p < ' ') {
			char *msg = NULL;
			switch( *p )
			{
				case '\n': msg = "\\n"; break;
				case '\t': msg = "\\t"; break;
				case '\r': msg = "\\r"; break;
				case 27:   msg = "\\e"; break;
			}
			if(msg) printf("[1m%s[0m",msg);
			else    printf("[1m\\%02x[0m", *p);
		}
		else printf("%c",*p);
	}
	printf("\"\n");
}

void print_rbseg_buf( ULONG sz )
{
	for(;;) {
		/* The line in the rollback segment may be spread over several chunk */
		if( rbsz >= sz ) {
			print_n(rbs->data + (rbsz -= sz), sz);
			break;
		} else {
			print_n(rbs->data, rbsz);
			sz -= rbsz;
			rbsz = (rbs = rbs->prev)->max;
		}
	}
}

int get_ln(Project p, LINE *ln)
{
	LINE * l = p->the_line;
	int n = 0;
	while( l && l != ln ) l = l->next, n++;
	return (l ? n : -1);
}

/** Show content of rollback segments (note: this is quick and dirty) **/
void show_modifs( JBuf jb )
{
	RBOps buf; UWORD size, op, i = 0;

	rbs  = jb->rbseg;
	rbsz = jb->rbsz;
	printf("[1mRBSeg info: size=%d (0x%08lx) rbsz:%d (0x%08lx)[0m\n",
	       jb->size,jb->ops,jb->rbsz,jb->rbseg);

	for(buf = jb->ops, size = jb->size; buf; size=buf->size, buf=buf->prev)
	{
		while(size != 0)
		{
			op = LAST_TYPE(buf->data + size);
			size -= SizeOf[ op ];
			switch( op )
			{
				case ADD_CHAR:
				{
					AddChar buf2 = (AddChar) (buf->data + size);
					printf("%d [4mcharacters added[0m line %d, pos %d : ", buf2->nbc,
					       get_ln(PRJ(jb),buf2->line), buf2->pos);
					print_n(buf2->line->stream+buf2->pos, buf2->nbc);
				}	break;
				case REM_CHAR:
				{
					RemChar buf2 = (RemChar) (buf->data + size);
					printf("%d [4mcharacters removed[0m line %d, pos %d : ", buf2->nbc,
					       get_ln(PRJ(jb),buf2->line), buf2->pos);
					print_rbseg_buf(buf2->nbc);
				}	break;
				case REM_LINE:
					printf("[4mLine removed[0m\n");
					break;
				case JOIN_LINE:
				{	JoinLine buf2 = (JoinLine) (buf->data + size);
					printf("[4mLine joined[0m pos %d\n", buf2->pos);
				}	break;
				case GROUP_BY:
					printf("[4m%s[0m of group\n", i&1 ? "End" : "Start"); i++;
					break;
				default:
					printf("[2mUnsupported operation[0m (0x%x - size = %d)!\n", op, size);
					return;
			}
		}
	}
}
#endif

#endif

