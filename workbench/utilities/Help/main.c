/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/******************************************************************************

    NAME

        Help name (section)

    SYNOPSIS

        NAME, SECTION

    LOCATION

        SYS:Utilities

    FUNCTION

        Shows help information for commands and system applications.
        The help files are stored in HELP:English with a sub-directory
        for each section. The utility "Multiview" is used to show the
        guide files. If neither NAME nor SECTION are given an index page
        will be shown.

    INPUTS

        NAME    --  the name of the command or application whose help
                    you want to view. The name is case-insensitive.
	SECTION --  the section where the help document will be searched.
                    Currently available are "commands" and "system". If you
                    don't specify the section all available sections will
                    be searched. The section is case-insensitive.

    RESULT

    NOTES

    EXAMPLE

        Help dir
        Help dir commands
        Help help

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <proto/dos.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#define DEBUG 1
#include <aros/debug.h>

#define ARG_TEMPLATE "NAME,SECTION"

const char *ver = "$VER: Help 1.1 (09.03.2013)";

enum
{
    ARG_NAME,
    ARG_SECTION,
    ARG_COUNT
};

struct RDArgs *rda;
static STRPTR name;
static STRPTR section;
static TEXT docpath[100];
static TEXT viewpath[256];

static CONST_STRPTR section_arr[] = {"Commands", "System", NULL};


static void clean_exit(CONST_STRPTR s)
{
    if (s) PutStr(s);
    if (rda) FreeArgs(rda);
    exit(0);
}


static LONG show_file(STRPTR buffer, ULONG buflen, CONST_STRPTR path)
{
    strlcpy(buffer, "SYS:Utilities/Multiview ", buflen);
    strlcat(buffer, path, buflen);
    D(bug("[Help/show_file] SystemTags path %s\n", buffer));
    // FIXME: running asynchron?
    LONG error = SystemTags
    (
        buffer,
        TAG_DONE
    );
    return error;
}


static STRPTR find_file
(
    STRPTR buffer, ULONG buflen,
    CONST_STRPTR name, CONST_STRPTR section
)
{
    // TODO: support different suffixes

    STRPTR retval = NULL;
    struct FileInfoBlock *fib = AllocDosObjectTagList(DOS_FIB, NULL);
    if (fib)
    {
        snprintf(buffer, buflen, "HELP:English/%s/%s.guide", section, name);
        D(bug("[Help/find_file] search for %s\n", buffer));
        BPTR lock = Lock(buffer, ACCESS_READ);
        if (lock)
        {
            BOOL success = Examine(lock, fib);
            if (success)
            {
                if (fib->fib_DirEntryType < 0) // is file?
                {
                    retval = buffer;
                }
            }
            UnLock(lock);
        }
        FreeDosObject(DOS_FIB, fib);
    }
    return retval;
}


int main(int argc, char **argv)
{
    if (argc) // started from Shell
    {
        IPTR args[ARG_COUNT] = {0};
        
        rda = ReadArgs(ARG_TEMPLATE, args, NULL);
        if (!rda)
        {
            PrintFault(IoErr(), argv[0]);
            clean_exit("ReadArgs() failed.\n");
        }
        
        if (args[ARG_NAME])
        {
            name = (STRPTR)args[ARG_NAME];
            if (strpbrk(name, ":/()#?~"))
            {
                clean_exit("Illegal characters in argument 'name'.\n");
            }
        }

        if (args[ARG_SECTION])
        {
            section = (STRPTR)args[ARG_SECTION];
            if (strpbrk(section, ":/()#?~"))
            {
                clean_exit("Illegal characters in argument 'section'.\n");
            }
        }

        D(bug("[Help] name %s section %s\n", name, section));

        if (name == NULL && section == NULL)
        {
            strcpy(docpath, "HELP:English/Index.guide");
            show_file(viewpath, sizeof(viewpath), docpath);
        }
        else if (section == NULL || *section == '\0')
        {
            CONST_STRPTR *sect;
            BOOL found = FALSE;
            for (sect = section_arr ; (found == FALSE) && (*sect) ; sect++)
            {
                if (find_file(docpath, sizeof(docpath), name, *sect))
                {
                    show_file(viewpath, sizeof(viewpath), docpath);
                    found = TRUE;
                }
            }
            if (found == FALSE)
            {
                clean_exit("Can't find help document.\n");
            }
        }
        else
        {
            if (find_file(docpath, sizeof(docpath), name, section))
            {
                show_file(viewpath, sizeof(viewpath), docpath);
            }
            else
            {
                clean_exit("Can't find help document.\n");
            }
        }
    }
    else // started from Wanderer
    {
        strcpy(docpath, "HELP:English/Index.guide");
        show_file(viewpath, sizeof(viewpath), docpath);
    }

    clean_exit(NULL);
    return 0;
}
