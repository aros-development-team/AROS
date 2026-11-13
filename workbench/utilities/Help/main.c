/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
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

        NAME    --  Name of a command or application whose guide is requested.
                    Required when SECTION is specified.
        SECTION --  Subdirectory of HELP:English to search. When omitted,
                    Commands is searched first, followed by System.
                    Name matching follows the underlying file system.

    RESULT

        RETURN_OK (0) after normal viewer termination.
        RETURN_ERROR (10) for invalid arguments, a missing document,
        or failure to start the viewer. Other viewer statuses are
        returned unchanged.

    EXAMPLE

        Help dir
        Help dir commands
        Help help

******************************************************************************/

#include <dos/dos.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

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


/****i* Help/clean_exit ***************************************************
*
*   NAME
*       clean_exit -- Release parsed arguments and return a status.
*
*   SYNOPSIS
*       status = clean_exit(message, status)
*
*   FUNCTION
*       Prints message if it is not NULL, and releases the RDArgs allocation
*       held in rda if one exists. Does not terminate the process.
*
*   INPUTS
*       message - Optional NUL-terminated diagnostic string.
*       status  - DOS return code to pass back to the caller.
*
*   RESULT
*       The supplied status, unchanged.
*
****************************************************************************
*
*/

static LONG clean_exit(CONST_STRPTR s, LONG result)
{
    if (s) PutStr(s);
    if (rda) FreeArgs(rda);
    return result;
}


/****i* Help/show_file ****************************************************
*
*   NAME
*       show_file -- Display a guide using the system MultiView command.
*
*   SYNOPSIS
*       status = show_file(buffer, buflen, path)
*
*   INPUTS
*       buffer - Caller-owned command buffer.
*       buflen - Capacity of buffer, including its NUL terminator.
*       path   - Guide path to append to the MultiView command line.
*
*   RESULT
*       RETURN_OK for a SystemTags() status of 0 or 1. All other statuses
*       are returned unchanged, including -1 when the shell cannot start.
*
*   NOTES
*       The command buffer is not checked for truncation.
*
****************************************************************************
*
*/

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
    /* MultiView returns 1 on normal shutdown. */
    return error == 1 ? RETURN_OK : error;
}


/****i* Help/find_file ****************************************************
*
*   NAME
*       find_file -- Locate a regular guide file in a help section.
*
*   SYNOPSIS
*       match = find_file(buffer, buflen, name, section)
*
*   FUNCTION
*       Formats HELP:English/section/name.guide, then uses Lock() and
*       Examine() to check whether the path denotes a file.
*
*   INPUTS
*       buffer  - Caller-owned pathname buffer.
*       buflen  - Capacity of buffer, including its NUL terminator.
*       name    - Non-NULL guide name.
*       section - Non-NULL section directory name.
*
*   RESULT
*       buffer when a regular file is found; NULL otherwise, including
*       allocation, Lock() and Examine() failures.
*
*   NOTES
*       Only .guide files are searched. A truncated pathname is not
*       distinguished from a missing document.
*
****************************************************************************
*
*/

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


/* Shell and Wanderer entry point; see the public Help AutoDoc above. */
int main(int argc, char **argv)
{
    LONG result = RETURN_OK;
    if (argc) // started from Shell
    {
        IPTR args[ARG_COUNT] = {0};
        
        rda = ReadArgs(ARG_TEMPLATE, args, NULL);
        if (!rda)
        {
            PrintFault(IoErr(), argv[0]);
            return clean_exit("ReadArgs() failed.\n", RETURN_ERROR);
        }
        
        if (args[ARG_NAME])
        {
            name = (STRPTR)args[ARG_NAME];
            if (strpbrk(name, ":/()#?~"))
            {
                return clean_exit("Illegal characters in argument 'name'.\n", RETURN_ERROR);
            }
        }

        if (args[ARG_SECTION])
        {
            section = (STRPTR)args[ARG_SECTION];
            if (strpbrk(section, ":/()#?~"))
            {
                return clean_exit("Illegal characters in argument 'section'.\n", RETURN_ERROR);
            }
        }

        if (section && !name)
        {
            return clean_exit("NAME is required when SECTION is specified.\n", RETURN_ERROR);
        }

        D(bug("[Help] name %s section %s\n", name, section));

        if (name == NULL && section == NULL)
        {
            strcpy(docpath, "HELP:English/Index.guide");
            result = show_file(viewpath, sizeof(viewpath), docpath);
        }
        else if (section == NULL || *section == '\0')
        {
            CONST_STRPTR *sect;
            BOOL found = FALSE;
            for (sect = section_arr ; (found == FALSE) && (*sect) ; sect++)
            {
                if (find_file(docpath, sizeof(docpath), name, *sect))
                {
                    result = show_file(viewpath, sizeof(viewpath), docpath);
                    found = TRUE;
                }
            }
            if (found == FALSE)
            {
                return clean_exit("Can't find help document.\n", RETURN_ERROR);
            }
        }
        else
        {
            if (find_file(docpath, sizeof(docpath), name, section))
            {
                result = show_file(viewpath, sizeof(viewpath), docpath);
            }
            else
            {
                return clean_exit("Can't find help document.\n", RETURN_ERROR);
            }
        }
    }
    else // started from Wanderer
    {
        strcpy(docpath, "HELP:English/Index.guide");
        result = show_file(viewpath, sizeof(viewpath), docpath);
    }

    return clean_exit(NULL, result == -1 ? RETURN_ERROR : result);
}
