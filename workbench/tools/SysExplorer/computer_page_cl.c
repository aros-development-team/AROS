/*
    Copyright (C) 2013, The AROS Development Team.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <resources/hpet.h>
#include <libraries/mui.h>
#include <mui/NFloattext_mcc.h>
#include <resources/processor.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/alib.h>
#include <proto/aros.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/hpet.h>
#include <proto/kernel.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/processor.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "classes.h"
#include "cpuspecific.h"
#include "locale.h"

#include <aros/debug.h>

#include <zune/customclasses.h>

APTR ProcessorBase = NULL;


/*** Instance Data **********************************************************/
struct ComputerWindow_DATA
{
    /* Nothing to add */
};


ULONG ExtUDivMod32(ULONG a, ULONG b, ULONG *mod)
{
    *mod = a % b;

    return a/b;
}


void PrintNum(char *buffer, LONG bufsize, ULONG num)
{
    /* MBytes ? */
    if(num > 1023) 
    {
        ULONG  x, xx;
        char* fmt = "meg";
        
        /* GBytes ? */
        if(num > 0xfffff)
        { 
            num >>= 10; 
            fmt = "gig";
        }
        
        num = ExtUDivMod32(UMult32(num, 100) >> 10, 100, &x);
        
        /* round */
        x = ExtUDivMod32(x, 10, &xx);
        
        if(xx > 4)
        {
            if(++x > 9)
            {
                x = 0;
                num++;
            }
        }

        snprintf(buffer, bufsize, "%d.%d %s", (int)num, (int)x, fmt);
    }
    else 
    {
        snprintf(buffer, bufsize, "%d K", (int)num);
    }
}

ULONG ComputeKBytes(APTR a, APTR b)
{
    IPTR result = b - a;

    return (ULONG)(result >> 10);
}

static ULONG GetProcessorsCount()
{
    ULONG count = 0;
    struct TagItem tags [] = 
    {
        {GCIT_NumberOfProcessors, (IPTR)&count},
        {TAG_DONE, TAG_DONE}
    };

    GetCPUInfo(tags);

    return count;
}

struct
{
    ULONG Architecture;
    STRPTR Description;
} ProcessorArchitecture [] =
{
    { PROCESSORARCH_UNKNOWN, "Unknown" },
    { PROCESSORARCH_M68K, "Motorola 68K" },
    { PROCESSORARCH_PPC, "PowerPC" },
    { PROCESSORARCH_X86, "X86" },
    { PROCESSORARCH_ARM, "ARM" },
    { 0, NULL }   
};

struct
{
    ULONG Endianness;
    STRPTR Description;
} CurrentEndianness [] =
{
    { ENDIANNESS_UNKNOWN, "Unknown" },
    { ENDIANNESS_LE, "LE" },
    { ENDIANNESS_BE, "BE" },
    { 0, NULL}
};

static VOID PrintProcessorInformation(char *buffer, LONG bufsize)
{
    ULONG count = GetProcessorsCount();
    ULONG i, j;
    CONST_STRPTR modelstring;
    ULONG architecture, endianness;
    CONST_STRPTR architecturestring = "", endiannessstring = "";
    UQUAD cpuspeed;
    char *bufptr = buffer;
    LONG slen;

    for (i = 0; bufsize > 0 && i < count; i++)
    {
        struct TagItem tags [] =
        {
            {GCIT_SelectedProcessor, i},
            {GCIT_ModelString, (IPTR)&modelstring},
            {GCIT_Architecture, (IPTR)&architecture},
            {GCIT_Endianness, (IPTR)&endianness},
            {GCIT_ProcessorSpeed, (IPTR)&cpuspeed},
            {TAG_DONE, TAG_DONE}
        };
        
        GetCPUInfo(tags);

        j = 0;
        while(ProcessorArchitecture[j].Description != NULL)
        {
            if (ProcessorArchitecture[j].Architecture == architecture)
            {
                architecturestring = ProcessorArchitecture[j].Description;
                break;
            }
            j++;
        }

        j = 0;
        while(CurrentEndianness[j].Description != NULL)
        {
            if (CurrentEndianness[j].Endianness == endianness)
            {
                endiannessstring = CurrentEndianness[j].Description;
                break;
            }
            j++;
        }       

        if (!modelstring)
            modelstring = "Unknown";

        snprintf(bufptr, bufsize, "PROCESSOR %d:\t[%s/%s] %s", (int)(i + 1), architecturestring,
                 endiannessstring, modelstring);
        slen = strlen(bufptr);
        bufptr += slen;
        bufsize -= slen;

        if (bufsize < 10)
            break;

        if (cpuspeed)
        {
            snprintf(bufptr, bufsize, " (%llu MHz)", (unsigned long long)(cpuspeed / 1000000));
            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;
        }
        snprintf(bufptr, bufsize, "\n");
        slen = strlen(bufptr);
        bufptr += slen;
        bufsize -= slen;

        PrintCPUSpecificInfo(bufptr, bufsize, i, ProcessorBase);
    }
}

static inline void VersionStr(char *ptr, int len, struct Library *base)
{
    snprintf(ptr, len, "%d.%d", base->lib_Version, base->lib_Revision);
}    

