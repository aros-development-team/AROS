/*
 * Copyright (c) 2010-2011 Matthias Rustler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *   
 * $Id$
 */

#include <proto/utility.h>
#include <proto/dos.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "identify_intern.h"
#include "identify.h"

//#define DEBUG 1
#include <aros/debug.h>


static struct LibNode *searchLibrary(struct List *libList, CONST_STRPTR libname);
static BOOL addFuncNode(struct List *funcList, CONST_STRPTR line, ULONG offset, struct IdentifyBaseIntern *libBase);
static struct LibNode *loadFD(struct List *libList, CONST_STRPTR libname, struct IdentifyBaseIntern *libBase);
static CONST_STRPTR searchFunction(struct LibNode *libNode, ULONG offset);


/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH3(LONG, IdFunction,

/*  SYNOPSIS */
        AROS_LHA(STRPTR          , libname, A0),
        AROS_LHA(LONG            , offset , D0),
        AROS_LHA(struct TagItem *, taglist, A1),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 8, Identify)

/*  FUNCTION
        Decodes the offset of the provided library name into function name.

        This function requires the .fd files in a drawer with 'FD:' assigned
        to it. All files must have the standard file name format, e.g.
        'exec_lib.fd'.

        The appropriate .fd file will be scanned. The result will be
        cached until the identify.library is removed from system.

    INPUTS
        LibName -- (STRPTR) name of the function's library, device
                   or resource. All letters behind the point (and
                   the point itself) are optional. The name is
                   case sensitive.

                   Examples: 'exec.library', 'dos', 'cia.resource'.

        Offset  -- (LONG) offset of the function. It must be a
                   multiple of 6. You do not need to provide the
                   minus sign.

                   Examples: -456, 60

        TagList -- (struct TagItem *) tags that describe further
                   options.

    RESULT
        Error   -- (LONG) error code, or 0 if everything went fine.

   TAGS
        IDTAG_FuncNameStr   -- (STRPTR) Buffer where the function name
                               will be copied into.

        IDTAG_StrLength     -- (UWORD) Maximum length of the string buffer,
                               including termination. Defaults to 50.

    NOTES

    EXAMPLE

    BUGS
        Every line in the .fd file must have a maximum of 254 characters.
        Otherwise the internal offset table may be corrupted (but the
        system won't be harmed). Anyhow, this should be no problem.

    SEE ALSO

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag;
    struct TagItem *tags;
    STRPTR funcNameStr = NULL;
    ULONG strLength = 0;
    struct LibNode *libNode;
    LONG retVal = IDERR_OKAY;

    for (tags = taglist; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case IDTAG_FuncNameStr:
                funcNameStr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_StrLength:
                strLength = tag->ti_Data;
                break;
        }
    }

    if (funcNameStr == NULL || strLength == 0)
        return IDERR_NOLENGTH;

    offset = abs(offset);
    if (offset % 6)
        return IDERR_OFFSET;

    ObtainSemaphore(&IdentifyBase->sem);

    libNode = searchLibrary(&IdentifyBase->libList, libname);
    if (!libNode)
    {
        libNode = loadFD(&IdentifyBase->libList, libname, IdentifyBase);
    }

    if (libNode)
    {
        strlcpy(funcNameStr, searchFunction(libNode, offset), strLength);
        if (funcNameStr == NULL)
        {
            retVal = IDERR_OFFSET;
        }
    }
    else
    {
        retVal = IDERR_NOFD;
    }

    ReleaseSemaphore(&IdentifyBase->sem);

    return retVal;

    AROS_LIBFUNC_EXIT
} /* IdFunction */




static struct LibNode *searchLibrary(struct List *libList, CONST_STRPTR libname)
{
    struct LibNode *libNode;
    ULONG len;
    STRPTR dot = strchr(libname, '.');

    if (dot)
    {
        len = (IPTR)dot - (IPTR)libname;
    }
    else
    {
        len = strlen(libname);
    }

    D(bug("[idfunction/searchLibrary] libList %p libname %s len %u\n", libList, libname, len));

    ForeachNode(libList, libNode)
    {
        if (strncmp(libname, libNode->nd.ln_Name, len) == 0)
        {
            D(bug("[idfunction/searchLibrary] found %p\n", libNode));
            return libNode;
        }
    }
    D(bug("[idfunction/searchLibrary] found nothing\n"));
    return NULL;
}

