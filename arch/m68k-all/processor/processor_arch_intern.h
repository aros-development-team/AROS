/*
    Copyright © 2011-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROCESSOR_ARCH_INTERN_H
#define PROCESSOR_ARCH_INTERN_H

#include <exec/types.h>

#define MODELSTRINGMAX                  64

struct M68KProcessorInformation
{
    ULONG                               CPUModel;
    ULONG                               FPUModel;

    /* Processor cache */
    ULONG                               L1DataCacheSize;
    ULONG                               L1InstructionCacheSize;

    /* Frequency information */
    UQUAD                               CPUFrequency;
    UQUAD                               BusFrequency;

    STRPTR                              ModelString;
    TEXT                                ModelStringBuffer[MODELSTRINGMAX];
};

struct SystemProcessors
{
    struct M68KProcessorInformation     processor;
};

#define FPUMODEL_UNKNOWN                0
#define FPUMODEL_NONE                   1
#define FPUMODEL_68881                  2
#define FPUMODEL_68882                  3
#define FPUMODEL_INTERNAL               4

#define CPUMODEL_UNKNOWN                0
#define CPUMODEL_68000                  1
#define CPUMODEL_68010                  2
#define CPUMODEL_68020                  3
#define CPUMODEL_68030                  4
#define CPUMODEL_68040                  5
#define CPUMODEL_68060                  6
#define CPUMODEL_68080                  7    /* Apollo 'AC68080' */

#endif /* PROCESSOR_ARCH_INTERN_H */
