/*
    Copyright (C) 2013-2019, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

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

static VOID ParseProcessorInformation(Object *GrpProcessors)
{
    ULONG count = GetProcessorsCount();
    ULONG i, j;
    CONST_STRPTR modelstring;
    ULONG architecture, endianness;
    CONST_STRPTR architecturestring = "", endiannessstring = "";
    UQUAD cpuspeed;

    D(bug("[SysExplorer] %s()\n", __func__));

    for (i = 0; i < count; i++)
    {
        Object  *CoreIDLabelObj, *CoreIDStrObj,
                *CoreSpdLabelObj, *CoreSpdStrObj,
                *CoreFeatLabelObj, *CoreFeatStrObj;

        char    *CoreIDLabelStr, *CoreIDStr;

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
        D(bug("[SysExplorer] %s: CPU #%d\n", __func__, i));

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

        CoreIDLabelStr = AllocVec(14, MEMF_PUBLIC);
        snprintf(CoreIDLabelStr, 14, "CPU Core #%u", (int)(i + 1));
        CoreIDLabelObj = Label(CoreIDLabelStr);

        CoreIDStr = AllocVec(strlen(architecturestring) + strlen(endiannessstring) + strlen(modelstring) + 4, MEMF_PUBLIC);
        snprintf(CoreIDStr,
                strlen(architecturestring) + strlen(endiannessstring) + strlen(modelstring) + 4,
                "%s/%s %s",
                architecturestring, endiannessstring, modelstring);
        CoreIDStrObj = TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, (IPTR)CoreIDStr,
            End;
        
        if (DoMethod(GrpProcessors, MUIM_Group_InitChange))
        {
            DoMethod(GrpProcessors, OM_ADDMEMBER, CoreIDLabelObj);
            DoMethod(GrpProcessors, OM_ADDMEMBER, CoreIDStrObj);
            DoMethod(GrpProcessors, MUIM_Group_ExitChange);
        }

        if (cpuspeed)
        {
            char *CoreSpdStr;
            CoreSpdLabelObj = Label("Speed");
            CoreSpdStr = AllocVec(20, MEMF_PUBLIC);
            snprintf(CoreSpdStr, 20, "%llu MHz", (unsigned long long)(cpuspeed / 1000000));
            CoreSpdStrObj = TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)CoreSpdStr,
                End;

            if (DoMethod(GrpProcessors, MUIM_Group_InitChange))
            {
                DoMethod(GrpProcessors, OM_ADDMEMBER, CoreSpdLabelObj);
                DoMethod(GrpProcessors, OM_ADDMEMBER, CoreSpdStrObj);
                DoMethod(GrpProcessors, MUIM_Group_ExitChange);
            }
        }

        char *CPUFeatureStr = AllocVec(1024, MEMF_PUBLIC);
        PrintCPUSpecificInfo(CPUFeatureStr, 1024, i, ProcessorBase);
        CoreFeatLabelObj = Label("Features");
        CoreFeatStrObj = ScrollgroupObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_Scrollgroup_Contents, (IPTR)(NFloattextObject,
                    NoFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Floattext_Text, (IPTR)CPUFeatureStr,
                End),
            End;
        if (DoMethod(GrpProcessors, MUIM_Group_InitChange))
        {
            DoMethod(GrpProcessors, OM_ADDMEMBER, CoreFeatLabelObj);
            DoMethod(GrpProcessors, OM_ADDMEMBER, CoreFeatStrObj);
            DoMethod(GrpProcessors, MUIM_Group_ExitChange);
        }
    }
}

static inline void VersionStr(char *ptr, int len, struct Library *base)
{
    snprintf(ptr, len, "%d.%d", base->lib_Version, base->lib_Revision);
}    

char *SplitBootArgs(struct TagItem *bootinfo, char *buffer, LONG bufsize)
{
    char *rawargs = (char *)GetTagData(KRN_CmdLine, 0, bootinfo);
    int i, count;

    D(bug("[SysExplorer] %s()\n", __func__));

    if (rawargs)
    {
        D(bug("[SysExplorer] %s: splitting '%s'\n", __func__, rawargs));
        count = strlen(rawargs);
        if (count > bufsize)
            count = bufsize;
        for (i = 0; i < count ; i++)
        {
            if (rawargs[i] == ' ')
                buffer[i] = '\n';
            else
                buffer[i] = rawargs[i];
        }
    }
    return buffer;
}

static Object *ComputerWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    Object *GrpProcessors;
    Object *hpet_flt;
    Object *ram_flt;
    char aros_ver[12], exec_ver[12];
    char buffer[2000];
    IPTR bootldr = 0;
    IPTR args = 0;
    APTR KernelBase;
    APTR HPETBase;

    STRPTR pagetitles[5];
    int pagecnt = 0;

    D(bug("[SysExplorer] %s()\n", __func__));

    VersionStr(aros_ver, sizeof(aros_ver), ArosBase);
    VersionStr(exec_ver, sizeof(exec_ver), &SysBase->LibNode);

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
    {
        struct TagItem *bootinfo = KrnGetBootInfo();

        bootldr = GetTagData(KRN_BootLoader, 0, bootinfo);
        args    = (IPTR)SplitBootArgs(bootinfo, buffer, sizeof(buffer));
    }

    D(bug("[SysExplorer] %s: prepairing pages ..\n", __func__));

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

    D(bug("[SysExplorer] %s: %d pages\n", __func__, pagecnt));

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, __(MSG_SYSTEM_PROPERTIES),
        MUIA_Window_ID, MAKE_ID('S', 'Y', 'P', 'R'),
        WindowContents, (IPTR)(RegisterObject,
            MUIA_Register_Titles, (IPTR) pagetitles,
            MUIA_CycleChain, 1,
            Child, (IPTR)(VGroup,
                Child, (IPTR)(ColGroup(2),
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
                    Child, (IPTR)Label("Exec Library"),
                    Child, (IPTR)(TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, (IPTR)exec_ver,
                    End),
                End),
                Child, (IPTR)(ColGroup(2),
                    GroupFrame,
                    MUIA_FrameTitle, (IPTR)"Boot Config",
                    Child, (IPTR)Label("Loader"),
                    Child, (IPTR)(TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_CycleChain, 1,
                        MUIA_Text_Contents, bootldr,
                    End),
                    Child, (IPTR)Label(__(MSG_ARGUMENTS)),
                    Child, (IPTR)(ScrollgroupObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Scrollgroup_Contents, (IPTR)(NFloattextObject,
                            NoFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_CycleChain, 1,
                            MUIA_Floattext_Text, args,
                        End),
                    End),
                End),
            End),
            ProcessorBase ? Child : TAG_IGNORE, (IPTR)(HGroup,
                Child, (IPTR)(GrpProcessors = ColGroup(2),
                End),
            End),
            Child, (IPTR)(VGroup,
                Child, (IPTR)(NListviewObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_NListview_NList, (IPTR)(ram_flt = NFloattextObject,
                        NoFrame,
                        MUIA_Background, MUII_TextBack,
                    End),
                End),
            End),
            HPETBase ? Child : TAG_IGNORE, (IPTR)(VGroup,
                Child, (IPTR)(NListviewObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_NListview_NList, (IPTR)(hpet_flt = NFloattextObject,
                        NoFrame,
                        MUIA_Background, MUII_TextBack,
                    End),
                End),
            End),
        End),
        TAG_DONE
    );

    if (self)
    {
        struct MemHeader *mh;
        char *bufptr;
        LONG slen;
        LONG bufsize;

        D(bug("[SysExplorer] %s: self @ %p\n", __func__, self));

        // processors
        *buffer = '\0';
        if (ProcessorBase)
            ParseProcessorInformation(GrpProcessors);

        D(bug("[SysExplorer] %s: Processor Information read\n", __func__));

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
