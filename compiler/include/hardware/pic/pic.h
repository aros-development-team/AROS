/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS PIC/APIC Definitions.
    Lang: english
*/
#ifndef __AROS_PIC_H__
#define __AROS_PIC_H__

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_SEMAPHORES
#   include <exec/semaphores.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif

#define     MAX_IO_APICS                    32

enum PIC_TYPES 
{
    PIC_TYPE_8259                           = 0,
    PIC_TYPE_IOAPIC,
    PIC_TYPE_IOSAPIC,
    PIC_TYPE_COUNT
};

/********** APIC DEFINITIONS ****************/

struct GenericAPIC                                                          /* !! DO NOT USE!! THIS WILL BE REMOVED SOON!! */
{ 
    char                                    *name; 
    IPTR                                    (*probe)(); 
    IPTR                                    (*apic_id_registered)();
};

struct PICBase
{
    struct  Node                            PICB_Node;
    struct  ExecBase                        *PICB_SysBase;
    struct  UtilityBase                     *PICB_UtilBase;

    BOOL                                    PICB_APIC_Enabled;
    int                                     PICB_APIC_IRQ_Model;

    int                                     PICB_APIC_IOAPIC;
    int                                     PICB_APIC_IOAPIC_Count;

    int                                     PICB_APIC_LAPIC;
    APTR                                    PICB_APIC_LAPIC_addr;              /* Local APIC address */
};

#endif /* __AROS_PIC_H__ */
