/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/utility.h>
#include <proto/dos.h>

#include <string.h>
#include <stdlib.h>

#include "identify_intern.h"
#include "identify.h"

#define DEBUG 1
#include <aros/debug.h>


static struct LibNode *searchLibrary(struct List *libList, CONST_STRPTR libname);
static BOOL addFuncNode(struct List *funcList, CONST_STRPTR line, ULONG offset);
static struct LibNode *loadFD(struct List *libList, CONST_STRPTR libname);
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag;
    const struct TagItem *tags;
    CONST_STRPTR funcNameStr = NULL;
    ULONG strLength = 0;
    struct LibNode *libNode;

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

    libNode = searchLibrary(&IdentifyBase->libList, libname);
    if (!libNode)
    {
        libNode = loadFD(&IdentifyBase->libList, libname);
    }

    if (libNode)
    {
        funcNameStr = searchFunction(libNode, offset);
        if (funcNameStr == NULL)
        {
            return IDERR_OFFSET;
        }
    }
    else
    {
        return IDERR_NOFD;
    }

    return IDERR_OKAY;

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

static BOOL addFuncNode(struct List *funcList, CONST_STRPTR line, ULONG offset)
{
    D(bug("[idfunction/addFuncNode] funcList %p line %s offset %u\n", funcList, line, offset));

    struct FuncNode *newNode = AllocVec(sizeof (struct FuncNode), MEMF_CLEAR);
    if (newNode)
    {
        // TODO: copy only function name
        STRPTR name = AllocVec(strlen(line) + 1, MEMF_ANY);
        if (name)
        {
            newNode->nd.ln_Name = name;
            strcpy(newNode->nd.ln_Name, line);
            newNode->offset = offset;
            AddTail(funcList, (struct Node *)newNode);
            D(bug("[idfunction/addFuncNode] funcnode %p created\n", newNode));
            return TRUE;
        }
        else
        {
            FreeVec(newNode);
        }
    }
    D(bug("[idfunction/addFuncNode] failed to create funcnode\n"));
    return FALSE;
}

static struct LibNode *loadFD(struct List *libList, CONST_STRPTR libname)
{
    struct LibNode *retVal = NULL;
    struct LibNode *newNode = NULL;
    STRPTR fileName = NULL;
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

    fileName = AllocVec(len + 30, MEMF_ANY);
    if (fileName == NULL)
    {
        D(bug("[idfunction/loadFD] out of mem for filename\n"));
        goto bailout;
    }

    strcpy(fileName, "Development:fd/");
    strncat(fileName, libname, len);
    strcat(fileName, "_lib.fd");

    D(bug("[idfunction/loadFD] filename %s len %u\n", fileName, len));

    fileHandle = Open(fileName, MODE_OLDFILE);
    if (fileHandle == NULL)
    {
        D(bug("[idfunction/loadFD] failed to open file\n"));
        goto bailout;
    }

    newNode = AllocVec(sizeof (struct LibNode), MEMF_CLEAR);
    if (newNode == NULL)
    {
        D(bug("[idfunction/loadFD] out of mem for LibNode\n"));
        goto bailout;
    }

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
                offset += value;
            }
            else if (strncmp(buffer, "##end", 5) == 0) 
            {
                D(bug("[idfunction/loadFD] ##break\n"));
                break;
            }
            else
            {
                D(bug("[idfunction/loadFD] unknown command\n"));
            }
        }
        else
        {
            BOOL success = addFuncNode(&newNode->funcList, buffer, offset);
        }
    }
    AddTail(libList, (struct Node *)newNode);

bailout:
    if (fileHandle) Close(fileHandle);
    FreeVec(fileName);
    return retVal;
}

static CONST_STRPTR searchFunction(struct LibNode *libNode, ULONG offset)
{
    struct FuncNode *node;

    offset = abs(offset);

    D(bug("[idfunction/searchFunction] libNode %p offset %u\n", libNode, offset));

    ForeachNode(&libNode->funcList, node)
    {
        if (node->offset == offset)
        {
            D(bug("[idfunction/searchFunction] found node %p name %s\n", node, node->nd.ln_Name));
            return node->nd.ln_Name;
        }
    }
    return NULL;
}
