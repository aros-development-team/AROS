#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#include <utility/tagitem.h>
#include <utility/utility.h>
#include <stdlib.h>

/* Don't define symbols before the entry point. */
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct UtilityBase *UtilityBase;
extern const char dosname[];
static LONG tinymain(void);

__AROS_LH0(LONG,entry,struct ExecBase *,sysbase,,)
{
    __AROS_FUNC_INIT
    LONG error=RETURN_FAIL;

    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary((STRPTR)dosname,39);
    UtilityBase=(struct UtilityBase *)OpenLibrary(UTILITYNAME,39);
    if(DOSBase!=NULL && UtilityBase != NULL)
    {
	error=tinymain();
	CloseLibrary((struct Library *)DOSBase);
    }
    return error;
    __AROS_FUNC_EXIT
}

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct UtilityBase *UtilityBase;
const char dosname[]="dos.library";

char ** files;
int num_files, max_files;
char ** dirs;
int num_dirs, max_dirs;

static int strlen (const char * ptr)
{
    int len=0;

    while (*ptr++) len++;

    return len;
}

static int AddEntry (const char * entry, int flag /* 0-dir, 1-file */)
{
    char *** entries;
    int * num_entries, * max_entries;
    char * dup;
    int len;

    if (flag)
    {
	entries = &files;
	num_entries = &num_files;
	max_entries = &max_files;
    }
    else
    {
	entries = &dirs;
	num_entries = &num_dirs;
	max_entries = &max_dirs;
    }

    if (*num_entries == *max_entries)
    {
	int new_max = *max_entries + 128;
	char ** new_entries;

	new_entries = AllocVec (sizeof(char *)*new_max, MEMF_ANY);

	if (!new_entries)
	    return 0;

	if (*num_entries)
	{
	    CopyMemQuick (*entries, new_entries, sizeof(char *)* *num_entries);
	    FreeVec (*entries);
	}

	*entries = new_entries;
	*max_entries = new_max;
    }

    len = strlen (entry) + 1;

    if (!(dup = AllocVec (len, MEMF_ANY)) )
	return 0;

    CopyMem ((char *)entry, dup, len);

    (*entries)[(*num_entries) ++] = dup;
    return 1;
}

static int compare_strings (const void * s1, const void * s2)
{
    return Stricmp (*(char **)s1, *(char **)s2);
}

#if 1
#include <sys/types.h>
/* #include <stdlib.h> */

static inline char	*med3 __P((char *, char *, char *, int (*)()));
static inline void	 swapfunc __P((char *, char *, int, int));

#define min(a, b)       (a) < (b) ? a : b

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n) {               \
	long i = (n) / sizeof (TYPE);                   \
	register TYPE *pi = (TYPE *) (parmi);           \
	register TYPE *pj = (TYPE *) (parmj);           \
	do {						\
		register TYPE	t = *pi;		\
		*pi++ = *pj;				\
		*pj++ = t;				\
	} while (--i > 0);                              \
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static inline void
swapfunc(a, b, n, swaptype)
	char *a, *b;
	int n, swaptype;
{
	if(swaptype <= 1)
		swapcode(long, a, b, n)
	else
		swapcode(char, a, b, n)
}

#define swap(a, b)                                      \
	if (swaptype == 0) {                            \
		long t = *(long *)(a);                  \
		*(long *)(a) = *(long *)(b);            \
		*(long *)(b) = t;                       \
	} else						\
		swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n)        if ((n) > 0) swapfunc(a, b, n, swaptype)

static inline char *
med3(a, b, c, cmp)
	char *a, *b, *c;
	int (*cmp)();
{
	return cmp(a, b) < 0 ?
	       (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ))
	      :(cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c ));
}

void
qsort(a, n, es, cmp)
	void *a;
	size_t n, es;
	int (*cmp)();
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int d, r, swaptype, swap_cnt;

