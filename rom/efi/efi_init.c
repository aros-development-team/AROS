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

	/* Install ShutdownA() replacement */
    	SetFunction(&SysBase->LibNode, -173 * LIB_VECTSIZE,
		    AROS_SLIB_ENTRY(ShutdownA, Efi, 173));
    }

    return TRUE;
}

ADD2INITLIB(efi_Init, 0);
