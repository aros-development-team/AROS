/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Combine paths.
    Lang: English
*/
#include "dos_intern.h"

#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(BOOL, AddPart,

/*  SYNOPSIS */
        AROS_LHA(STRPTR,       dirname, D1),
        AROS_LHA(CONST_STRPTR, filename, D2),
        AROS_LHA(ULONG,        size, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 147, Dos)

/*  FUNCTION
        AddPart() will add a file, directory or other path name to a
        directory path. It will take into account any pre-existing
        separator characters (':','/').

        If filename is an absolute path it will replace
        the current value of dirname.

    INPUTS
        dirname  - The path to add the new path to.
        filename - The path you wish added.
        size     - The size of the dirname buffer (must NOT be 0).

    RESULT
        Non-zero if everything succeeded, FALSE if the buffer would have
        overflowed.

        If the buffer would have overflowed, then dirname will not have
        been changed.

    NOTES

    EXAMPLE
        UBYTE buffer[80];
        buffer[0]='\0';
        AddPart(buffer, "Work:", 80);
        AddPart(buffer, "Programming/Include/exec", 80);

        FPuts(Output(), buffer);
        --> Work:Programming/Include/exec

        AddPart(buffer, "/graphics", 80);

        FPuts(Output(), buffer);
        --> Work:Programming/Include/graphics

        AddPart(buffer, "gfxmacros.h", 80);
        FPuts(Output(), buffer);
        --> Work:Programming/Include/graphics/gfxmacros.h

    BUGS

    SEE ALSO
        FilePart(), PathPart()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#if 1
    /* stegerg: a bit simple, but since the other code does not
                work correctly. And then even AmigaOS AddPart()
                does not seem to do things like eliminating
                "dead" path components like:
                
                    "test/test2//test3" --> "test/test3"
                    
                It just makes sure that the path is correct
    */

    char *stringpos;
    LONG stringlen;
    WORD mustaddslash = 0;
    
    stringpos = strchr(filename, ':');
    if (stringpos)
    {
        if (stringpos == (char *)filename)
        {
            /* The first char of filename is a ':' */
            
            /* Find dirname position to where we copy filename later */
            
            stringpos = strchr(dirname, ':');
            if (!stringpos) stringpos = dirname;
        }       
        else
        {
            /* Filename is a fully qualified path including
               volume/device name -> replace dirname with filename */
               
            stringpos = dirname;
        }
    }
    else
    {
        /* filename did not contain ':' */
        
        stringlen = strlen(dirname);
        stringpos = dirname + stringlen;
        
        /* Check if we must add a slash */
        
        if (stringlen > 0)
        {
            if ((stringpos[-1] != ':') && (stringpos[-1] != '/'))
                mustaddslash = 1;
        }
    }
    
    /* Check if dirname buffer is big enough */
    
    stringlen = ((LONG)(stringpos - (char *)dirname)) + strlen(filename) + 1 + mustaddslash;
    if (stringlen <= size)
    {
        if (mustaddslash) *stringpos++ = '/';
        strcpy(stringpos, filename);
        return DOSTRUE;
    }
    else
    {
        SetIoErr(ERROR_LINE_TOO_LONG);
        return DOSFALSE;
    }
    
#else
                
    /* stegerg: this is buggy, because in some places it accesses
                chars in dirname behind the (initial) string in
                there (after the 0 char). For example:
                
                char buffer[256];
                
                strcpy(buffer, "LOCALE:Catalogs");
                AddPart(buffer, "deutsch", 256);
                
                gives correct "LOCALE:Catalogs/deutsch"
                
           but: exactly the same, but buffer before was used for
                something else:
                
                char buffer[256];

                strcpy(buffer, "PROGDIR:Catalogs");
                AddPart(buffer, "deutsch", 256);
                
                gives correct "PROGDIR:Catalogs/deutsch"
                
                strcpy(buffer, "LOCALE:Catalogs");
                AddPart(buffer, "deutsch", 256);
                
                gives wrong "LOCALE:Catalogs//deutsch"
                                            ^^
                
    */
    
    LONG didx, fidx;
    BOOL gotfull = FALSE;

    /*
        Make sure the buffer wouldn't overflow, also finds the ends
        of the strings...
    */

    didx = fidx = 0;

    while(dirname[didx])    didx++;
    while(filename[fidx])
    {
        /*
            If this has a colon, and its not the first char,
            then this is probably a FQ path.
        */
        if((filename[fidx] == ':') && (fidx != 0))
            gotfull = TRUE;

        fidx++;
    }

    /* If we got a fully qualified path, then just do a copy. */
    if(gotfull == TRUE)
    {
        /* +1 for NULL byte */
        if( fidx + 1 > size ) {
            SetIoErr(ERROR_LINE_TOO_LONG);
            return DOSFALSE;

        while(*filename)
        {
            *dirname++ = *filename++;
        }
        *dirname = '\0';
        return DOSTRUE;
    }

    /* Otherwise correctly add the subpath on to the end */
    else
    {
        /* +1 for NULL byte, +1 for '/' */
        if((didx + fidx + 2) > size) {
            SetIoErr(ERROR_LINE_TOO_LONG);
            return DOSFALSE;
        }

        /*
            Add a '/' onto the end of the current path, unless of course
            the new path has a ':' or '/' on it already or current path
            is emtpy (stegerg) ...
        */
        if(    (*filename != '/')
            && (didx != 0 )
            && (dirname[didx - 1] != ':')
            && (dirname[didx - 1] != '/')
          )
        {
            dirname[didx++] = '/';
        }
        
        /*
            Handle leading '/'s 
        */
        while (*filename == '/')
        {
            filename ++; 
            while ((dirname[didx] != '/') && (dirname[didx] != ':') && didx)
                didx --;
            
            /*
                if we are at start of dirname buf then break even
                if there are more leading '/'s
            */
            if  (!didx)
                break;
            
            /* 
                Another leading '/' ?.
                Only move up a dir if we are not at the root
            */
            if ((*filename== '/') && (dirname[didx] != ':'))
                didx --;
                
        }
        /* If at root, don't overwrite the ':' */ 
        if (dirname[didx] == ':')
             didx ++;
        /* 
            if filename not only was a number of '/'s but also contained
            a subpath, then be sure to skip the found '/' of dirname.
        */
        else if ((dirname[didx] == '/') && *filename)
            didx ++;

        /* Now add the parts, making sure to do any backtracking... */
        while(*filename)
        {
            if(*filename == ':')
            {
                /*
                    Search back for a ':' or the start of the buffer
                    do the ':' test first, so we test for didx = 0 after...
                */
                while( (dirname[didx] != ':') && didx)
                {
                    didx--;
                }
                dirname[didx++] = *filename++;
            }
            else
                dirname[didx++] = *filename++;
        } /* while(*filename) */

        dirname[didx] = '\0';
    }
    return DOSTRUE;
#endif

    AROS_LIBFUNC_EXIT
} /* AddPart */
