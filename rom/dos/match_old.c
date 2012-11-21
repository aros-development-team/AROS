matchfirst.c:



/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

        AROS_LH2(LONG, MatchFirst,

/*  SYNOPSIS */
        AROS_LHA(STRPTR             , pat, D1),
        AROS_LHA(struct AnchorPath *, AP , D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 137, Dos)

/*  FUNCTION

        Searches for the first file or directory that matches a given pattern.
        MatchFirst() initializes the AnchorPath structure for you but you
        must initilize the following fields: ap_Flags, ap_Strlen, ap_BreakBits
        and ap_FoundBreak. The first call to MatchFirst() also passes you
        the first matching file which you can examine in ap_Info and the directory
        the files is in in ap_Current->an_Lock. After the first call to
        MatchFirst() call MatchNext().
        The search begins whereever the current directory is set to. See
        CurrentDir();
        For more info on patterns see ParsePattern().

    INPUTS
        pat  - pattern to search for
        AP   - pointer to (initilized) AnchorPath structure
        
    RESULT
        0     = success
        other = DOS error code

    NOTES

    EXAMPLE

    BUGS
        Copying of the relative path to ap_Buf is not implemented yet

    SEE ALSO
        MatchNext(), MatchEnd(), ParsePattern(), Examine(), CurrentDir()
        <dos/dosasl.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct AChain * AC;
    struct AChain * AC_Prev = NULL;
    LONG PatLength;
    STRPTR ParsedPattern;
    BPTR firstlock; 

    bug("matchfirst\n");
    
    if (!pat)
        return FALSE;

    AP->ap_Base = NULL;
    AP->ap_Current = NULL;

    PatLength = 2*strlen(pat)+2;
    ParsedPattern = AllocMem(PatLength, MEMF_ANY);


    if (NULL != ParsedPattern)
    {
        LONG PatStart = 0;
        LONG PatEnd   = 0;
        BOOL AllDone  = FALSE;
        LONG index;
        LONG success = FALSE;
        BPTR origdir;
        
        bug("matchfirst: ParsedPattern mem okay. Calling ParsePatternNoCase\n");

        /* 
        ** Put the preparsed string to some memory 
        ** If there are any wildcards then leave info 
        */
        if (1 == ParsePatternNoCase(pat, ParsedPattern, PatLength))
          AP->ap_Flags |= (BYTE)APF_ITSWILD;

        //bug("matchfirst: ParsePatternNoCase returned: pattern = \"%s\"\n", ParsedPattern);

        /*
        ** First I search for the very first ':'. If a '/' comes along
        ** before that I quit. The string before and including the ':' 
        ** is assumed to be an assigned directory, for example 'libs:'.
        ** So I will start looking for the pattern in that directory.
        */
        while (TRUE)
        {
            if (ParsedPattern[PatEnd] == ':')
            {
                success = TRUE;
                break;
            }
            else
            {
                if ( ParsedPattern[PatEnd]         == '/'  ||
                     ParsedPattern[PatEnd]         == '\0' ||
                    (ParsedPattern[PatEnd] & 0x80) != 0 
                                  /* a token or nonprintable letter */)
                {
                    PatEnd = 0;
                    break;
                }
            } 
            PatEnd++;
        }

        /* 
        ** Only if success == TRUE an assigned dir was found. 
        */
        if (TRUE == success)
        {
            /* 
            ** try to create a lock to that assigned dir. 
            */
            char Remember = ParsedPattern[PatEnd+1];
            PatEnd++;
            ParsedPattern[PatEnd] = '\0';
            firstlock = Lock(ParsedPattern, ACCESS_READ);
            origdir = CurrentDir(firstlock);

            /* 
            ** check whether an error occurred 
            */
            if (NULL == firstlock)
            {
                FreeMem(ParsedPattern, PatLength);
                return IoErr(); // ERROR_DIR_NOT_FOUND; /* !!! hope that's the right error code... */
            }

            /* 
            ** I have the correct lock. 
            */
            ParsedPattern[PatEnd] = Remember;
            PatStart=PatEnd;
            
        } /* if (TRUE == success) */
        else
        {
            /* 
            ** Create a lock to the current dir. 
            */
            origdir   = CurrentDir(NULL);
            firstlock = DupLock(origdir);
            if (!firstlock)
            {
                FreeMem(ParsedPattern, PatLength);
                return IoErr();
            }
            
            (void)CurrentDir(firstlock);
        }

        bug("MatchFirst: origdir = %x\n", origdir);
        bug("MatchFirst: firstlock = %x\n", firstlock);
        
        /*
        ** Allocate an AChain structure for the original directory.
        */
        AC = (struct AChain *)AllocVec(sizeof(struct AChain), MEMF_CLEAR);
        if (NULL == AC)
        {
            /*
            ** No more memory
            */
            FreeMem(ParsedPattern, PatLength);
            UnLock(firstlock);
            CurrentDir(origdir);
            
            return ERROR_NO_FREE_STORE;
        }

        AC->an_Lock  = origdir;
        AC->an_Flags = DDF_Completed|DDF_Single;
        AC_Prev     = AC;


        AP->ap_Base = AC;

        /* 
        ** Build the Anchor Chain. For every subdirectory I allocate
        ** an AChain structure and link them all together 
        */   
        while (FALSE == AllDone)
        {
            /* 
            ** Search for the next '/' in the pattern and everything behind
            ** the previous '/' and before this '/' will go to an_String 
            */
            while (TRUE)
            {
                if (ParsedPattern[PatEnd] == '\0')
                {
                    AllDone = TRUE;
                    PatEnd--;
                    break;
                }
                if (ParsedPattern[PatEnd] == '/')
                {
                    PatEnd--;
                    break;
                }
                PatEnd++;
                
            } /* while(TRUE) */

            AC = AllocVec(sizeof(struct AChain)+(PatEnd-PatStart+2), MEMF_CLEAR);
            if (NULL == AC)
            {
                /* not so bad if this was not the very first AC. */
                if (NULL == AP->ap_Base)
                {
                    /*
                    ** oops, it was the very first one. I really cannot do anything for 
                    ** you. - sorry 
                    */
                    FreeMem(ParsedPattern, PatLength);

                    UnLock(AP->ap_Base->an_Lock);
                    CurrentDir(origdir); /* stegerg */
                    FreeMem(AP->ap_Base, sizeof(struct AChain));

                    return ERROR_NO_FREE_STORE;
                }

                /* 
                ** let me out of here. I will at least try to do something for you.
                ** I can check the first few subdirs but that's gonna be it. 
                */
                AP->ap_Flags |= APF_NOMEMERR;
                break;
                
            } /* if (NULL == AC) */

            if (NULL == AP->ap_Base)
                AP->ap_Base = AC;

            if (NULL == AP->ap_Current)
                AP->ap_Current = AC;


            if (NULL != AC_Prev)
                AC_Prev->an_Child = AC;

            AC->an_Parent = AC_Prev;
            AC_Prev       = AC;

            /* 
            ** copy the part of the pattern to the end of the AChain. 
            */
            index = 0;
            while (PatStart <= PatEnd)
            {
                AC->an_String[index] = ParsedPattern[PatStart];
                index++;
                PatStart++;
            }

            /* 
            ** Put PatStart and PetEnd behind the '/' that was found. 
            */
            PatStart   = PatEnd + 2;
            PatEnd    += 2;

            /* 
            ** the trailing '\0' is there automatically as I allocated enough store
            ** with MEMF_CLEAR
            */

        } /* while (FALSE == AllDone) */

        /*
        ** Free the pattern since it has been distributed now
        */
        FreeMem(ParsedPattern, PatLength);

        /* 
        ** The AnchorChain to work with is the second one. 
        */
        AP->ap_Base = AP->ap_Base->an_Child;
        AC          = AP->ap_Base;

        AC->an_Lock = firstlock;

        (void)Examine(AC->an_Lock, &AC->an_Info);

        return followpattern(AP, AC, DOSBase);

    } /* if (NULL != ParsedPattern) */
    else
    {
        return ERROR_NO_FREE_STORE;
    }  

    return 0;

    AROS_LIBFUNC_EXIT
    
} /* MatchFirst */