static Object *ComputerWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    Object *processors_flt;
    Object *hpet_flt;
    Object *ram_flt;
    char aros_ver[8], exec_ver[8];
    IPTR bootldr = 0;
    IPTR args = 0;
    APTR KernelBase;
    APTR HPETBase;

    STRPTR pagetitles[5];
    int pagecnt = 0;

    VersionStr(aros_ver, sizeof(aros_ver), ArosBase);
    VersionStr(exec_ver, sizeof(exec_ver), &SysBase->LibNode);

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
    {
        struct TagItem *bootinfo = KrnGetBootInfo();

        bootldr = GetTagData(KRN_BootLoader, 0, bootinfo);
        args    = GetTagData(KRN_CmdLine   , 0, bootinfo);
    }

    pagetitles[pagecnt++] = (STRPTR)_(MSG_GENERAL);
    if ((ProcessorBase = OpenResource(PROCESSORNAME)) != NULL)
    {
        pagetitles[pagecnt++] = (STRPTR)_(MSG_PROCESSORS);
    }
    pagetitles[pagecnt++] = (STRPTR)_(MSG_RAM);
    if ((HPETBase = OpenResource("hpet.resource")) != NULL)
    {
        pagetitles[pagecnt++] = (STRPTR)_(MSG_HPET);
    }
    pagetitles[pagecnt] = NULL;

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, __(MSG_SYSTEM_PROPERTIES),
        MUIA_Window_ID, MAKE_ID('S', 'Y', 'P', 'R'),
        WindowContents, (IPTR)(RegisterObject,
            MUIA_Register_Titles, (IPTR) pagetitles,
            Child, (IPTR)(VGroup,
                Child, (IPTR)(HGroup,
                    MUIA_FrameTitle, __(MSG_VERSION),
                    GroupFrame,
                    MUIA_Background, MUII_GroupBack,
                    Child, (IPTR)Label("AROS"),
                    Child, (IPTR)(TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, (IPTR)aros_ver,
                    End),
                    Child, (IPTR)Label("Exec"),
                    Child, (IPTR)(TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, (IPTR)exec_ver,
                    End),
                End),
                Child, (IPTR)(HGroup,
                    GroupFrame,
                    MUIA_FrameTitle, __(MSG_BOOTLOADER),
                    MUIA_Background, MUII_GroupBack,
                    Child, (IPTR)(TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, bootldr,
                    End),
                End),
                Child, (IPTR)(HGroup,
                    GroupFrame,
                    MUIA_FrameTitle, __(MSG_ARGUMENTS),
                    MUIA_Background, MUII_GroupBack,
                    Child, (IPTR)(TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, args,
                    End),
                End),
            End),
            ProcessorBase ? Child : TAG_IGNORE, (IPTR)(VGroup,
                Child, (IPTR)(NListviewObject,
                    MUIA_NListview_NList, (IPTR)(processors_flt = NFloattextObject,
                    End),
                End),
            End),
            Child, (IPTR)(VGroup,
                Child, (IPTR)(NListviewObject,
                    MUIA_NListview_NList, (IPTR)(ram_flt = NFloattextObject,
                    End),
                End),
            End),
            HPETBase ? Child : TAG_IGNORE, (IPTR)(VGroup,
                Child, (IPTR)(NListviewObject,
                    MUIA_NListview_NList, (IPTR)(hpet_flt = NFloattextObject,
                    End),
                End),
            End),
        End),
        TAG_DONE
    );

    if (self)
    {
        struct MemHeader *mh;
        char buffer[2000];
        char *bufptr;
        LONG slen;
        LONG bufsize;

        // processors
        *buffer = '\0';
        if (ProcessorBase)
            PrintProcessorInformation(buffer, sizeof(buffer));
        // we intentionally use MUIA_Floattext_Text because it copies the text
        SET(processors_flt, MUIA_Floattext_Text, buffer);

        // high precision timers
        *buffer = '\0';
        bufptr = buffer;
        bufsize = sizeof(buffer);

        if (HPETBase)
        {
            const char *owner;
            ULONG i = 0;

            while (bufsize > 5 && GetUnitAttrs(i, HPET_UNIT_OWNER, &owner, TAG_DONE))
            {
                if (!owner)
                    owner = _(MSG_AVAILABLE);

                snprintf(bufptr, bufsize, "HPET %u:\t\t%s\n", (unsigned)(++i), owner);

                slen = strlen(bufptr);
                bufptr += slen;
                bufsize -= slen;
            }
        }
        // we intentionally use MUIA_Floattext_Text because it copies the text
        SET(hpet_flt, MUIA_Floattext_Text, buffer);

        // RAM
        *buffer = '\0';
        bufptr = buffer;
        bufsize = sizeof(buffer);

        for
        (
            mh = (struct MemHeader *)SysBase->MemList.lh_Head;
            bufsize > 5 && mh->mh_Node.ln_Succ;
            mh = (struct MemHeader *)mh->mh_Node.ln_Succ
        )
        {
            char *memtype = "ROM";

            if (mh->mh_Attributes & MEMF_CHIP)
                memtype = "CHIP";
            if (mh->mh_Attributes & MEMF_FAST)
                memtype = "FAST";

            snprintf(bufptr, bufsize, "Node Type 0x%X, Attributes 0x%X (%s), at $%p-$%p (",
                     mh->mh_Node.ln_Type, mh->mh_Attributes, memtype, mh->mh_Lower, mh->mh_Upper - 1);

            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;

            if (bufsize < 30)
                break;

            PrintNum(bufptr, bufsize, ComputeKBytes(mh->mh_Lower, mh->mh_Upper));
            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;

            snprintf(bufptr, bufsize, ")\n");
            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;
        }
        // we intentionally use MUIA_Floattext_Text because it copies the text
        SET(ram_flt, MUIA_Floattext_Text, buffer);
    }

    return self;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    ComputerWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
