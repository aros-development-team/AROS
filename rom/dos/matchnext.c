/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include "dos_intern.h"
#include <string.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

	AROS_LH1(LONG, MatchNext,

/*  SYNOPSIS */
	AROS_LHA(struct AnchorPath *, AP, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 138, Dos)

/*  FUNCTION
	Find next file or directory that matches a given pattern.
	See <dos/dosasl.h> for more docs and how to control MatchNext().


    INPUTS
	AP  - pointer to Anchor Path structure which had been passed to
              MatchFirst() before.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MatchFirst() MatchEnd() CurrentDir() Examine() ExNext()
	ParsePattern() <dos/dosasl.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct AChain 	*ac = AP->ap_Current;
    BPTR 		origdir, old_current_lock;
    LONG 		error = 0;
    BOOL    	    	dir_changed = FALSE;
    
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
	** Alloc a new AChain. Make it the active one. Set its string to
	** "#?" and mark it with DDF_AllBit Flag to indicate that this is a
	** "APF_DODIR-AChain".  This is important for "back steppings",
	** because "APF_DODIR-AChains" must be removed and freed then and
	** the user must be notified about the leaving of a APF_DODIR-AChain
	** with APF_DIDDIR.
	*/
	   
        if ((ac->an_Child = Match_AllocAChain(1, DOSBase)))
	{
	    ac->an_Child->an_Parent = ac;
	    ac = ac->an_Child;
	    AP->ap_Current = ac;
	    
	    ac->an_String[0] = P_ANY;
	    ac->an_String[1] = 0;
	    ac->an_Flags = DDF_PatternBit | DDF_AllBit;
	    
	    dir_changed = TRUE;	    
	}
	
	/*
	** If the allocation did not work, we simple ignore APF_DODIR. Just
	** like if the user did not set this flag. Good idea or bad idea?
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
	   
	    dir_changed = TRUE;
	    
	    if (ac->an_Parent)
	    {
		CurrentDir(ac->an_Parent->an_Lock);
		if (ac->an_Parent->an_Flags & DDF_PatternBit)
		{
		    ac->an_Lock = Lock(ac->an_Parent->an_Info.fib_FileName, SHARED_LOCK);
		}
		else
		{
		    ac->an_Lock = Lock(ac->an_Parent->an_String, SHARED_LOCK);
		}

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
	    ** first AChain whose lock was already setup in
	    ** Match_BuildAChainList
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
		    
		    if ((AP->ap_Flags & APF_ITSWILD) && (ac->an_Parent))
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

		    if (*ac->an_String == '\0')
		    {
		    	strcpy(ac->an_Info.fib_FileName, ac->an_String);
		    }
		    
		    ac->an_Flags |= DDF_ExaminedBit;

		    /*
		    ** If this is a file, but there are still more path
		    ** components to follow then we have to go back one step
		    ** (AChain)
		    */
		    
		    if (ac->an_Child && (ac->an_Info.fib_DirEntryType < 0))
		    {
		        /* [1] */
			
			must_go_back = TRUE;
		    }
		
		    /*
		    ** Here we either have found a matching file/directory
		    ** (result) or, if ac->an_Child != NULL we have still to
		    ** continue walking through the AChains until we are in
		    ** the last one. This all happens further below 
		    */
		    
		} /* if (!(lock = Lock(ac->an_String, SHARED_LOCK))) else ... */
		
	    } /* if (ac->an_Flags & DDF_PatternBit) else ... */ 

 	} /* if (!(ac->an_Flags & DDF_ExaminedBit)) */
	else
	{
	    /*
	    ** This AChain was already setup.
	    **
	    ** When an AChain which is *not* a pattern already had
	    ** DDF_PatternBit set, then this means ERROR_NO_MORE_ENTRIES, so
	    ** we try to go back one step
	    */
	       
	    if (!(ac->an_Flags & DDF_PatternBit))
	    {
	        /* [4] */
		
		must_go_back = TRUE;
	    }
	}
	
	/*
	** Here we can be sure that the actual AChain is setup, ie: it will
	** have ac->an_Lock set correctly and to indicate this
	** DDF_ExaminedBit was set
	*/
	   
	CurrentDir(ac->an_Lock);
	
	if (ac->an_Flags & DDF_PatternBit)
	{
	    if(ExNext(ac->an_Lock, &ac->an_Info))
	    {
	        if (MatchPatternNoCase(ac->an_String, ac->an_Info.fib_FileName))
		{
		    /*
		    ** This file matches the pattern in ac->an_String. If
		    ** there are no more AChains to follow then we have
		    ** found a matching file/directory (a result)  -->
		    ** break.
		    */	
			       
		    if (!ac->an_Child)
		    {		    	
		    	break;
		    }
    	    	    else
		    {
		    	if (ac->an_Info.fib_DirEntryType < 0)
			{
			    /* This is a file, no chance to follow child
			       AChain. Go to top of "for(;;)" loop */
			    continue;			    
			}
		    }
		    
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
	** Handle the cases where we must (try to) go back to the previous
	** AChain.  This can happen if the actual AChain turned out to be a
	** file although there are still more AChains to follow [1]. Or if
	** the actual AChain did not exist at all [2]. Or if in a pattern
	** AChain ExNext() told us that there are no more entries [3]. Or if
	** we were getting to a normal (no pattern) AChain which was already
	** setup (DDF_ExaminedBit) [4].
	*/
	
	if (must_go_back)
	{
	    /* Check if going back is possible at all */
	    
	    if (!ac->an_Parent)
	    {
	        error = ERROR_NO_MORE_ENTRIES;
		goto done;
	    }
	    
	    dir_changed = TRUE;
	    
	    /* Yep. It is possible. So let's cleanup the AChain. */
	    
	    CurrentDir(ac->an_Parent->an_Lock);
	    
	    UnLock(ac->an_Lock);
	    
	    ac->an_Lock = NULL;
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
	    
	    dir_changed = TRUE; /* CHECKME!!! Really? */
	}

    } /* for(;;) */

    error = Match_MakeResult(AP, DOSBase);
    
done:
    CurrentDir(origdir);
    
    AP->ap_Flags &= ~APF_DODIR;
    
    if (dir_changed)
    {
        AP->ap_Flags |= APF_DirChanged;
    }
    else
    {
        AP->ap_Flags &= ~APF_DirChanged;
    }

    SetIoErr(error);
    
    return error;
	
    AROS_LIBFUNC_EXIT
    
} /* MatchNext */

