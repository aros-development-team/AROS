/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/******************************************************************************

    NAME

        Help name (section)

    SYNOPSIS

        NAME/A, SECTION

    LOCATION

        SYS:Utilities

    FUNCTION

        Shows help information for commands and system applications. The help files
        are stored in HELP:English with a sub-directory for each section. More
        sections might be available in the future (similar than "man1", "man2" etc.
        on Linux systems). The utility "Multiview" is used to show the guide files.

    INPUTS

        NAME    --  the name of the command or application whose help you want
                    to view. The name is case-insensitive.
	SECTION --  the section where the help document will be searched. Currently
                    available are "commands" and "system". If you don't specify
                    the section all available sections will be searched. The
                    section is case-insensitive.

    RESULT

    NOTES

    EXAMPLE

        Help dir
        Help dir commands
        Help help

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/dos.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#include <aros/debug.h>

#define ARG_TEMPLATE "NAME/A,SECTION"

const char *ver = "$VER: Help 1.0 (01.03.2013)";

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
    struct AnchorPath ap;
    memset(&ap, 0, sizeof ap);
    snprintf(buffer, buflen, "HELP:English/%s/%s.guide", section, name);
    LONG error = MatchFirst(buffer, &ap);
    MatchEnd(&ap);
    D(bug("[Help/find_file] path %s error %d\n", buffer, error));
    if (error) return NULL;
    return buffer;
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
        }

        if (args[ARG_SECTION])
        {
            section = (STRPTR)args[ARG_SECTION];
        }

        D(bug("[Help] name %s section %s\n", name, section));

        if (section == NULL || *section == '\0')
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
                PutStr("Can't find help document\n");
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
                PutStr("Can't find help document\n");
            }
        }
    }
    else // started from Wanderer
    {
        // FIXME: open Multiview?
    }

    clean_exit(NULL);
    return 0;
}
