/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"


static STRPTR SkipChars(STRPTR v)
{
    STRPTR c;
    c = strstr(v, "=");
    return ++c;
}

LONG GetInt(STRPTR v)
{
    STRPTR c;
    c = SkipChars(v);
    return (LONG) atol(c);
}

void GetIntegers(STRPTR v, LONG *v1, LONG *v2)
{
    STRPTR c;
    TEXT va1[32], va2[32];
    LONG cnt;
    c = SkipChars(v);
    if (c)
    {
        cnt = sscanf(c, "%s %s", va1, va2);
        if (cnt == 1)
        {
            *v1 = -1;
            *v2 = atol(va1);
        }
        else if (cnt == 2)
        {
            *v1 = atol(va1);
            *v2 = atol(va2);
        }
    }
}

void GetTripleIntegers(STRPTR v, LONG *v1, LONG *v2, LONG *v3)
{
    STRPTR ch;
    LONG a, b, c;
    LONG cnt;
    ch = SkipChars(v);
    if (ch)
    {
        cnt = sscanf(ch, "%x %x %d", &a, &b, &c);
        if (cnt == 3)
        {
            *v1 = a;
            *v2 = b;
            *v3 = c;
        }
    }
}

void GetColors(STRPTR v, LONG *v1, LONG *v2)
{
    STRPTR ch;
    LONG a, b;
    LONG cnt;
    ch = SkipChars(v);
    if (ch)
    {
        cnt = sscanf(ch, "%x %x", &a, &b);
        if (cnt == 2)
        {
            *v1 = a;
            *v2 = b;
        }
    }
}


BOOL GetBool(STRPTR v, STRPTR id)
{
    if (strstr(v, id)) return TRUE; else return FALSE;
}

static void LoadMenuConfig(STRPTR path, struct DecorConfig * dc)
{
    TEXT    buffer[256];
    STRPTR  line, v;
    BPTR    file;
    BPTR    lock;
    BPTR    olddir = 0;

    dc->MenuIsTiled = FALSE;
    dc->MenuTileLeft = 0;
    dc->MenuTileTop = 0;
    dc->MenuTileRight = 0;
    dc->MenuTileBottom = 0;
    dc->MenuInnerLeft = 0;
    dc->MenuInnerTop = 0;
    dc->MenuInnerRight = 0;
    dc->MenuInnerBottom = 0;

    lock = Lock(path, ACCESS_READ);
    if (lock)
        olddir = CurrentDir(lock);
    else 
        return;

    file = Open("Menu/Config", MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "TileLeft ")) == line)
                {
                    dc->MenuTileLeft = GetInt(v);
                    dc->MenuIsTiled = TRUE;
                }
                else  if ((v = strstr(line, "TileTop ")) == line)
                {
                    dc->MenuTileTop = GetInt(v);
                    dc->MenuIsTiled = TRUE;
                }
                else  if ((v = strstr(line, "TileRight ")) == line)
                {
                    dc->MenuTileRight = GetInt(v);
                    dc->MenuIsTiled = TRUE;
                }
                else  if ((v = strstr(line, "TileBottom ")) == line)
                {
                    dc->MenuTileBottom = GetInt(v);
                    dc->MenuIsTiled = TRUE;
                }
                else if ((v = strstr(line, "InnerLeft ")) == line)
                {
                    dc->MenuInnerLeft = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerTop ")) == line)
                {
                    dc->MenuInnerTop = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerRight ")) == line)
                {
                    dc->MenuInnerRight = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerBottom ")) == line)
                {
                    dc->MenuInnerBottom = GetInt(v);
                }
            }
        }
        while(line);
        Close(file);
    }

    if (olddir) CurrentDir(olddir);
    UnLock(lock);
}

struct DecorConfig * LoadConfig(STRPTR path)
{
    struct DecorConfig * dc = AllocVec(sizeof(struct DecorConfig), MEMF_ANY | MEMF_CLEAR);
    
    LoadMenuConfig(path, dc);
    
    return dc;
};