matchnext.c:







/*
    Copyright © 1995-96, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include "dos_intern.h"

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
        Copying of the relative path to ap_Buf is not implemented yet


    SEE ALSO
        MatchFirst() MatchEnd() CurrentDir() Examine() ExNext()
        ParsePattern() <dos/dosasl.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* 
    ** If the user says I am supposed to enter the directory then I first check
    ** whether it is a directory... 
    */

    struct AChain * AC = AP->ap_Current;
    BOOL success;
    struct Task * task = FindTask(NULL);

    bug("matchnext\n");
    
    AP->ap_BreakBits &= (SIGBREAKF_CTRL_C|
                         SIGBREAKF_CTRL_D|
                         SIGBREAKF_CTRL_E|
                         SIGBREAKF_CTRL_F);

    if (0 != (AP->ap_Flags & APF_DODIR ))
    {
        bug("matchnext: APF_DODIR is set\n");
        if (AC->an_Info.fib_DirEntryType >= 0 /* &&
            AC->an_Info.fib_DirEntryType != ST_SOFTLINK */)
        { 
            bug("matchnext: APF_DODIR is set. Is a directory.\n");

            /* Ok, it seems to be a directory so I will enter it. */
            /* See whether there's a AnchorChain for that dir... */
            if (NULL == AC->an_Child)
            {
                bug("matchnext: APF_DODIR is set. Is a directory. Has no child. Creating temp AChain\n");
                AC->an_Child = (struct AChain *)
                    AllocVec(sizeof(struct AChain)+1, MEMF_CLEAR);

                if (AC->an_Child)
                {
                    AC->an_Child->an_Parent = AC;

                    AC->an_Child->an_String[0] = P_ANY;
                    AC->an_Child->an_Flags = DDF_PatternBit;
                }
                
                bug("matchnext: Created temporary AChain structure: %x!\n", AC->an_Child);
            }

            if (NULL != AC->an_Child)
            {
                BPTR newdir;

                bug("matchnext: APF_DODIR is set. Is a directory. Has child.\n");

                /* Ok, we're all set. */
                /* Lock the director by it's name. */
                AP->ap_Current = AC->an_Child;
 
                bug("matchnext: APF_DODIR is set. Is a directory. Has child. Locking \"%s\"\n", AC->an_Info.fib_FileName);

                newdir = Lock(AC->an_Info.fib_FileName, ACCESS_READ);
                bug("matchnext: APF_DODIR is set. Is a directory. Has child. Lock = %x\n", newdir);

                if (!newdir) /* stegerg */
                {
                    AC = AC->an_Child;
                    return IoErr();
                } /* end stegerg */
                
                //kprintf("CurrentDir()ing %x\n",AC->an_Info.fib_FileName);
                (void)CurrentDir(newdir);
                bug("matchnext: APF_DODIR is set. Is a directory. Has child. CurrentDir()ed to lock\n");

                AC = AC->an_Child;
                AC->an_Lock = newdir;
                bug("matchnext: APF_DODIR is set. Is a directory. Has child. Calling Examine\n");
                Examine(AC->an_Lock, &AC->an_Info);
                bug("matchnext: APF_DODIR is set. Is a directory. Has child. Called Examine\n");
            }
            else
            {
                bug("matchnext: APF_DODIR is set. Could not alloc temp AChain. Returnin ERROR_NO_FREE_STORE\n");
                return ERROR_NO_FREE_STORE;
            }
            
        } /* if (AC->an_Info.fib_DirEntryType >= 0 ... */
        
    } /* if (0 != (AP->ap_Flags & APF_DODIR )) */

    AP->ap_Flags &= ~(BYTE)(APF_DODIR|APF_DIDDIR);

    bug("matchnext 2\n");

    /* 
    ** AC points to the current AnchorChain 
    */
    while (TRUE)
    {
        do
        {
            ULONG breakbits;
            /*
            ** Check for a break signal CTRL C/D/E/F
            */
            breakbits = (AP->ap_BreakBits & SetSignal(0, 0)); /* task->tc_SigRecvd */

            if (0 != breakbits)
            {
                /*
                ** Finish right here... there might be a problem when/if the
                ** algorithm is resumed the next time... Gotta test that.
                */
                AP->ap_FoundBreak = breakbits;
                bug("matchnext 2: break bits were set. Returning ERROR_BREAK\n");
                
                return ERROR_BREAK;
            }

            success = ExNext (AC->an_Lock, &AC->an_Info);
        }
        
        while (DOSTRUE == success &&
               DOSFALSE == MatchPatternNoCase(AC->an_String,
                                              AC->an_Info.fib_FileName));


        if (DOSFALSE == success)
        {
            bug("matchnext 2: success == DOSFALSE (no matching file)\n");
            /* 
            ** No more entries in this dir that match. So I might have to
            ** step back one directory. Unlock the current dir first.
            */

      //kprintf("Couldn't find a matching file.!\n");

            if (AP->ap_Base == AC)
            {
                bug("matchnext 2: success == DOSFALSE (no matching file). Unlocking %x\n", AC->an_Lock);
                UnLock(AC->an_Lock);
                AP->ap_Current = AC->an_Parent;
                bug("matchnext 2: success == DOSFALSE (no matching file). AP->ap_Current = %x\n", AP->ap_Current);
                bug("matchnext 2: currentdiring to %x\n", AP->ap_Current->an_Lock);
                CurrentDir(AP->ap_Current->an_Lock); /* stegerg */
                bug("matchnext 2: Cannot step back dir. Returning ERROR_NO_MORE_ENTRIES\n");
                return ERROR_NO_MORE_ENTRIES;
            }

            /* 
            ** Are there any previous directories??? 
            */
            if (NULL != AC && NULL != AC->an_Parent)
            {
                LONG retval = 0;

                bug("matchnext 2: success == DOSFALSE. There is a Parent and AC is *not* NULL. Unlocking %x\n", AC->an_Lock);

                UnLock(AC->an_Lock);
                AC->an_Lock = NULL;

                AC             = AC->an_Parent;
                AP->ap_Current = AC;

                bug("matchnext 2: success == DOSFALSE. There is a Parent and AC is *not* NULL. CurrentDir()ing %x\n", AC->an_Lock);

                CurrentDir(AC->an_Lock);
                
                if (AC->an_Child->an_Flags & DDF_PatternBit)
                {
                    FreeVec(AC->an_Child);
                    AC->an_Child = NULL;
                }
                else
                if (0 == (AC->an_Flags & DDF_PatternBit))
                {
                    /*
                    ** In this case I must silently follow the pattern again...
                    */
                    bug("matchnext 2: success == DOSFALSE. DDF_PatternBit is *not* set. Leaving matchnext with result from followpattern()\n");
                    return followpattern(AP, AC, DOSBase);
                }

                AP->ap_Flags |= APF_DIDDIR;
                /* 
                ** I show this dir again as I come back from searching it 
                */

                retval = createresult(AP, AC, DOSBase);

                /* 
                ** Step back to this directory and go on searching here 
                */

                CurrentDir(AC->an_Lock);

                if (NULL == AC->an_Parent)
                  retval = ERROR_NO_MORE_ENTRIES;

                bug("matchnext 2: returning retval\n", retval);

                return retval;
              
            } /* if (NULL != AC->an_Parent && NULL != AC) */

            bug("matchnext 2: success == DOSFALSE. There is no Parent and/or AC is NULL. Returning ERROR_NO_MORE_ENTRIES\n");

            /* 
            ** No previous directory, so I am done here... 
            */
            return ERROR_NO_MORE_ENTRIES;
            
        } /* if (DOSFALSE == success) */
        else
        {
            bug("matchnext 2: success == DOSTRUE (found a match). Leaving matchnext wth result from createresult()\n");
            /* Alright, I found a match... */
            return createresult(AP, AC, DOSBase);
        }
      
    } /* while (TRUE) */

    bug("matchnext 2: returning 0.\n");
    
    return 0;

    AROS_LIBFUNC_EXIT
    
} /* MatchNext */





