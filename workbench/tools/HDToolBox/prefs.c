/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <workbench/icon.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define DEBUG 0
#include "debug.h"

#include "prefs.h"
#include "devices.h"
#include "hdtoolbox_support.h"
#include "platform.h"

struct List tabletypelist;

struct TableTypeNode *findTableTypeNodeName(STRPTR name)
{
    struct TableTypeNode *ttn;

    D(bug("[HDToolBox] findTableTypeNodeName('%s')\n", name));

    ttn = (struct TableTypeNode *)tabletypelist.lh_Head;
    while (ttn->ln.ln_Succ)
    {
        if (strcmp(ttn->pti->pti_Name, name) == 0)
            return ttn;
        ttn = (struct TableTypeNode *)ttn->ln.ln_Succ;
    }
    return NULL;
}

struct TableTypeNode *findTableTypeNode(ULONG tabletype)
{
    struct TableTypeNode *ttn;

    D(bug("[HDToolBox] findTableTypeNode()\n"));

    ttn = (struct TableTypeNode *)tabletypelist.lh_Head;
    while (ttn->ln.ln_Succ)
    {
        if (ttn->pti->pti_Type == tabletype)
            return ttn;
        ttn = (struct TableTypeNode *)ttn->ln.ln_Succ;
    }
    return NULL;
}

struct TypeNode *findPartitionType(struct PartitionType *type, ULONG tabletype)
{
    struct TableTypeNode *ttn;
    struct TypeNode *tn;

    D(bug("[HDToolBox] findPartitionType()\n"));

    ttn = findTableTypeNode(tabletype);
    if (ttn)
    {
        tn = (struct TypeNode *)ttn->typelist->lh_Head;
        while (tn->ln.ln_Succ)
        {
            if (tn->type.id_len == type->id_len)
            {
                if (memcmp(tn->type.id, type->id, type->id_len) == 0)
                    return tn;
            }
            tn = (struct TypeNode *)tn->ln.ln_Succ;
        }
    }
    return NULL;
}

void getTableTypeList(struct List *list)
{
    struct TableTypeNode *ttn;
    int i;

    D(bug("[HDToolBox] getTableTypeList()\n"));

    NEWLIST(&tabletypelist);
    for (i=0;PartitionBase->tables[i];i++)
    {
        ttn = AllocMem(sizeof(struct TableTypeNode), MEMF_PUBLIC | MEMF_CLEAR);
        if (ttn)
        {
            ttn->typelist =
                AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
            if (ttn->typelist != NULL)
            {
                ttn->pti = PartitionBase->tables[i];
                NEWLIST(ttn->typelist);
                AddTail(list, &ttn->ln);
            }
            else
            {
                FreeMem(ttn, sizeof(struct TableTypeNode));
                ttn = NULL;
            }
        }
    }
}

