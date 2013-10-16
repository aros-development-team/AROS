/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************************/

#define MATCHFUNCS_NO_DUPLOCK	0

/****************************************************************************************/

#define COMPTYPE_NORMAL  1 
#define COMPTYPE_PATTERN 2
#define COMPTYPE_UNKNOWN 3

/****************************************************************************************/

LONG My_MatchNext(struct AnchorPath *AP);

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
	 printf("UNKNOWN = %8x ", flags);
    }

    printf(")");
}

static void showaclist(struct AChain *ac)
{
    while(ac)
    {
	printf("achain: address = %p flags = %x ", ac, ac->an_Flags);
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

static struct AChain *Match_AllocAChain(LONG extrasize, struct DosLibrary *DOSBase)
{
    return AllocVec(sizeof(struct AChain) + extrasize, MEMF_PUBLIC | MEMF_CLEAR);
}

/****************************************************************************************/

static void Match_FreeAChain(struct AChain *ac, struct DosLibrary *DOSBase)
{
    FreeVec(ac);
}

/*****************************************************************************************

The job of BuildAChainList is to split the pattern string passed to MatchFirst into
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

static LONG BuildAChainList(STRPTR pattern, struct AnchorPath *AP,
			    struct AChain **retac, struct DosLibrary *DOSBase)
{
    struct AChain 	*baseac = 0, *prevac = 0, *ac;
    STRPTR 		patterncopy = 0;
    STRPTR		patternstart, patternend, patternpos;
    LONG 		len, error = 0;
    WORD		comptype = COMPTYPE_UNKNOWN;
    WORD		compcount = 0;
    WORD		i;
    UBYTE		c;

    *retac = 0;

    len = strlen(pattern);

    patterncopy = AllocVec(len + 1, MEMF_PUBLIC);
    if (!patterncopy)
    {
        error = ERROR_NO_FREE_STORE;
	goto done;
    }
    
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
		     (c == '|') ||
		     (c == '%'))
	    {
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

	ac = Match_AllocAChain(len, DOSBase);
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
	    
	} /* if (comptype == COMPTYPE_NORMAL) */
	else
	{
	    if (*patternend == '\0')
	    {
		i = ParsePatternNoCase(patternstart, ac->an_String, len);
	    }
	    else
	    {
		c = patternend[1];
		patternend[1] = '\0';
		i = ParsePatternNoCase(patternstart, ac->an_String, len);
		patternend[1] = c;
	    }
	    
	    if (i == -1)
	    {
		error = ERROR_BAD_TEMPLATE;
		Match_FreeAChain(ac, DOSBase);ac = 0;
		goto done;
	    }
	    
	    if (i)
	    {
	        ac->an_Flags |= DDF_PatternBit;
		AP->ap_Flags |= APF_ITSWILD;
	    }
	    
	} /* if (comptype == COMPTYPE_NORMAL) else ... */

	RemoveTrailingSlash(ac->an_String);

	if (!prevac)
	{
	    baseac = ac;
	}
	else
	{
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
    FreeVec(patterncopy);

    if (!error)
    {
#if MATCHFUNCS_NO_DUPLOCK
        /*
	** No DupLock() here, because then we would have to UnLock it in MatchEnd
	** and we would not know any valid lock to which we could CurrentDir after,
	** because we must make sure there is a valid CurrentDir after MatchEnd.
	*/
	
        baseac->an_Lock = CurrentDir(0);
	CurrentDir(baseac->an_Lock);
#endif
	*retac = baseac;
    }
    else
    {
        AP->ap_Flags |= APF_NOMEMERR;
	
     	if (baseac)
	{
	    #define nextac prevac /* to not have to add another variable */

	    ac = baseac;
	    while(ac)
	    {
		nextac = ac->an_Child;
		Match_FreeAChain(ac, DOSBase);
		ac = nextac;
	    }
	}
    }

    return error;
}

/****************************************************************************************/

static LONG Match_MakeResult(struct AnchorPath *AP, struct DosLibrary *DOSBase)
{
    LONG error = 0;

    AP->ap_Info = AP->ap_Current->an_Info;
    if (AP->ap_Strlen)
    {
	AP->ap_Buf[0] = 0;
	if (NameFromLock(AP->ap_Current->an_Lock, AP->ap_Buf, AP->ap_Strlen))
	{
	    if (!AddPart(AP->ap_Buf, AP->ap_Current->an_Info.fib_FileName, AP->ap_Strlen))
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

LONG My_MatchFirst(STRPTR pat, struct AnchorPath *AP)
{
    struct AChain 	*ac;
    LONG 		error;

    AP->ap_Flags = 0;
    AP->ap_Base = 0;
    AP->ap_Current = 0;

    error = BuildAChainList(pat, AP, &ac, DOSBase);
    if (error == 0)
    {
	AP->ap_Base = AP->ap_Current = ac;

	error = My_MatchNext(AP);

    } /* if (error == 0) */

    printf("My_MatchFirst: returning %ld. Ac = %p\n", (long)error, ac);

    SetIoErr(error);

    return error;
}

/****************************************************************************************/

LONG My_MatchNext(struct AnchorPath *AP)
{
    struct AChain 	*ac = AP->ap_Current;
    BPTR 		origdir, old_current_lock;
    LONG 		error = 0;

    origdir = CurrentDir(0);
    CurrentDir(origdir);

    old_current_lock = ac->an_Lock;
    
    AP->ap_Flags &= ~APF_DIDDIR;
    
    /*
    ** Check if we are asked to enter a directory, but only do this
    ** if it is really possible
    */
       
    if ((AP->ap_Flags & APF_DODIR) &&
        (ac->an_Flags & DDF_ExaminedBit) &&
	(ac->an_Info.fib_DirEntryType > 0) &&
	(ac->an_Child == NULL))
    {
        /*
	** Alloc a new AChain. Make it the active one. Set its string to "#?" and
	** mark it with DDF_AllBit Flag to indicate that this is a "APF_DODIR-AChain".
	** This is important for "back steppings", because "APF_DODIR-AChains" must
	** be removed and freed then and the user must be notified about the leaving
	** of a APF_DODIR-AChain with APF_DIDDIR.
	*/
	   
        if ((ac->an_Child = Match_AllocAChain(1, DOSBase)))
	{
	    ac->an_Child->an_Parent = ac;
	    ac = ac->an_Child;
	    AP->ap_Current = ac;
	    
	    ac->an_String[0] = P_ANY;
	    ac->an_String[1] = 0;
	    ac->an_Flags = DDF_PatternBit | DDF_AllBit;	    
	}
	
	/*
	** If the allocation did not work, we simple ignore APF_DODIR. Just like if
	** the user did not set this flag. Good idea or bad idea?
	*/
    }

    
    /* Main loop for AChain traversing */
    
    for(;;)
    {
        BOOL must_go_back = FALSE;
	
	/* Check for user breaks (CTRL_C, ...) */
	 
	if (AP->ap_BreakBits)
	{
	    AP->ap_FoundBreak = CheckSignal(AP->ap_BreakBits);
	    if (AP->ap_FoundBreak)
	    {
	        error = ERROR_BREAK;
		goto done;
	    }
	}
	
	/* Check if AChain must be "setup" */
	
	if (!(ac->an_Flags & DDF_ExaminedBit))
	{
	    /*
	    ** This AChain must be "setup". First AChain->an_Lock must point
	    ** to the parent directory, that is the directory where this
	    ** AChain is "in". !
	    */
	   
	    if (ac->an_Parent)
	    {
		CurrentDir(ac->an_Parent->an_Lock);
		ac->an_Lock = Lock(ac->an_Parent->an_Info.fib_FileName, SHARED_LOCK);

		if (!ac->an_Lock)
		{
		    error = IoErr();
		    goto done;
		}

	    }
#if !MATCHFUNCS_NO_DUPLOCK
	    else
	    {
	        ac->an_Lock = DupLock(origdir);
		if (!ac->an_Lock)
		{
		    error = IoErr();
		    goto done;
		}		
	    }
#else
	    /*
	    ** If there was no ac->an_Parent then we are dealing with the
	    ** first AChain whose lock was already setup in BuildAChainList
	    */
#endif
	    CurrentDir(ac->an_Lock);
	    
	    if (ac->an_Flags & DDF_PatternBit)
	    {
	        /*
		** If this is a pattern AChain we first Examine here our
		** parent directory, so that it then can be traversed with
		** ExNext
		*/
		  
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
		
		/*
		** This is a normal AChain (no pattern). Try to lock it
		** to see if it exists.
		*/
		   
		if (!(lock = Lock(ac->an_String, SHARED_LOCK)))
		{
		    /* It does not exist, so if possible go back one step */
		    
		    if (ac->an_Parent)
		    {
		        /* [2] */
			
		        must_go_back = TRUE;
		    }
		    else
		    {
		        /* if going back is not possible get error code and exit */
		        error = IoErr();
		        goto done;
		    }
		    
		} /* if (!(lock = Lock(ac->an_String, SHARED_LOCK))) */
		else
		{
		    /* The File/Direcory ac->an_String exists */
		    
		    success = Examine(lock, &ac->an_Info);
		    UnLock(lock);

		    if (!success)
		    {
		        /*
			** Examine()ing the file/directory did not
			** work, although the lock was successful!?.
			** Get error code and exit
			*/
			   
			error = IoErr();
			goto done;
		    }

		    /*
		    ** This strcpy is necessary, because in case
		    ** of empty ac->an_String("") fib_FileName would
		    ** get parent directory name which it must not!
		    */
		       
		    strcpy(ac->an_Info.fib_FileName, ac->an_String);
		    
		    ac->an_Flags |= DDF_ExaminedBit;

		    /*
		    ** If this is a file, but there are still more path components to
		    ** follow then we have to go back one step (AChain)
		    */
		    
		    if (ac->an_Child && (ac->an_Info.fib_DirEntryType < 0))
		    {
		        /* [1] */
			
			must_go_back = TRUE;
		    }
		
		    /*
		    ** Here we either have found a matching file/directory (result)
		    ** or, if ac->an_Child != NULL we have still to continue walking
		    ** through the AChains until we are in the last one. This all
		    ** happens further below 
		    */
		    
		} /* if (!(lock = Lock(ac->an_String, SHARED_LOCK))) else ... */
		
	    } /* if (ac->an_Flags & DDF_PatternBit) else ... */ 

 	} /* if (!(ac->an_Flags & DDF_ExaminedBit)) */
	else
	{
	    /*
	    ** This AChain was already setup.
	    **
	    ** When an AChain which is *not* a pattern already had DDF_PatternBit
	    ** set, then this means ERROR_NO_MORE_ENTRIES, so we try to go back
	    ** one step
	    */
	       
	    if (!(ac->an_Flags & DDF_PatternBit))
	    {
	        /* [4] */
		
		must_go_back = TRUE;
	    }
	}
	
	/*
	** Here we can be sure that the actual AChain is setup, ie: it will
	** have ac->an_Lock set correctly and to indicate this DDF_ExaminedBit
	** was set
	*/
	   
	CurrentDir(ac->an_Lock);
	
	if (ac->an_Flags & DDF_PatternBit)
	{
	    if(ExNext(ac->an_Lock, &ac->an_Info))
	    {
	        if (MatchPatternNoCase(ac->an_String, ac->an_Info.fib_FileName))
		{
		    /*
		    ** This file matches the pattern in ac->an_String. If there
		    ** are no more AChains to follow then we have found a matching
		    ** file/directory (a result)  --> break.
		    */	
		    	       
		    if (!ac->an_Child) break;

		} else {
		    /* Did not match. Go to top of "for(;;)" loop */
		    continue;
		}
	    }
	    else
	    {
	        error = IoErr();
		if (error != ERROR_NO_MORE_ENTRIES) goto done;
		
		/* [3] */
		
		must_go_back = TRUE;
	    }
	    
	} /* if (ac->an_Flags & DDF_PatternBit) */
	
	/*
	** Handle the cases where we must (try to) go back to the previous AChain.
	** This can happen if the actual AChain turned out to be a file although
	** there are still more AChains to follow [1]. Or if the actual AChain did not
	** exist at all [2]. Or if in a pattern AChain ExNext() told us that there are
	** no more entries [3]. Or if we were getting to a normal (no pattern) AChain
	** which was already setup (DDF_ExaminedBit) [4].
	*/
	
	if (must_go_back)
	{
	    /* Check if going back is possible at all */
	    
	    if (!ac->an_Parent)
	    {
	        error = ERROR_NO_MORE_ENTRIES;
		goto done;
	    }
	    
	    /* Yep. It is possible. So let's cleanup the AChain. */
	    
	    CurrentDir(ac->an_Parent->an_Lock);
	    
	    UnLock(ac->an_Lock);
	    
	    ac->an_Lock = BNULL;
	    ac->an_Flags &= ~DDF_ExaminedBit;
	    
	    /* Make ac and AP->ap_Current point to the previous AChain */
	    
	    AP->ap_Current = ac->an_Parent;
	    
	    /*
	    ** If this was an APF_DODIR Achain (indicated by DDF_AllBit)
	    ** then the AChain must be unlinked and freed. And the user
	    ** must be informed about the leaving with APF_DIDDIR and
	    ** a "result" in AnchorPath which points to the directory which
	    ** was leaved.
	    */
	    
	    if (ac->an_Flags & DDF_AllBit)
	    {
	        AP->ap_Current->an_Child = NULL;
		Match_FreeAChain(ac, DOSBase);
		AP->ap_Flags |= APF_DIDDIR;
		
		/* go out of for(;;) loop --> MakeResult */
		
		break;
	    }
	    
	    ac = AP->ap_Current;
	    
	} /* if (must_go_back) */
	else
	{
	    if (!ac->an_Child)
	    {
	        /*
		** We have reached the last AChain. And this means that
		** we have found a matching file/directory :-)). Go out of
		** for(;;) loop --> MakeResult 
		*/
		
	        break;
	    }
	    
	    ac = ac->an_Child;
	    AP->ap_Current = ac;
	}

    } /* for(;;) */

    error = Match_MakeResult(AP, DOSBase);
    
done:
    CurrentDir(origdir);
    
    AP->ap_Flags &= ~APF_DODIR;
    
    if (old_current_lock != AP->ap_Current->an_Lock)
    {
        AP->ap_Flags |= APF_DirChanged;
    }
    else
    {
        AP->ap_Flags &= ~APF_DirChanged;
    }

    SetIoErr(error);
    
    return error;
	
}

/****************************************************************************************/

void My_MatchEnd(struct AnchorPath *AP)
{
    struct AChain *ac = AP->ap_Base, *acnext;

    if (ac)
    {

#if MATCHFUNCS_NO_DUPLOCK
        /*
	** CurrentDir to a valid lock, ie. one that will not be
	** killed further below
	*/
	
        CurrentDir(ac->an_Lock);
#endif	
	while(ac)
	{
	    acnext = ac->an_Child;

	    /*
	    ** Dont unlock lock in first AChain because it is the same
	    ** as the current directory when MatchFirst was called. And
	    ** this lock was not DupLock()ed!!!
	    */
	    
	    if (ac->an_Lock
#if MATCHFUNCS_NO_DUPLOCK
	        && (ac != AP->ap_Base)
#endif
	        )
	    {
	        UnLock(ac->an_Lock);
	    }
	    
	    Match_FreeAChain(ac, DOSBase);

	    ac = acnext;
	}
    }
    
    AP->ap_Current = NULL;
    AP->ap_Base = NULL;
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
static IPTR args[NUM_ARGS];

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
    struct AnchorPath stackap[2], *AP;
    LONG error = 0;

    AP = (struct AnchorPath *)((((IPTR)stackap) + 3) & ~3);

    memset(AP, 0, sizeof(struct AnchorPath));

    error =  My_MatchFirst(pattern, AP);

    if (error != 0)
    {
	printf("MatchFirst: error = %d\n", (int)error);
    }
    else
    {
        printf("direntrytype = %d\n", (int)AP->ap_Info.fib_DirEntryType);
        if (!(AP->ap_Flags & APF_ITSWILD) &&
	     (AP->ap_Info.fib_DirEntryType > 0))
	{
	     /* pattern was an explicitely named directory */
	     AP->ap_Flags |= APF_DODIR;
	}
	
	printf("ap_Flags = %x\n", AP->ap_Flags);
	NameFromLock(AP->ap_Current->an_Lock, s, 300);
	printf("BaseLock = \"%s\"\n", s);

	showaclist(AP->ap_Base);

	while(error == 0)
	{
	    if (AP->ap_Flags & APF_DIDDIR)
	    {
	        printf("DIDDIR: ");
	    } else {
	        if (all && (AP->ap_Info.fib_DirEntryType > 0))
		{
		    AP->ap_Flags |= APF_DODIR;
		    printf("DOING DIR: ");
		}
	    }
	    printf("fib_FileName = \"%s\"\n", AP->ap_Info.fib_FileName);

	    error = My_MatchNext(AP);
	}

    }

    My_MatchEnd(AP);
    
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
