/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS Generic CPU Definitions.
    Lang: english
*/
#ifndef __AROS_CPU_H__
#define __AROS_CPU_H__

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_SEMAPHORES
#   include <exec/semaphores.h>
#endif

#include <hardware/acpi/acpi.h>
#include <hardware/pic/pic.h>

/* ADJUSTABLE SETTINGS --------------- */

#define     MAX_CPU         32

/* ----------------------------------- */

/* ALL supported processor families should have an entry here (for future compatability - DO NOT CHANGE THE ORDER!) */

enum supported_CPU_families 
{
    CPU_Family_Undefined    = 0,
    CPU_Family_68k          = 1,
    CPU_Family_PPC          = 2,
    CPU_Family_i386         = 3,
};

/* ALL supported processor chips */

enum supported_CPU_chips 
{
    CPU_Undefined           = 0,

    CPU_68k_68000           = 1,
    CPU_68k_68010           = 2,
    CPU_68k_68020           = 3,
    CPU_68k_68030           = 4,
    CPU_68k_68040           = 5,
    CPU_68k_68060           = 6,

    CPU_PPC_                = 100,

    CPU_i386_386            = 200,
    CPU_i386_486            = 201,
    CPU_i386_586            = 202,
    CPU_i386_686            = 203,
    CPU_i386_786            = 204,
    CPU_i386_886            = 205,
};

enum supported_FPU_chips 
{
    FPU_68881               = 0,
    FPU_68882               = 1,
    FPU_68040               = 2,
};

/***********/

struct CPUFam_Definition
{
    struct MinList          CPUF_FamilyList;
    APTR                    CPUF_Name;
    ULONG                   CPUF_FamilyID;
    APTR                    CPUF_Resource;
};

struct CPU_Definition       /* each "processor" in the system is allocated one of these blocks */
{
    struct MinList          CPU_CPUList;                        /* Is there another CPU in this system?                     */
    ULONG                   *CPU_SMPGroup;                      /* Points to this CPUS SMP group (if applicable)            */

    ULONG                   CPU_ID;                             /* ID for processor in the system processor list
                                                                   ID = 0 = listbase (not a cpu - they start at 1)          */
    ULONG                   CPU_Physical;                       /* SMP Physical Processor ID (if applicable)                */
                                                                /* for CPU_ID 0 , this contains the number of processors    */
    ULONG                   CPU_Family;
    ULONG                   CPU_Model;
    
    BOOL                    CPU_Enabled;                        /* Can this CPU be used?                                    */
    BOOL                    CPU_IsOnline;                       /* is this CPU running?                                     */
    BOOL                    CPU_BootCPU;                        /* Was this the CPU that booted the system..                */

    /* The next 4 pointers are used to store processor specific information */

    APTR                    CPU_Private1;                       /* CPU architecture specific control information etc
                                                                    i386 stores i386_compat_intern here....                 */
    APTR                    CPU_Private2;                       /* CPU architecture specific control information etc        */
    APTR                    CPU_Private3;                       /* CPU architecture specific control information etc        */
    APTR                    CPU_Private4;                       /* CPU architecture specific control information etc        */
};

struct SMP_Definition       /* each SMP processor group in the system is allocated one of these blocks */
{
    struct MinList          SMP_SMPList;                        /*  Are there MORE SMP groups!?!?!?                         */
    struct SignalSemaphore  SMP_GrpLock;                        /* Control access to the SMP group..                        */
    ULONG                   SMP_ID;                             /* Which SMP Group is this                                  */
    ULONG                   SMP_CPUCount;                       /* SMP Physical Processor ID (if applicable)                */
    ULONG                   SMP_RecordCount;                    /* No. of records in the SMP config                         */
    ULONG                   SMP_PIC_Mode;
    ULONG                   *SMP_APIC;                          /* Points to the local APIC address                         */
};

/***********/

struct CPUBase
{
    struct  Node                CPUB_Node;
    struct  ExecBase            *CPUB_SysBase;
    struct  UtilityBase         *CPUB_UtilBase;
    struct  ACPIBase            *CPUB_ACPIBase;
    struct  PICBase             *CPUB_PICBase;

    struct SignalSemaphore      CPUB_ListLock;                   /* Control access to the cpu list..                         */

    struct CPUFam_Definition    *CPUB_ProcFamilies;

    struct CPU_Definition       *CPUB_Processors;               /* Lists ALL processors in the system                       */
    struct SMP_Definition       *CPUB_SMP_Groups;               /* Points to a list of SMP groups                           */

    LONG                        CPUB_BOOT_Physical;
    LONG                        CPUB_BOOT_Logical;

    BOOL                        CPUB_SMP_Enabled;
    int                         CPUB_SMP_Config;

};

#endif /* __AROS_CPU_H__ */
