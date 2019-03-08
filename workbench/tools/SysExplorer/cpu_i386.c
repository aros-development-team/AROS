#include <resources/processor.h>
#include <proto/processor.h>

#include <stdio.h>
#include <string.h>

#include "cpuspecific.h"

#ifdef __x86__

#define FLAGS_NUM 14

static const char *features[] =
{
    "FPU",
    "MMX",
    "MMXExt",
    "3DNow!",
    "3DNowExt!",
    "SSE",
    "SSE2",
    "SSE3",
    "SSSE3",
    "SSE4.1",
    "SSE4.2",
    "SSE4A",
    "NoExecute",
    "64Bit"
};

void PrintCPUSpecificInfo(char *buffer, LONG bufsize, ULONG i, APTR ProcessorBase)
{
    LONG slen;
    char *bufptr = buffer;

    BOOL nothing = TRUE;
    BOOL flags[FLAGS_NUM];
    struct TagItem tags [FLAGS_NUM + 2] =
    {
        {GCIT_SelectedProcessor, i},
        {GCIT_SupportsFPU           , (IPTR)&flags[0 ]},
        {GCIT_SupportsMMX           , (IPTR)&flags[1 ]},
        {GCIT_SupportsMMXEXT        , (IPTR)&flags[2 ]},
        {GCIT_Supports3DNOW         , (IPTR)&flags[3 ]},
        {GCIT_Supports3DNOWEXT      , (IPTR)&flags[4 ]},
        {GCIT_SupportsSSE           , (IPTR)&flags[5 ]},
        {GCIT_SupportsSSE2          , (IPTR)&flags[6 ]},
        {GCIT_SupportsSSE3          , (IPTR)&flags[7 ]},
        {GCIT_SupportsSSSE3         , (IPTR)&flags[8 ]},
        {GCIT_SupportsSSE41         , (IPTR)&flags[9 ]},
        {GCIT_SupportsSSE42         , (IPTR)&flags[10]},
        {GCIT_SupportsSSE4A         , (IPTR)&flags[11]},
        {GCIT_SupportsNoExecutionBit, (IPTR)&flags[12]},
        {GCIT_Supports64BitMode     , (IPTR)&flags[13]},
        {TAG_DONE                   , 0               }
    };

    GetCPUInfo(tags);

    for (i = 0; bufsize > 1 && i < FLAGS_NUM; i++)
    {
        if (flags[i])
        {
            nothing = FALSE;
            snprintf(bufptr, bufsize, "%s ", features[i]);
            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;
        }
    }

    if (bufsize > 5)
    {
        if (nothing)
        {
            sprintf(bufptr, "None");
            slen = strlen(bufptr);
            bufptr += slen;
            bufsize -= slen;
        }
        sprintf(bufptr, "\n");
    }
}

#endif
