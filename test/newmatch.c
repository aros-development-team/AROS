#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

/****************************************************************************************/

#define COMPTYPE_NORMAL  1 
#define COMPTYPE_PATTERN 2
#define COMPTYPE_UNKNOWN 3

/****************************************************************************************/

LONG My_MatchNext(struct AnchorPath *ap);

/****************************************************************************************/


static void showacflags(struct AChain *ac)
{
    BYTE flags = ac->an_Flags;

    printf("(");

    if (flags & DDF_PatternBit)
    {
	flags &= ~DDF_PatternBit;
	printf("DDF_PatternBit ");
    }

    if (flags & DDF_ExaminedBit)
    {
	flags &= ~DDF_ExaminedBit;
	printf("DDF_ExaminedBit ");
    }

    if (flags & DDF_Completed)
    {
	flags &= ~DDF_Completed;
	printf("DDF_Completed ");
    }

    if (flags & DDF_AllBit)
    {
	flags &= ~DDF_AllBit;
	printf("DDF_All ");
    }

    if (flags & DDF_Single)
    {
	 flags &= ~DDF_Single;
	 printf("DDF_Single ");
    }

    if (flags)
    {
	 printf("UNKNOWN = %8lx ", flags);
    }

    printf(")");
}

static void showaclist(struct AChain *ac)
{
    while(ac)
    {
	printf("achain: address = %8lx flags = %lx ", ac, ac->an_Flags);
	showacflags(ac);
	printf(" string=\"%s\"\n", ac->an_String);
	ac = ac->an_Child;
    }

}

/****************************************************************************************/

static void RemoveTrailingSlash(STRPTR s)
{
    LONG len = strlen(s);

    if (len >= 2)
    {
	if ((s[len - 1] == '/') &&
	    (s[len - 2] != '/'))
	{
	    s[len - 1] = '\0';
	}
    }
}

/****************************************************************************************/

static struct AChain *AllocAChain(LONG extrasize)
{
    return AllocVec(sizeof(struct AChain) + extrasize, MEMF_PUBLIC | MEMF_CLEAR);
}

/****************************************************************************************/

static void FreeAChain(struct AChain *ac)
{
    if (ac)
    {
	FreeVec(ac);
    }
}

/*****************************************************************************************

The job of Build_AChainList is to split the pattern string passed to MatchFirst into
path components. Most imporant rules (as found out after hours of testing on Amiga):

  - Each path component containing a pattern string is put into a single AChain
  - If there are several successive path components *without* pattern then this
    are merged into one single AChain.
  - No matter what: the last path component always gets into its own single AChain.

Examples: [<???>] is one AChain

  pictures                          [pictures]
  pictures/#?                       [pictures} [#?]
  work:                             [work:] []
  work:pictures                     [work:} [pictures]
  work:pictures/#?                  [work:pictures] [#?]
  work:pictures/aros                [work:pictures] [aros]
  work:pictures/aros/games          [work:pictures/aros] [games]
  work:#?/aros/games                [work:] [#?] [aros] [games}
  work:#?/#?/aros/games/quake       [work:} [#?] [#?] [aros/games] [quake]
  
*****************************************************************************************/