matchend.c:






/*
    Copyright © 1995-96, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

        AROS_LH1(void, MatchEnd,

/*  SYNOPSIS */
        AROS_LHA(struct AnchorPath *, AP, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 139, Dos)

/*  FUNCTION
        Free the memory that was allocated by calls to MatchFirst() and
        MatchNext()

    INPUTS
        AP  - pointer to Anchor Path structure which had been passed to
              MatchFirst() before.

    RESULT
        Allocated memory is returned and filelocks are freed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Free the AChain and unlock all locks that are still there */
    struct AChain * AC = AP->ap_Current;
    struct AChain * AC_tmp;

    /* Unlock everything */
    if (NULL == AC)
        return;

    while (NULL != AC->an_Parent)
    {
        bug("MatchEnd: unlocking %x\n", AC->an_Lock);
        UnLock(AC->an_Lock);
        AC = AC->an_Parent;
    }

    bug("MatchEnd: CurrentDir(%x)\n", AC->an_Lock);
    
    CurrentDir(AC->an_Lock);

    /* AC points to the very first AChain obj. in the list */
    /* Free the AChain List */
    while (NULL != AC)
    {
        AC_tmp = AC->an_Child;
        FreeVec(AC);
        AC = AC_tmp;
    }

    /* Cleanup AP */
    AP->ap_Base = NULL;
    AP->ap_Current = NULL;

    AROS_LIBFUNC_EXIT
    
} /* MatchEnd */