loop:	SWAPINIT(a, es);
	swap_cnt = 0;
	if (n < 7) {
		for (pm = a + es; pm < (char *) a + n * es; pm += es)
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}
	pm = a + (n / 2) * es;
	if (n > 7) {
		pl = a;
		pn = a + (n - 1) * es;
		if (n > 40) {
			d = (n / 8) * es;
			pl = med3(pl, pl + d, pl + 2 * d, cmp);
			pm = med3(pm - d, pm, pm + d, cmp);
			pn = med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = med3(pl, pm, pn, cmp);
	}
	swap(a, pm);
	pa = pb = a + es;

	pc = pd = a + (n - 1) * es;
	for (;;) {
		while (pb <= pc && (r = cmp(pb, a)) <= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a)) >= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		swap_cnt = 1;
		pb += es;
		pc -= es;
	}
	if (swap_cnt == 0) {  /* Switch to insertion sort */
		for (pm = a + es; pm < (char *) a + n * es; pm += es)
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}

	pn = a + n * es;
	r = min(pa - (char *)a, pb - pa);
	vecswap(a, pb - r, r);
	r = min(pd - pc, pn - pd - es);
	vecswap(pb, pn - r, r);
	if ((r = pb - pa) > es)
		qsort(a, r / es, es, cmp);
	if ((r = pd - pc) > es) {
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r / es;
		goto loop;
	}
/*		qsort(pn - r, r / es, es, cmp);*/
}
#endif

static LONG tinymain(void)
{
    char *args[1]={ 0 };
    struct RDArgs *rda;
    BPTR dir;
    LONG loop;
    struct ExAllControl *eac;
    struct ExAllData *ead;
    static UBYTE buffer[4096];
    LONG error=0;

    VPrintf ("dir\n", NULL);

    rda=ReadArgs("DIR",(ULONG *)args,NULL);
    if(rda!=NULL)
    {
	dir=Lock(args[0]!=NULL?args[0]:"",SHARED_LOCK);
	if(dir)
	{
	    eac=AllocDosObject(DOS_EXALLCONTROL,NULL);
	    if(eac!=NULL)
	    {
		int t;
		LONG argv[3];

		eac->eac_LastKey=0;
		do
		{
		    loop=ExAll(dir,(struct ExAllData *)buffer,4096,ED_COMMENT,eac);
		    if(!loop&&IoErr()!=ERROR_NO_MORE_ENTRIES)
		    {
			error=RETURN_ERROR;
			break;
		    }
		    if(eac->eac_Entries)
		    {
			ead=(struct ExAllData *)buffer;
			do
			{
			    if (!AddEntry (ead->ed_Name, ead->ed_Type < 0))
			    {
				loop = 0;
				error=RETURN_ERROR;
				VPrintf ("out of memory\n", NULL);
				break;
			    }

			    ead=ead->ed_Next;
			}while(ead!=NULL);
		    }
		}while(loop);
		FreeDosObject(DOS_EXALLCONTROL,eac);

		if (!error)
		{
		    if (num_dirs)
		    {
			qsort (dirs, num_dirs, sizeof (char *),
			    compare_strings);

			for (t=0; t<num_dirs; t++)
			{
			    argv[0] = (LONG) dirs[t];
			    VPrintf ("    %-25.s <DIR>\n", argv);
			}
		    }

		    if (num_files)
		    {
			qsort (files, num_files, sizeof (char *),
			    compare_strings);

			for (t=0; t<num_files; t+=3)
			{
			    argv[0] = (LONG) (files[t]);
			    argv[1] = (LONG) (t+1 < num_files ? files[t+1] : "");
			    argv[2] = (LONG) (t+2 < num_files ? files[t+2] : "");
			    VPrintf ("%-25.s %-25.s %s\n", argv);
			}
		    }
		}

		if (max_dirs)
		{
		    for (t=0; t<num_dirs; t++)
		    {
			FreeVec (dirs[t]);
		    }

		    FreeVec (dirs);

		    max_dirs = 0;
		    num_dirs = 0;
		}

		if (max_files)
		{
		    for (t=0; t<num_files; t++)
		    {
			FreeVec (files[t]);
		    }

		    FreeVec (files);

		    max_files = 0;
		    num_files = 0;
		}
	    }else
	    {
		SetIoErr(ERROR_NO_FREE_STORE);
		error=RETURN_ERROR;
	    }
	    UnLock(dir);
	}
	FreeArgs(rda);
    }else
	error=RETURN_FAIL;
    if(error)
	PrintFault(IoErr(),"List");
    return error;
}