static LONG Build_AChainList(STRPTR pattern, struct AnchorPath *ap, struct AChain **retac)
{
    struct AChain 	*baseac = 0, *prevac = 0, *ac;
    STRPTR 		patterncopy = 0;
    STRPTR		patternstart, patternend, patternpos;
    LONG 		len, error = 0;
    WORD		comptype = COMPTYPE_UNKNOWN;
    WORD		compcount = 0;
    WORD		i, i2;
    UBYTE		c;

    *retac = 0;

    len = strlen(pattern);

    patterncopy = AllocVec(len + 1, MEMF_PUBLIC);
    if (!patterncopy) goto done;

    strcpy(patterncopy, pattern);

    RemoveTrailingSlash(patterncopy);

    patternstart = patterncopy;

    patternpos = strchr(patterncopy, ':');
    if (!patternpos)
    {
	comptype = COMPTYPE_UNKNOWN;
	patternpos = patternstart;
	patternend = patternstart;
    }
    else
    {
	comptype = COMPTYPE_NORMAL;
	patternend = patternpos++;
	compcount = 1;
    }

    do
    {
	for(;;)
	{
	    c = *patternpos;
	    if (c == '/')
	    {
		    if (comptype == COMPTYPE_UNKNOWN)
		    {
			    comptype = COMPTYPE_NORMAL;
			    patternend = patternpos;
		    }
		    else if (comptype == COMPTYPE_NORMAL)
		    {
			    patternend = patternpos;
			    compcount++;
		    }
		    if (comptype == COMPTYPE_PATTERN)
		    {
			    patternend = patternpos;
			    break;		
		    }
	    }
	    else if (c == '\0')
	    {
		    if (comptype == COMPTYPE_UNKNOWN)
		    {
			    comptype = COMPTYPE_NORMAL;
			    patternend = patternpos;
			    break;
		    }
		    if (comptype == COMPTYPE_NORMAL)
		    {
			    compcount++;
			    break;
		    }
		    patternend = patternpos;
		    break;
	    }
	    else if ((c == '#') ||
		     (c == '~') ||
		     (c == '[') ||
		     (c == ']') ||
		     (c == '?') ||
		     (c == '*') ||
		     (c == '(') ||
		     (c == ')') ||
		     (c == '|'))
	    {
		ap->ap_Flags |= APF_ITSWILD;

		if (comptype == COMPTYPE_NORMAL)
		{
		    break;
		}
		comptype = COMPTYPE_PATTERN;
	    }

	    patternpos++;

	} /* for(;;) */

	len = (LONG)(patternend - patternstart + 2);
	if (comptype == COMPTYPE_PATTERN) len = len * 2 + 2;

	ac = AllocAChain(len);
	if (!ac)
	{
	    error = ERROR_NO_FREE_STORE;
	    goto done;
	}

	if (comptype == COMPTYPE_NORMAL)
	{
	    if (*patternend == '\0')
	    {
		strcpy(ac->an_String, patternstart);
	    } else {
		c = patternend[1];
		patternend[1] = '\0';
		strcpy(ac->an_String, patternstart);
		patternend[1] = c;
	    }
	} else {
	    if (*patternend == '\0')
	    {
		i = ParsePatternNoCase(patternstart, ac->an_String, len);
	    } else {
		c = patternend[1];
		patternend[1] = '\0';
		i = ParsePatternNoCase(patternstart, ac->an_String, len);
		patternend[1] = c;
	    }
	    if (i == -1)
	    {
		error = ERROR_BAD_TEMPLATE;
		FreeAChain(ac);ac = 0;
		goto done;
	    }
	    ac->an_Flags |= DDF_PatternBit;
	}

	RemoveTrailingSlash(ac->an_String);

	if (!prevac)
	{
	    baseac = ac;
	} else {
	    prevac->an_Child = ac;
	    ac->an_Parent = prevac;
	}

	prevac = ac;

	patternpos = patternend;
	comptype = COMPTYPE_UNKNOWN;
	patternstart = patternend = patternpos + 1;
	compcount = 0;

    } while (*patternpos++ != '\0');

done:
    if (patterncopy) FreeVec(patterncopy);

    if (!error)
    {
	*retac = baseac;
    }
    else if (baseac)
    {
#define nextac prevac

	ac = baseac;
	while(ac)
	{
	    nextac = ac->an_Child;
	    FreeAChain(ac);
	    ac = nextac;
	}
    }

    return error;
}

/****************************************************************************************/

static LONG MakeResult(struct AnchorPath *ap)
{
    LONG error = 0;

    ap->ap_Info = ap->ap_Current->an_Info;
    if (ap->ap_Strlen)
    {
	ap->ap_Buf[0] = 0;
	if (NameFromLock(ap->ap_Current->an_Lock, ap->ap_Buf, ap->ap_Strlen))
	{
	    if (!AddPart(ap->ap_Buf, ap->ap_Current->an_Info.fib_FileName, ap->ap_Strlen))
	    {
		error = IoErr();
	    }
	} else {
	    error = IoErr();
	}
    }

    return error;
}

/****************************************************************************************/

LONG My_MatchFirst(STRPTR pattern, struct AnchorPath *ap)
{
    struct AChain 	*ac;
    LONG 		error;
    LONG 		success;
    BPTR 		origdir;

    ap->ap_Flags = 0;
    ap->ap_Base = 0;
    ap->ap_Current = 0;

    origdir = CurrentDir(0);
    CurrentDir(origdir);

    error = Build_AChainList(pattern, ap, &ac);
    if (error == 0)
    {
	ap->ap_Base = ap->ap_Current = ac;

	error = My_MatchNext(ap);

    } /* if (error == 0) */

done:
    CurrentDir(origdir);

    printf("My_MatchFirst: returning %ld. Ac = %lx\n", error, ac);

    SetIoErr(error);

    return error;
}

/****************************************************************************************/

