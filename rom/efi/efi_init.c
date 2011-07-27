#include <aros/debug.h>
#include <hardware/efi/efi.h>
#include <resources/efi.h>
#include <proto/arossupport.h>
#include <proto/kernel.h>

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
    struct EFI_SystemTable *efi;

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

    efi = (struct EFI_SystemTable *)tag->ti_Data;
    D(bug("Found EFI system table at 0x%p\n", efi));

    if (!CheckTable(&efi->Hdr, EFI_SYSTEM_TABLE_SIGNATURE))
    {
    	D(bug("[EFI] System table broken\n"));
    	return FALSE;
    }

    if (CheckTable(&efi->RuntimeServices->Hdr, EFI_RUNTIME_SERVICES_SIGNATURE))
    {
    	D(bug("[EFI] Valid runtime services table at 0x%p\n", efi->RuntimeServices));

    	EFIBase->Runtime = efi->RuntimeServices;
    }

    return TRUE;
}

ADD2INITLIB(efi_Init, 0);
