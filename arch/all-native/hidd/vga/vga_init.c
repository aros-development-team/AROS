/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: vga gfx Hidd for standalone AROS
    Lang: english
*/

#define __OOP_NOATTRBASES__

#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <proto/acpica.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "vga.h"
#include "vgaclass.h"

#include LC_LIBDEFS_FILE

extern struct vgaModeDesc vgaDefMode[];

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

/* ACPICABase is optional */
struct Library *ACPICABase = NULL;

OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddChunkyBMAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddVGABitMapAB;

static struct OOP_ABDescr abd[] = 
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase },
    { IID_Hidd_ChunkyBM,	&HiddChunkyBMAttrBase },
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase },
    { IID_Hidd_Gfx,		&HiddGfxAttrBase },
    { IID_Hidd_Sync,		&HiddSyncAttrBase },
    /* Private bases */
    { IID_Hidd_VGABitMap,	&HiddVGABitMapAB },
    { NULL, NULL }
};

static int PCVGA_Init(LIBBASETYPEPTR LIBBASE)
{
    struct GfxBase *GfxBase;
    struct vga_staticdata *xsd = &LIBBASE->vsd;
    struct vgaModeEntry *entry;
    BOOL res = FALSE;
    int i;

    /* We are not compatible with VESA driver */
    if (OOP_FindClass("hidd.gfx.vesa"))
    {
	D(bug("[VGA] VESA driver found, not initializing VGA\n"));
	return FALSE;
    }

    if ((ACPICABase = OpenLibrary("acpica.library", 0)))
    {
        ACPI_TABLE_FADT *fadt;
        
        if ((AcpiGetTable("FACP", 1, (ACPI_TABLE_HEADER **)&fadt) == AE_OK) &&
            (fadt->BootFlags & ACPI_FADT_NO_VGA))
        {
            D(bug("[VGA] Disabled by ACPI\n"));
            CloseLibrary(ACPICABase);
            ACPICABase = NULL;
            return FALSE;
        }
        CloseLibrary(ACPICABase);
        ACPICABase = NULL;
    }

    InitSemaphore(&xsd->sema);
    InitSemaphore(&xsd->HW_acc);
    NEWLIST(&xsd->modelist);

    if (!OOP_ObtainAttrBases(abd))
	return FALSE;
    
    /* Insert default videomodes */
	
    for (i=0; i < NUM_MODES; i++)
    {
	entry = AllocMem(sizeof(struct vgaModeEntry),MEMF_CLEAR|MEMF_PUBLIC);
	if (entry)
	{
	    entry->Desc=&(vgaDefMode[i]);
	    ADDHEAD(&xsd->modelist,entry);
	    D(bug("Added default mode: %s\n", entry->Desc->name));
	}
    }

    D(bug("[VGA] Init: Everything OK, installing driver\n"));

    /*
     * Open graphics.library ourselves because we will close it
     * after adding the driver.
     * Autoinit code would close it only upon driver expunge.
     */
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    if (!GfxBase)
    {
	D(bug("[VGA] Failed to open graphics.library!\n"));

	return FALSE;
    }

    /* 
     * It is unknown (and no way to know) what hardware part this driver uses.
     * In order to avoid conflicts with disk-based native-mode hardware
     * drivers it needs to be removed from the system when some other driver
     * is installed.
     * This is done by graphics.library if DDRV_BootMode is set to TRUE.
     */
    i = AddDisplayDriver(xsd->vgaclass, NULL, DDRV_BootMode, TRUE, TAG_DONE);

    D(bug("[VGA] AddDisplayDriver() result: %u\n", i));
    if (!i)
    {
	/* We use ourselves, and no one else does */
    	LIBBASE->library.lib_OpenCnt = 1;
    	res = TRUE;
    }

    CloseLibrary(&GfxBase->LibNode);
    return res;
}

ADD2INITLIB(PCVGA_Init, 0)