match_misc.c:

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosasl.h>
#include "dos_intern.h"
#include <string.h>

LONG followpattern(struct AnchorPath * AP,
                   struct AChain * AC,
                   struct DosLibrary * DOSBase)
{
    LONG success;
  
    bug("followpattern start\n");
    
    /* 
    ** If the user specified the pattern libs:#?/s#/
    ** then I must enter that pattern before I return to the user.
    */
    while (TRUE)
    {
        do
        {
            success = ExNext (AC->an_Lock, &AC->an_Info);
        }
        while (DOSTRUE == success &&
               DOSFALSE == MatchPatternNoCase(AC->an_String,
                                              AC->an_Info.fib_FileName));

        /*
        ** If no matching entry could be found here then I
        ** step back one directory, unless this is the very
        ** first directory already.
        */
        if (DOSFALSE == success)
        {
            bug("followpattern: success is DOSFALSE. Unlocking AC->an_Lock %x\n", AC->an_Lock);
            UnLock(AC->an_Lock);
            AC->an_Lock = NULL;

            AC = AC->an_Parent;
            bug("followpattern: success is DOSFALSE. AC now %x\n", AC);

            AP->ap_Current = AC;

            bug("followpattern: success is DOSFALSE. CurrentDir()ing to %x\n", AC->an_Lock);

            CurrentDir(AC->an_Lock);

            bug("followpattern: success is DOSFALSE. AC has parent = %s\n", AC->an_Parent ? "yes" : "no");

            if (NULL == AC->an_Parent) 
            {
                bug("followpattern: success is DOSFALSE. Has no parent. --> returning ERROR_NO_MORE_ENTRIES\n");
                return ERROR_NO_MORE_ENTRIES;
            }
        }
        else
        {
            bug("followpattern: success *not* DOSFALSE.\n");
            if (AC->an_Info.fib_DirEntryType >= 0 /* &&
                AC->an_Info.fib_DirEntryType != ST_SOFTLINK */)
            {
                bug("followpattern: success *not* DOSFALSE. Is a directory.\n");
                /*
                ** I must silently enter this directory if there
                ** are further patterns left, otherwise I return to the
                ** user. 
                */
                if (NULL != AC->an_Child)
                {
                    /*
                    ** Silently entering this dir according to the 
                    ** pattern.
                    */
                    BPTR newdir;

                    bug("followpattern: success *not* DOSFALSE. Is a directory. Has a child. Entering\n");

                    AP->ap_Current = AC->an_Child;

                    bug("followpattern: success *not* DOSFALSE. Is a directory. Locking + CurrentDir()ing to %s\n", AC->an_Info.fib_FileName);

                    newdir = Lock(AC->an_Info.fib_FileName, ACCESS_READ);
                    bug("followpattern: success *not* DOSFALSE. Is a directory. Locking done. Lock = %x\n", newdir);
                   (void)CurrentDir(newdir);

                    bug("followpattern: success *not* DOSFALSE. Is a directory. CurrentDir()ing done\n");

                    AC = AC->an_Child;
                    AC->an_Lock = newdir;

                    bug("followpattern: success *not* DOSFALSE. Is a directory. Examining lock %x\n", newdir);

                    Examine(AC->an_Lock, &AC->an_Info);
                    bug("followpattern: success *not* DOSFALSE. Is a directory. Examining lock done\n");
                }
                else
                {
                    bug("followpattern: success *not* DOSFALSE. Is a directory. Has now child. Leaving followpattern with result from createresult\n");
                    /*
                    ** Ask the user whether to enter this dir or not
                    */
                    return createresult(AP, AC, DOSBase);
                }
                
            } /* is a directory */
            else
            {
                /*
                ** This is a file. If there is pattern left to match then
                ** I must not show this file because I must fulfill the pattern
                ** first.
                */
                if (NULL == AC->an_Child)
                {
                    bug("followpattern: is a file and has no child: leaving followpattern with result from createresult \n");

                    /*
                    ** There's no pattern left!
                    */
                    return createresult(AP, AC, DOSBase);
                }
                else
                    bug("followpattern: Silently skipping file %s!\n",AC->an_Info.fib_FileName);

            } /* is a file */
          
        } /* if (DOSFALSE == success) */

    } /* while (TRUE) */

#if 0
    /* 
    ** Hooray! A matching file was found. Also show the data in AP 
    */
    createresult(AP, AC, DOSBase);
#endif      

}


