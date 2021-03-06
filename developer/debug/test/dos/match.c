/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
*/

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************************/

static void showacflags(struct AChain *ac)
{
    BYTE flags = ac->an_Flags;

    printf("(");

    if (flags & DDF_PatternBit)
    {
        flags &= ~DDF_PatternBit;
        printf("DDF_PatternBit ");
    }

    if (flags & DDF_ExaminedBit)
    {
        flags &= ~DDF_ExaminedBit;
        printf("DDF_ExaminedBit ");
    }

    if (flags & DDF_Completed)
    {
        flags &= ~DDF_Completed;
        printf("DDF_Completed ");
    }

    if (flags & DDF_AllBit)
    {
        flags &= ~DDF_AllBit;
        printf("DDF_All ");
    }

    if (flags & DDF_Single)
    {
         flags &= ~DDF_Single;
         printf("DDF_Single ");
    }

    if (flags)
    {
         printf("UNKNOWN = %8x ", flags);
    }

    printf(")");
}

static void showaclist(struct AChain *ac)
{
    while(ac)
    {
        printf("achain: address = %p flags = %x ", ac, ac->an_Flags);
        showacflags(ac);
        printf(" string=\"%s\"\n", ac->an_String);
        ac = ac->an_Child;
    }

}

/****************************************************************************************/

#define ARG_TEMPLATE "FILE/A,ALL/S"
#define ARG_FILE 0
#define ARG_ALL  1
#define NUM_ARGS 2

/****************************************************************************************/

static char s[300];
static char *filename;
static BOOL all;
static struct RDArgs *myargs;
static IPTR args[NUM_ARGS];

/****************************************************************************************/

static void cleanup(char *msg)
{
    if (msg) printf("newmatch: %s\n", msg);

    if (myargs) FreeArgs(myargs);
        
    exit(0);
}

/****************************************************************************************/

static void doserror(void)
{
    Fault(IoErr(), 0, s, 255);
    cleanup(s);
}

/****************************************************************************************/

static void getarguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
        doserror();
    }
    
    filename = (char *)args[ARG_FILE];
    all = args[ARG_ALL] ? TRUE : FALSE;
}

/****************************************************************************************/

static void my_matchme(char *pattern, BOOL all)
{
    struct AnchorPath stackap[2], *AP;
    LONG error = 0;

    AP = (struct AnchorPath *)((((IPTR)stackap) + 3) & ~3);

    memset(AP, 0, sizeof(struct AnchorPath));

    error =  MatchFirst(pattern, AP);

    if (error != 0)
    {
        printf("MatchFirst: error = %d\n", (int)error);
    }
    else
    {
        printf("direntrytype = %d\n", (int)AP->ap_Info.fib_DirEntryType);
        if (!(AP->ap_Flags & APF_ITSWILD) &&
             (AP->ap_Info.fib_DirEntryType > 0))
        {
             /* pattern was an explicitely named directory */
             AP->ap_Flags |= APF_DODIR;
        }
        
        printf("ap_Flags = %x\n", AP->ap_Flags);
        NameFromLock(AP->ap_Current->an_Lock, s, 300);
        printf("BaseLock = \"%s\"\n", s);

        showaclist(AP->ap_Base);

        while(error == 0)
        {
            if (AP->ap_Flags & APF_DIDDIR)
            {
                printf("DIDDIR: ");
            } else {
                if (all && (AP->ap_Info.fib_DirEntryType > 0))
                {
                    AP->ap_Flags |= APF_DODIR;
                    printf("DOING DIR: ");
                }
            }
            printf("fib_FileName = \"%s\"\n", AP->ap_Info.fib_FileName);

            error = MatchNext(AP);
        }

    }

    MatchEnd(AP);
    
}

/****************************************************************************************/
/****************************************************************************************/

int main(void)
{
    getarguments();
    my_matchme(filename, all);
    cleanup(0);
    return 0;
}

/****************************************************************************************/