LONG My_MatchNext(struct AnchorPath *ap)
{
    struct AChain 	*ac = ap->ap_Current;
    BPTR 		origdir;
    LONG 		error = 0;

    origdir = CurrentDir(0);
    CurrentDir(origdir);

    ap->ap_Flags &= ~APF_DIDDIR;
    
    /* Check if we are asked to enter a directory, but only do this
       if it is really possible */
       
    if ((ap->ap_Flags & APF_DODIR) &&
        (ac->an_Flags & DDF_ExaminedBit) &&
	(ac->an_Info.fib_DirEntryType > 0) &&
	(ac->an_Child == NULL))
    {
        /* Alloc a new AChain. Make it the active one. Set its string to "#?" and
	   mark it with DDF_AllBit Flag to indicate that this is a "APF_DODIR-AChain".
	   This is important for "back steppings", because "APF_DODIR-AChains" must
	   be removed and freed then. */
	   
        if ((ac->an_Child = AllocAChain(1)))
	{
	    ac->an_Child->an_Parent = ac;
	    ac = ac->an_Child;
	    ap->ap_Current = ac;
	    
	    ac->an_String[0] = P_ANY;
	    ac->an_String[1] = 0;
	    ac->an_Flags = DDF_PatternBit | DDF_AllBit;	    
	}
    }

    
    /* Main loop for AChain traversing */
    
    for(;;)
    {
        BOOL must_go_back = FALSE;
	
	/* Check for user breaks (CTRL_C, ...) */
	 
	if (ap->ap_BreakBits)
	{
	    ap->ap_FoundBreak = CheckSignal(ap->ap_BreakBits);
	    if (ap->ap_FoundBreak)
	    {
	        error = ERROR_BREAK;
		goto done;
	    }
	}
	
	/* Check if AChain must be "setup" */
	
	if (!(ac->an_Flags & DDF_ExaminedBit))
	{
	    /* This AChain must be "setup". First AChain->an_Lock must point
	       to the parent directory! */
	   
	    if (ac->an_Parent)
	    {
		CurrentDir(ac->an_Parent->an_Lock);
		ac->an_Lock = Lock(ac->an_Parent->an_Info.fib_FileName, SHARED_LOCK);
	    } else {
		ac->an_Lock = DupLock(origdir);
	    }

	    if (!ac->an_Lock)
	    {
		error = IoErr();
		goto done;
	    }

	    CurrentDir(ac->an_Lock);
	    
	    if (ac->an_Flags & DDF_PatternBit)
	    {
		if (!Examine(ac->an_Lock, &ac->an_Info))
		{
		    error = IoErr();
		    goto done;
		}
		ac->an_Flags |= DDF_ExaminedBit;

	    } /* if (ac->an_Flags & DDF_PatternBit) */
	    else
	    {
		BPTR lock;
		LONG success;
		
		if (!(lock = Lock(ac->an_String, SHARED_LOCK)))
		{
		    if (ac->an_Parent)
		    {
		        must_go_back = TRUE;
		    }
		    else
		    {
		        error = IoErr();
		        goto done;
		    }
		    
		} /* if (!(lock = Lock(ac->an_String, SHARED_LOCK))) */
		else
		{
		    success = Examine(lock, &ac->an_Info);
		    UnLock(lock);

		    if (!success)
		    {
			error = IoErr();
			goto done;
		    }

		    /* This strcpy is necessary, because in case
		       of empty ac->an_String("") fib_FileName would
		       get directory name which it must not! */
		       
		    strcpy(ac->an_Info.fib_FileName, ac->an_String);
		    
		    ac->an_Flags |= DDF_ExaminedBit;

		    /* if this is a file, but there are still more path components to
		       follow then we have to go back one step (AChain) */

		    if (ac->an_Child && (ac->an_Info.fib_DirEntryType < 0))
		    {
			must_go_back = TRUE;
		    }
		
		} /* if (!(lock = Lock(ac->an_String, SHARED_LOCK))) else ... */
		
	    } /*  /* if (ac->an_Flags & DDF_PatternBit) else ... */ 

 	} /* if (!(ac->an_Flags & DDF_ExaminedBit)) */
	else
	{
	    /* When an AChain which is *not* a pattern already had DDF_PatternBit
	       set, then this means ERROR_NO_MORE_ENTRIES, unless the AChain
	       has a parent. In this case go back one step */
	       
	    if (!(ac->an_Flags & DDF_PatternBit))
	    {
	        if (ac->an_Parent)
		{
		    must_go_back = TRUE;
		}
		else
		{
	            error = ERROR_NO_MORE_ENTRIES;
		    goto done;
		}
	    }
	}
	
	/* Here we can be sure that the actual AChain is setup, ie: it will
	   have ac->an_Lock set correctly and to indicate this DDF_ExaminedBit
	   was set */
	   
	CurrentDir(ac->an_Lock);
	
	if (ac->an_Flags & DDF_PatternBit)
	{
	    if(ExNext(ac->an_Lock, &ac->an_Info))
	    {
	        if (MatchPatternNoCase(ac->an_String, ac->an_Info.fib_FileName))
		{
		    /* this file matches the pattern in ac->an_String. If there
		       are no more AChains to follow then we have found a result
		       --> break. */
		       
		    if (!ac->an_Child) break;

		} else {
		    /* did not match. Go to top of "for(;;)" loop */
		    continue;
		}
	    } else {
	        error = IoErr();
		if (error != ERROR_NO_MORE_ENTRIES) goto done;
		must_go_back = TRUE;
	    }
	}
	
	if (must_go_back)
	{
	    if (!ac->an_Parent)
	    {
	        error = ERROR_NO_MORE_ENTRIES;
		goto done;
	    }
	    
	    UnLock(ac->an_Lock);
	    ac->an_Lock = NULL;
	    ac->an_Flags &= ~(DDF_ExaminedBit);
	    
	    ap->ap_Current = ac->an_Parent;
	    
	    if (ac->an_Flags & DDF_AllBit)
	    {
	        ap->ap_Current->an_Child = NULL;
		FreeAChain(ac);
		ap->ap_Flags |= APF_DIDDIR;
		break;
	    }
	    
	    ac = ap->ap_Current;
	    
	} /* if (must_go_back) */
	else
	{
	    if (!ac->an_Child) break;

	    ac = ac->an_Child;
	    ap->ap_Current = ac;
	}

    } /* for(;;) */

    error = MakeResult(ap);
    
done:
    CurrentDir(origdir);

    SetIoErr(error);
    
    ap->ap_Flags &= ~APF_DODIR;
    
    return error;
	
}