LONG parsePrefs(char *buffer, LONG size)
{
    struct TableTypeNode *ttn=NULL;
    struct TypeNode *tn;
    struct CSource csrc = {buffer, size, 0};
    struct DiskObject *hdtbicon=NULL;
    char ident[256];
    LONG res;
    ULONG id_len = 0;
    WORD current = 0;
    WORD line = 1;

    D(bug("[HDToolBox] parsePrefs()\n"));

    while (csrc.CS_CurChr < csrc.CS_Length)
    {
        res = ReadItem(ident, 256, &csrc);
        switch (res)
        {
        case ITEM_ERROR:
            return IoErr();
        case ITEM_UNQUOTED:
            if (strcasecmp(ident, "[Devices]") == 0)
                current = 1;
            else if (strcasecmp(ident, "[TableIDs]") == 0)
            {
                current = 2;
                ttn = NULL;
                id_len = 0;
            }
            else
            {
                if (current == 1)
                    addDeviceName(ident);
                else if (current == 2)
                {
                    if (strcasecmp(ident, "TableType") == 0)
                    {
                        res = ReadItem(ident, 256, &csrc);
                        if (res == ITEM_ERROR)
                            return IoErr();
                        else if (res == ITEM_EQUAL)
                        {
                            res = ReadItem(ident, 256, &csrc);
                            if (res == ITEM_ERROR)
                                return IoErr();
                            else if (res == ITEM_QUOTED)
                                ttn = findTableTypeNodeName(ident);
                            else if (res == ITEM_UNQUOTED)
                                ttn = findTableTypeNode(strtoul(ident, NULL, 0));
                            else
                            {
                                printf("LINE %d: Unexpected item in TableType\n", line);
                                return 0;
                            }
                            if (ttn == 0)
                            {
                                printf("LINE %d: Unknown Table %s\n", line, ident);
                                return 0;
                            }
                        }
                        else
                        {
                            printf("LINE %d: Unexpected item after TableType\n", line);
                            return 0;
                        }
                    }
                    else if (strcasecmp(ident, "IDLen") == 0)
                    {
                        res = ReadItem(ident, 256, &csrc);
                        if (res == ITEM_ERROR)
                            return IoErr();
                        else if (res == ITEM_EQUAL)
                        {
                            res = ReadItem(ident, 256, &csrc);
                            if (res == ITEM_ERROR)
                                return IoErr();
                            else if (res == ITEM_UNQUOTED)
                            {
                                id_len = strtoul(ident, NULL, 0);
                                if (id_len == 0)
                                {
                                    printf("LINE %d: Illegal value of IDLen\n", line);
                                    return 0;
                                }
                            }
                            else
                            {
                                printf("LINE %d: Value in IDLen expected\n", line);
                                return 0;
                            }
                        }
                        else
                        {
                            printf("LINE %d: Unexpected item after IDLen\n", line);
                            return 0;
                        }
                    }
                    else if (strcasecmp(ident, "Default") == 0)
                    {
                        if (ttn && id_len)
                        {
                            res = ReadItem(ident, 256, &csrc);
                            if (res == ITEM_ERROR)
                                return IoErr();
                            else if (res == ITEM_EQUAL)
                            {
                                res = ReadItem(ident, 256, &csrc);
                                if (res == ITEM_ERROR)
                                    return IoErr();
                                else if (res == ITEM_QUOTED)
                                {
                                    ttn->defaulttype.id_len = id_len;
                                    strcpyESC(ttn->defaulttype.id, ident);
                                }
                                else
                                {
                                    printf("LINE %d: Unexpected expression after Default\n", line);
                                    return 0;
                                }
                            }
                            else
                            {
                                printf("LINE %d: Unexpected item after IDLen\n", line);
                                return 0;
                            }
                        }
                        else
                        {
                            printf("LINE %d: Unknown option '%s'\n", line, ident);
                            return 0;
                        }
                    }
                    else
                    {
                        printf("LINE %d: Missing partition table type or IDLen\n", line);
                        return 0;
                    }
                }
                else
                {
                    printf("LINE %d: Unexpected item '%s' in prefs\n", line, ident);
                    return 0;
                }
            }
            break;
        case ITEM_QUOTED:
            if (ttn && id_len)
            {
                tn = AllocMem(sizeof(struct TypeNode), MEMF_PUBLIC | MEMF_CLEAR);
                if (tn == NULL)
                    return ERROR_NO_FREE_STORE;
                tn->type.id_len = id_len;
                strcpyESC(tn->type.id, ident);
                res = ReadItem(ident, 256, &csrc);
                if (res == ITEM_ERROR)
                    return IoErr();
                if (res == ITEM_EQUAL)
                {
                    res = ReadItem(ident, 256, &csrc);
                    if (res == ITEM_ERROR)
                        return IoErr();
                    else if (res == ITEM_QUOTED)
                    {
                        tn->ln.ln_Name = AllocVec(strlen(ident)+1, MEMF_PUBLIC | MEMF_CLEAR);
                        if (tn->ln.ln_Name == NULL)
                            return ERROR_NO_FREE_STORE;
                        strcpy(tn->ln.ln_Name, ident);
                    }
                    else
                    {
                        printf("LINE %d: Quoted expression expected\n", line);
                        return 0;
                    }
                }
                else
                {
                    printf("LINE %d: Unexpected item after table id\n", line);
                    return 0;
                }
                AddTail(ttn->typelist, &tn->ln);
            }
            else
            {
                printf("LINE %d: Missing partition table type or IDLen\n", line);
                return 0;
            }
            break;
        case ITEM_NOTHING:
            line++;
            break;
        }
    }

   hdtbicon = GetIconTags("HDToolBox",
            ICONGETA_FailIfUnavailable, TRUE,
            ICONGETA_IsDefaultIcon, FALSE,
            TAG_DONE);

    if (hdtbicon != NULL)
    {
D(bug("[HDToolBox] Got our Icon..\n"));
	if (hdtbicon->do_ToolTypes)
	{
	    char *tt = NULL, *devicename;
	    int   i  = 0;
D(bug("[HDToolBox] Icon has tooltypes..\n"));

	    while ((tt = hdtbicon->do_ToolTypes[i]) != NULL)
	    {
                if (strncmp(hdtbicon->do_ToolTypes[i], "DEVICE=", 7) == 0)
                {
		    devicename = hdtbicon->do_ToolTypes[i] + 7;
D(bug("[HDToolBox] Adding Device '%s' from ToolType\n", devicename));
		    addDeviceName(devicename);
		}
		i++;
	    }
	}
    }

    // EBR uses same types as MBR
    findTableTypeNode(PHPTT_EBR)->typelist =
        findTableTypeNode(PHPTT_MBR)->typelist;

    return 0;
}

void LoadPrefs(STRPTR filename) 
{
    struct FileInfoBlock fib;
    char *buffer;
    LONG retval;
    LONG size;
    BPTR fh;

    D(bug("[HDToolBox] LoadPrefs('%s')\n", filename));

    getTableTypeList(&tabletypelist);
    fh = Open(filename, MODE_OLDFILE);
    if (fh)
    {
        if (ExamineFH(fh, &fib))
        {
            if (fib.fib_Size>0)
            {
                buffer = AllocMem(fib.fib_Size, MEMF_PUBLIC | MEMF_CLEAR);
                if (buffer)
                {
                    size = Read(fh, buffer, fib.fib_Size);
                    if (size == fib.fib_Size)
                    {
                        retval = parsePrefs(buffer, size);
                        if (retval)
                            PrintFault(retval, filename);
                    }
                    FreeMem(buffer, fib.fib_Size);
                }
                else
                    PrintFault(ERROR_NO_FREE_STORE, filename);
            }
        }
        Close(fh);
    }
    else
        PrintFault(IoErr(), filename);
}
