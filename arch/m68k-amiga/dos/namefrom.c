/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Retrieve the full pathname from a lock or a filehandle.
    Lang: english
*/

#define DEBUG 0
#include <proto/exec.h>
#include <dos/filesystem.h>
#include "dos_intern.h"
#include <proto/dos.h>
#include <aros/debug.h>

BOOL namefrom_internal(struct DosLibrary *DOSBase, BPTR lock, STRPTR buffer, LONG length, BOOL filehandle)
{
    STRPTR  	    	 s1, s2, name, origbuffer;
    BPTR parentlock, origlock;
    struct FileInfoBlock *fib;
    LONG    	    	 error, code;
    BOOL first = TRUE;

    origbuffer = buffer;
    D(bug("NameFromX(%x,%x,%d,%d)\n", BADDR(lock), buffer, length, filehandle));
	
    if (length < 1)
    {
        SetIoErr(ERROR_LINE_TOO_LONG);
        return DOSFALSE;
    }
    
    if (filehandle) {
    	lock = dopacket1(DOSBase, NULL, ((struct FileHandle*)BADDR(lock))->fh_Type, ACTION_COPY_DIR_FH,
    		((struct FileHandle*)BADDR(lock))->fh_Arg1);
    	if (!lock)
    	    return DOSFALSE;
    }

    fib = AllocDosObject(DOS_FIB, 0);
    if (!fib)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }
    
    if (!lock)
    	lock = DOSBase->dl_SYSLock;

    /* Construct the name from top to bottom */
    name = buffer + length;
    *--name = 0;
    
    origlock = lock;
    parentlock = BNULL;

    /* Loop over path */
    do
    {
    	STRPTR sep = NULL;

	error = dopacket2(DOSBase, NULL, ((struct FileLock*)BADDR(lock))->fl_Task, ACTION_EXAMINE_OBJECT, lock, MKBADDR(fib)) == 0;
	//bug("name='%s'\n", fib->fib_FileName);
	if (!error) {
	    parentlock = dopacket1(DOSBase, &code, ((struct FileLock*)BADDR(lock))->fl_Task, ACTION_PARENT, lock);
	    if (!parentlock && !first)
	    	error = code;
	    //bug("parentlock=%x\n", parentlock);
	}
	if (lock != origlock && lock)
		UnLock(lock);
	lock = parentlock;
   	
    	/* Move name to the top of the buffer. */
    	if(!error)
    	{
    	    fixfib(fib);
    	    s1 = s2 = fib->fib_FileName;
    
    	    while(*s2++)
            {
                ;
            }
    		
    	    if(!parentlock)
    	    {
                if (name > buffer)
                {
                    *--name=':';
                }
                else
                {
                    error = ERROR_LINE_TOO_LONG;
                }
    	    }
    	    else if(!first)
    	    {
                if (name > buffer)
                {		    
                    *--name = '/';
                }
                else
                {
                    error = ERROR_LINE_TOO_LONG;
                }
    	    }

    	    /* Stash away the location of the directory separator */
    	    sep = name;
    	    
    	    if (!error)
    	    {
        		s2--;
        
        		if (name - (s2 - s1) >= buffer)
        		{ 
        	    	    while(s2 > s1)
                        {
                            *--name = *--s2;
                        }
        		}
        		else
        		{
        	    	    error = ERROR_LINE_TOO_LONG;
        		}
    	    }
    	    
    	} /* if(!error) */
    	first = FALSE;

    }
    while(!error && parentlock);

    /* Move the name from the top to the bottom of the buffer. */
    if (!error)
    {
        UBYTE c, old_c = '\0';
	
    	do
    	{
    	    c = *name++;

    	    if ((c != '/') || (old_c != ':'))
    	    {
    	        *buffer++ = c;
    	    }
    	    
    	    old_c = c;
    	    
    	}
        while (c);
    }

    if (filehandle)
    	UnLock(origlock);

    FreeDosObject(DOS_FIB, fib);

    /* All done. */

    SetIoErr(error);

    D(bug("[NameFromX]='%s':%d\n", origbuffer, error));

    return error
        ? DOSFALSE
        : DOSTRUE;
    
}