/****************************************************************************************/

void My_MatchEnd(struct AnchorPath *ap)
{
    struct AChain *ac = ap->ap_Base, *acnext;

    while(ac)
    {
	acnext = ac->an_Child;

	if (ac->an_Lock) UnLock(ac->an_Lock);
	FreeAChain(ac);

	ac = acnext;
    }
    
    ap->ap_Current = NULL;
    ap->ap_Base = NULL;
}

/****************************************************************************************/

#define ARG_TEMPLATE "FILE/A,ALL/S"
#define ARG_FILE 0
#define ARG_ALL  1
#define NUM_ARGS 2

/****************************************************************************************/

static char s[300];
static char *filename;
static BOOL all;
static struct RDArgs *myargs;
static LONG args[NUM_ARGS];

/****************************************************************************************/

static void cleanup(char *msg)
{
    if (msg) printf("newmatch: %s\n", msg);

    if (myargs) FreeArgs(myargs);
        
    exit(0);
}

/****************************************************************************************/

static void doserror(void)
{
    Fault(IoErr(), 0, s, 255);
    cleanup(s);
}

/****************************************************************************************/

static void getarguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
        doserror();
    }
    
    filename = (char *)args[ARG_FILE];
    all = args[ARG_ALL] ? TRUE : FALSE;  
}

/****************************************************************************************/

static void my_matchme(char *pattern, BOOL all)
{
    struct AnchorPath stackap[2], *ap;
    struct AChain *ac;
    LONG error = 0;

    ap = (struct AnchorPath *)((((ULONG)stackap) + 3) & ~3);

    memset(ap, 0, sizeof(struct AnchorPath));

    error =  My_MatchFirst(pattern, ap);

    if (error != 0)
    {
	printf("MatchFirst: error = %ld\n", error);
    }
    else
    {
        printf("direntrytype = %d\n", ap->ap_Info.fib_DirEntryType);
        if (!(ap->ap_Flags & APF_ITSWILD) &&
	     (ap->ap_Info.fib_DirEntryType > 0))
	{
	     /* pattern was an explicitely named directory */
	     ap->ap_Flags |= APF_DODIR;
	}
	
	printf("ap_Flags = %lx\n", ap->ap_Flags);
	NameFromLock(ap->ap_Current->an_Lock, s, 300);
	printf("BaseLock = \"%s\"\n", s);

	showaclist(ap->ap_Base);

	while(error == 0)
	{
	    if (ap->ap_Flags & APF_DIDDIR)
	    {
	        printf("DIDDIR: ");
	    } else {
	        if (all && (ap->ap_Info.fib_DirEntryType > 0))
		{
		    ap->ap_Flags |= APF_DODIR;
		    printf("DOING DIR: ");
		}
	    }
	    printf("fib_FileName = \"%s\"\n", ap->ap_Info.fib_FileName);

	    error = My_MatchNext(ap);
	}

    }

    My_MatchEnd(ap);
    
}

/****************************************************************************************/
/****************************************************************************************/

int main(void)
{
    getarguments();
    my_matchme(filename, all);
    cleanup(0);
    return 0;
}

/****************************************************************************************/