static BOOL addFuncNode(struct List *funcList, CONST_STRPTR line, ULONG offset, struct IdentifyBaseIntern *libBase)
{
    D(bug("[idfunction/addFuncNode] funcList %p line %s offset %u\n", funcList, line, offset));

    struct FuncNode *newNode = AllocVecPooled(libBase->poolMem, sizeof (struct FuncNode));
    if (newNode)
    {
        memset(newNode, 0, sizeof (struct FuncNode));
        int len;

        STRPTR bracket = strchr(line, '(');
        if (bracket)
        {
            len = (IPTR)bracket - (IPTR)line;
        }
        else
        {
            len = strlen(line);
        }

        STRPTR name = AllocVecPooled(libBase->poolMem, len + 1);
        if (name)
        {
            newNode->nd.ln_Name = name;
            strlcpy(newNode->nd.ln_Name, line, len + 1);
            newNode->offset = offset;
            AddTail(funcList, (struct Node *)newNode);
            D(bug("[idfunction/addFuncNode] funcnode %p name %s offset %u created\n", newNode, newNode->nd.ln_Name, newNode->offset));
            return TRUE;
        }
        else
        {
            FreeVecPooled(libBase->poolMem, newNode);
        }
    }
    D(bug("[idfunction/addFuncNode] failed to create funcnode\n"));
    return FALSE;
}

static struct LibNode *loadFD(struct List *libList, CONST_STRPTR libname, struct IdentifyBaseIntern *libBase)
{
    struct LibNode *retVal = NULL;
    struct LibNode *newNode = NULL;
    STRPTR fileName = NULL;
    STRPTR libNameStripped = NULL;
    BPTR fileHandle = BNULL;
    TEXT buffer[256];
    ULONG offset = 0;
    ULONG len;
    STRPTR dot = strchr(libname, '.');

    D(bug("[idfunction/loadFD] libList %p libname %s\n", libList, libname));

    if (dot)
    {
        len = (IPTR)dot - (IPTR)libname;
    }
    else
    {
        len = strlen(libname);
    }

    libNameStripped = AllocVecPooled(libBase->poolMem, len + 1);
    if (libNameStripped == NULL)
    {
        D(bug("[idfunction/loadFD] out of mem for libNameStripped\n"));
        goto bailout;
    }
    strlcpy(libNameStripped, libname, len + 1);

    fileName = AllocVecPooled(libBase->poolMem, len + 30);
    if (fileName == NULL)
    {
        D(bug("[idfunction/loadFD] out of mem for filename\n"));
        goto bailout;
    }
    sprintf(fileName, "Developer:fd/%s_lib.fd", libNameStripped);

    D(bug("[idfunction/loadFD] libnamestripped %s filename %s\n", libNameStripped, fileName));

    fileHandle = Open(fileName, MODE_OLDFILE);
    if (fileHandle == BNULL)
    {
        D(bug("[idfunction/loadFD] failed to open file\n"));
        goto bailout;
    }

    newNode = AllocVecPooled(libBase->poolMem, sizeof (struct LibNode));
    if (newNode == NULL)
    {
        D(bug("[idfunction/loadFD] out of mem for LibNode\n"));
        goto bailout;
    }

    newNode->nd.ln_Name = libNameStripped;

    NEWLIST(&newNode->funcList);

    while (FGets(fileHandle, buffer, sizeof (buffer)))
    {
        D(bug("[idfunction/loadFD] line %s\n", buffer));

        if (buffer[0] == '*')
        {
            D(bug("[idfunction/loadFD] comment\n"));
        }
        else if (buffer[0] == '#')
        {
            if (strncmp(buffer, "##bias", 6) == 0) 
            {
                D(bug("[idfunction/loadFD] ##bias\n"));
                LONG value;
                StrToLong(buffer + 6, &value);
                offset = value;
            }
            else if (strncmp(buffer, "##end", 5) == 0) 
            {
                D(bug("[idfunction/loadFD] ##end\n"));
                break;
            }
            else
            {
                D(bug("[idfunction/loadFD] unknown command\n"));
            }
        }
        else
        {
            addFuncNode(&newNode->funcList, buffer, offset, libBase);
            offset += 6;
        }
    }
    AddTail(libList, (struct Node *)newNode);
    retVal = newNode;

bailout:
    if (fileHandle) Close(fileHandle);
    FreeVecPooled(libBase->poolMem, fileName);
    return retVal;
}

static CONST_STRPTR searchFunction(struct LibNode *libNode, ULONG offset)
{
    struct FuncNode *node;

    D(bug("[idfunction/searchFunction] libNode %p offset %u\n", libNode, offset));

    ForeachNode(&libNode->funcList, node)
    {
        if (node->offset == offset)
        {
            D(bug("[idfunction/searchFunction] found node %p name %s\n", node, node->nd.ln_Name));
            return node->nd.ln_Name;
        }
    }
    return ""; // FIXME: should we check that offset doesn't exceed highest LVO?
}
