/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#define DEBUG 0
#include "debug.h"

#include "hdtoolbox_support.h"
#include "platform.h"

struct Node *getNumNode(struct List *list, int num)
{
    struct Node *node;

    D(bug("[HDToolBox] getNumNode()\n"));

    node = list->lh_Head;
    while ((num) && (node->ln_Succ))
    {
        node = node->ln_Succ;
        num--;
    }
    return node->ln_Succ ? node : 0;
}

ULONG getNodeNum(struct Node *node)
{
    ULONG num = 0;

    D(bug("[HDToolBox] getNodeNum()\n"));

    for (;;)
    {
        node = node->ln_Pred;
        if (node->ln_Pred == 0)
            return num;
        num++;
    }
}

ULONG countNodes(struct List *list, UBYTE type)
{
    ULONG count = 0;
    struct Node *node;

    D(bug("[HDToolBox] countNodes()\n"));

    node = list->lh_Head;
    while (node->ln_Succ)
    {
        if ((type == (UBYTE)-1) || (node->ln_Type == type))
            count++;
        node = node->ln_Succ;
    }
    return count;
}


void typestrncpy(STRPTR dst, STRPTR src, ULONG len)
{
    while (len)
    {
        if (isprint(*src))
            *dst++ = *src++;
        else
        {
            *dst++ = '\\';
            sprintf(dst,"%o", *src++);
            while (*dst)
                dst++;
        }
        len--;
    }
}

UWORD strcpyESC(STRPTR dst, STRPTR fmt, ...)
{
    UWORD count=0;

    while (*fmt)
    {
        if (*fmt == '\\')
        {
            fmt++;
            if (isdigit(*fmt))
            {
            ULONG val=0;

                for(;;)
                {
                    val += (*fmt-'0');
                    fmt++;
                    if (!isdigit(*fmt))
                        break;
                    val *= 8;
                }
                *dst++ = (UBYTE)val;
                count++;
            }
            else
                kprintf("%s-%ld: unknown escape sequence\n", __FILE__, __LINE__);
        }
        else
        {
            *dst++ = *fmt++;
            count++;
        }
    }
    return count;
}

/* size in kB */
void getSizeStr(STRPTR str, ULONG size)
{
    UBYTE c='M';
    ULONG r;

    r = size % 1024;
    size = size / 1024;
    if (size>512)
    {
        c='G';
        r = size % 1024;
        size = size / 1024;
    }
    r = r*10/1024;
    sprintf(str, "%ld.%ld%c",size,r,c);
}

/* size in kB */
ULONG sizeStrToUL(STRPTR str)
{
    char *end;
    ULONG size;
    ULONG value=0;
    ULONG div;

    size = strtoul(str, &end, 0);
    if (*end == '.')
    {
        value = strtoul(end+1, &end, 0);
    }
    if (*end == 'M')
    {
        size *= 1024;
        div = 1024;
    }
    else if (*end == 'G')
    {
        size *= 1024*1024;
        div = 1024*1024;
    }
    else
    {
        /* assume bytes */
        value = 0;
        div = 0;
        size /= 1024; /* we want it in kB */
    }
    if (div)
    {
    ULONG d=1;
        do
        {
            d *= 10;
        } while ((value/d)>=1);
        value *= div;
        value /= d;
    }
    size += value;
    return size;
}

LONG GetPartitionAttrsA(struct PartitionHandle *ph, LONG tag, ...)
{
    D(bug("[HDToolBox] GetPartitionAttrsA()\n"));

    return GetPartitionAttrs(ph, (struct TagItem *)&tag);
}

LONG SetPartitionAttrsA(struct PartitionHandle *ph, LONG tag, ...)
{
    D(bug("[HDToolBox] SetPartitionAttrsA()\n"));

    return SetPartitionAttrs(ph, (struct TagItem *)&tag);
}

LONG GetPartitionTableAttrsA(struct PartitionHandle *ph, LONG tag, ...)
{
    D(bug("[HDToolBox] GetPartitionTableAttrsA()\n"));

    return GetPartitionTableAttrs(ph, (struct TagItem *)&tag);
}

ULONG getAttrInfo(struct PartitionAttribute *attrlist, ULONG attr)
{
    D(bug("[HDToolBox] getAttrInfo()\n"));

    while (attrlist[0].attribute != PTA_DONE)
    {
        if (attrlist[0].attribute == attr)
            return attrlist[0].mode;
        attrlist++;
    }
    return 0;
}

UBYTE getBitNum(ULONG val)
{
    UBYTE count=0;

    if (val==0)
        return 0xFF;

    for (;;)
    {
        val >>= 1;
        if (val==0)
            break;
        count++;
    }
    return count;
}
