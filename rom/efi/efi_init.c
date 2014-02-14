/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <resources/efi.h>
#include <proto/arossupport.h>
#include <proto/kernel.h>

#include "efi_intern.h"

static BOOL CheckTable(struct EFI_TableHeader *t, UQUAD sig)
{
    if (t->Signature != sig)
    {
        D(bug("[EFI] Table 0x%p bad signature (has 0x%016llX, wanted 0x%016llX)\n", t, t->Signature, sig));
        return FALSE;
    }

    /* TODO: Check CRC */
    
    return TRUE;
}

AROS_INTH1(static ResetHandler, struct EFIBase *, EFIBase)
{
    AROS_INTFUNC_INIT

    UBYTE action = EFIBase->reset_handler.is_Node.ln_Type;
    IPTR efiAction;

    switch (action)
    {
    case SD_ACTION_COLDREBOOT:
    	efiAction = EFI_Reset_Cold;
    	break;

    case SD_ACTION_POWEROFF:
    	efiAction = EFI_Reset_Shutdown;
    	break;

    default:
    	/* Unknown action */
    	return FALSE;
    }

    /* Use EFI runtime services to perform the action */
    EFIBase->Runtime->ResetSystem(efiAction, 0, 0, NULL);

    /* Shut up the compiler, we should never reach this. */
    return FALSE;

    AROS_INTFUNC_EXIT
}

static int efi_Init(struct EFIBase *EFIBase)
{
    APTR KernelBase;
    struct TagItem *tag;

    D(bug("[EFI] Entered efi_Init() at 0x%p\n", efi_Init));

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    {
    	return FALSE;
    }

    tag = LibFindTagItem(KRN_EFISystemTable, KrnGetBootInfo());
    if (!tag)
    {
    	D(bug("[EFI] No EFI system table from the bootstrap!\n"));

    	return FALSE;
    }

    EFIBase->System = (struct EFI_SystemTable *)tag->ti_Data;
    D(bug("Found EFI system table at 0x%p\n", EFIBase->System));

    if (!CheckTable(&EFIBase->System->Hdr, EFI_SYSTEM_TABLE_SIGNATURE))
    {
    	D(bug("[EFI] System table broken\n"));
    	return FALSE;
    }

    if (CheckTable(&EFIBase->System->RuntimeServices->Hdr, EFI_RUNTIME_SERVICES_SIGNATURE))
    {
    	EFIBase->Runtime = EFIBase->System->RuntimeServices;
    	D(bug("[EFI] Valid runtime services table at 0x%p\n", EFIBase->Runtime));

        /* Install EFI reset/power-off mechanism */
        EFIBase->reset_handler.is_Node.ln_Pri = -56;
        EFIBase->reset_handler.is_Node.ln_Name = "EFI reset";
        EFIBase->reset_handler.is_Code = (VOID_FUNC)ResetHandler;
        EFIBase->reset_handler.is_Data = EFIBase;
        AddResetCallback(&EFIBase->reset_handler);
    }

    return TRUE;
}

ADD2INITLIB(efi_Init, 0);
