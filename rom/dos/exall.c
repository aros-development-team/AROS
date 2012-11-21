/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Examine a directory.
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/exall.h>
#include <stddef.h>
#include "dos_intern.h"
#include <aros/asmcall.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH5(BOOL, ExAll,

/*  SYNOPSIS */
        AROS_LHA(BPTR,                  lock,    D1),
        AROS_LHA(struct ExAllData *,    buffer,  D2),
        AROS_LHA(LONG,                  size,    D3),
        AROS_LHA(LONG,                  data,    D4),
        AROS_LHA(struct ExAllControl *, control, D5),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 72, Dos)

/*  FUNCTION

    Examine an entire directory.

    INPUTS

    lock     --  lock on the directory to be examined
    buffer   --  buffer for the data that is returned (must be aligned)
                 which is filled with (partial) ExAllData structures
                 (see NOTES)
    size     --  size of 'buffer' in bytes
    data     --  type of the data to be returned
    control  --  a control structure allocated by AllocDosObject()

    RESULT

    An indicator of if ExAll() is done. If FALSE is returned, either ExAll()
    has completed in which case IoErr() is ERROR_NO_MORE_ENTRIES or an
    error occurred. If a non-zero value is returned ExAll() must be called
    again until it returns FALSE.

    NOTES
    
    The following information is essential information on the ExAllData
    structure:

    ed_Type:

    ED_NAME        --  filename
    ED_TYPE        --  type
    ED_SIZE        --  size in bytes
    ED_PROTECTION  --  protection bits
    ED_DATE        --  date information (3 longwords)
    ED_COMMENT     --  file comment (NULL if no comment exists)
    ED_OWNER       --  owner user and group id

    This is an incremental list meaning that if you specify ED_OWNER you
    will get ALL attributes!


    Filesystems that support ExAll() must support at least up to ED_COMMENT.
    If a filesystem doesn't support a particular type, ERROR_BAD_NUMBER must
    be returned.

    ed_Next : pointer to the next entry in the buffer. The last entry
               has a NULL value for ed_Next.


    The control structure have the following fields.

    eac_Entries : the number of entries in the buffer after a call to ExAll().
                  Make sure that your code handles the case when eac_Entries
                  is 0 and ExAll() returns TRUE.

    eac_LastKey : must be initialized to 0 before calling ExAll() for the
                  first time.

    eac_MatchString : if NULL then information on all files will be returned.
                      If non-NULL it's interpreted as a pointer to a string
                      used for pattern matching which files to return
                      information on. This string must have been parsed by
                      ParsePatternNoCase()!

    eac_MatchFunc : pointer to a hook that will be called to decide if an
                    entry should be included in the buffer. If NULL, no
                    matching function will be called. The hook is called as
                    follows

                        BOOL = MatchFunc(hook, data, typeptr)

    EXAMPLE

    BUGS

    SEE ALSO

    Examine(), ExNext(), MatchPatternNoCase(), ParsePatternNoCase(),
    AllocDosObject(), ExAllEnd()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileLock *fl = BADDR(lock);
    LONG status = 0;
    SIPTR err = 0;

    /* If fib != NULL it means we've already been called and found out that
       we needed to emulate ExAll, thus don't waste time sending messages to the
       handler.  */
    if (((struct InternalExAllControl *)control)->fib != NULL)
    {
        err = ERROR_ACTION_NOT_KNOWN;
    }
    else
    {
        status = dopacket5(DOSBase, &err, fl->fl_Task, ACTION_EXAMINE_ALL, (SIPTR)lock, (IPTR)buffer, (IPTR)size, (IPTR)data, (IPTR)control);
        if (status != DOSFALSE)
            err = RETURN_OK;
    }
    
    if
    (
        err == ERROR_NOT_IMPLEMENTED ||
        err == ERROR_ACTION_NOT_KNOWN
    )
    {
        /* Try to emulate it */
        STRPTR end  = (STRPTR)buffer + size;
        STRPTR next;

        struct ExAllData *last = buffer, *curr = buffer;
        
        struct InternalExAllControl *icontrol = (struct InternalExAllControl *)control;

        static const ULONG sizes[]=
        {
            0,
            offsetof(struct ExAllData,ed_Type),
            offsetof(struct ExAllData,ed_Size),
            offsetof(struct ExAllData,ed_Prot),
            offsetof(struct ExAllData,ed_Days),
            offsetof(struct ExAllData,ed_Comment),
            offsetof(struct ExAllData,ed_OwnerUID),
            sizeof(struct ExAllData)
        };

        /* Reset the 'fake' error */
        err = 0;
        
        /* Allocate the FIB structure, if not allocated yet. It will be deallocated
           by DeleteDosObject().  */
        if (!icontrol->fib)
        {
            icontrol->fib = AllocDosObject(DOS_FIB, NULL);
            if (!icontrol->fib)
            {
                err = IoErr();
                goto end;
            }
        }

        /* No entries found as of now yet.  */
        control->eac_Entries = 0;
        
        /* If LastKey == 0 it means this is the first time we're getting called,
           in which case we need to initialize the FIB structure and a few other things. 
           A "nice" side effect of this, is that if one wants to restart the scanning, 
           he/she just has to set LastKey to 0.  */
        if (control->eac_LastKey == 0)
        {    
            if (!Examine(lock, icontrol->fib))
            {
                err = IoErr();
                goto end;
            }
            if (icontrol->fib->fib_DirEntryType <= 0)
            {
                err = ERROR_OBJECT_WRONG_TYPE;
                goto end;
            }
        }
        
        /* Macro used when the data doesn't fit in the provided buffer.
           In case not even one element fit in the buffer, return a buffer
           overflow error, so that the user knows he/she has to increase the
           buffer.  */
        #define ReturnOverflow()                               \
        do {                                                   \
            if (last == curr)                                  \
                err = ERROR_BUFFER_OVERFLOW;                   \
                                                               \
            icontrol->fib->fib_DiskKey = control->eac_LastKey; \
            goto end;                                          \
        } while (0)

        /* Copy a string pointer by _source into the buffer provided
           to the ExAll function. This macro gracefully handles buffer
           overflows.  */
        #define CopyStringSafe(_source)     \
        do {                                \
            STRPTR source = _source;        \
                                            \
            for (;;)                        \
            {                               \
                if (next >= end)            \
                    ReturnOverflow();       \
                if (!(*next++ = *source++)) \
                   break;                   \
                }                           \
            } while (0)

            
        if (data > ED_OWNER)
            /* We don't have that many fields to fill in... */
            err = ERROR_BAD_NUMBER;
        else
        {
            for
            (   ;
                ExNext(lock, icontrol->fib); 
                /* Record the latest DiskKey into LastKey so that we can roll back to it
                   in case of a buffer overflow and when getting called again.  */         
                control->eac_LastKey = icontrol->fib->fib_DiskKey
            )
            {    
                /* Try to match the filename, if required.  */
                if (control->eac_MatchString &&
                    !MatchPatternNoCase(control->eac_MatchString,
                                        icontrol->fib->fib_FileName))
                    continue;

                next = (STRPTR)curr + sizes[data];

                /* Oops, the buffer is full.  */
                if (next > end)
                    ReturnOverflow();

                /* Switch over the requested fields and fill them as appropriate.  */
                switch(data)
                {
                    case ED_OWNER:
                        curr->ed_OwnerUID = icontrol->fib->fib_OwnerUID;
                        curr->ed_OwnerGID = icontrol->fib->fib_OwnerGID;

                        /* Fall through */
                    case ED_COMMENT:
                        curr->ed_Comment = next;
                        CopyStringSafe(icontrol->fib->fib_Comment);

                        /* Fall through */
                    case ED_DATE:
                        curr->ed_Days  = icontrol->fib->fib_Date.ds_Days;
                        curr->ed_Mins  = icontrol->fib->fib_Date.ds_Minute;
                        curr->ed_Ticks = icontrol->fib->fib_Date.ds_Tick;

                        /* Fall through */
                    case ED_PROTECTION:
                        curr->ed_Prot = icontrol->fib->fib_Protection;

                        /* Fall through */
                    case ED_SIZE:
                        curr->ed_Size = icontrol->fib->fib_Size;

                        /* Fall through */
                    case ED_TYPE:
                        curr->ed_Type = icontrol->fib->fib_DirEntryType;

                        /* Fall through */
                    case ED_NAME:
                        curr->ed_Name = next;
                        CopyStringSafe(icontrol->fib->fib_FileName);

                        /* Fall through */
                    case 0:
                        curr->ed_Next = (struct ExAllData *)(((IPTR)next + AROS_PTRALIGN - 1) & ~(AROS_PTRALIGN - 1));
                }
                    
                /* Do some more matching... */
                if (control->eac_MatchFunc && !CALLHOOKPKT(control->eac_MatchFunc, curr, &data))
                    continue;
                    
                /* Finally go to the next entry in the buffer.  */
                last = curr;
                curr = curr->ed_Next;
                control->eac_Entries++;
            }
            err = IoErr();
        }
end:
        /* This is the last one, after it there's nothing.  */
        last->ed_Next = NULL;
    }

    /* Set error code and return */
    SetIoErr(err);
    return (err == 0) ? DOSTRUE : DOSFALSE;
  
    AROS_LIBFUNC_EXIT
} /* ExAll */