/*
** A file/directory has been found and now it must be written
** into the relevant structures.
*/
LONG createresult(struct AnchorPath * AP,
                  struct AChain * AC,
                  struct DosLibrary * DOSBase)
{
    bug("createresult\n");
    
    CopyMem(&AC->an_Info, &AP->ap_Info, sizeof(struct FileInfoBlock));
    if (0 != AP->ap_Strlen)
    {
        if (FALSE == writeFullPath(AP))
        {
            bug("createresult: returning ERROR_BUFFER_OVERFLOW\n");
            return ERROR_BUFFER_OVERFLOW;
        }
    }

    bug("createresult: done\n");

    return 0;
}


/* Function needed by MatchFirst/Next */

BOOL writeFullPath(struct AnchorPath * AP)
{
    struct AChain * AC = AP->ap_Base;
    BOOL end = FALSE;
    char * LastPos = (char *)&AP->ap_Buf;
    int copied = 0;

    bug("writefullpath\n");
    while (FALSE == end)
    {
        int len = strlen(AC->an_Info.fib_FileName);
        if (copied+len > AP->ap_Strlen)
        {
            bug("writefullpath: not enough space\n");
            return FALSE;
        } 
        strcpy(&LastPos[copied], AC->an_Info.fib_FileName);
        copied += len;

        if (AC != AP->ap_Current)
        {
            /* also add a '/' */
            if (copied+1 > AP->ap_Strlen)
            {
                bug("writefullpath: not enough space 2\n");
                return FALSE;
            }
            LastPos[copied]='/';
            copied++;
        }
        else
        {
            if (copied+1 > AP->ap_Strlen)
            {
                bug("writefullpath: not enough space 3\n");
                return FALSE;
            }
            LastPos[copied]='\0';
            end = TRUE;
        }

        AC = AC->an_Child;
    }

    bug("writefullpath: done\n");

    return TRUE;
}
