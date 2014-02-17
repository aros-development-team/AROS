/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot AROS
    Lang: english
*/

#include <string.h>
#include <stdlib.h>

#include <aros/debug.h>
#include <exec/alerts.h>
#include <aros/asmcall.h>
#include <aros/bootloader.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>
#include <devices/bootblock.h>
#include <devices/timer.h>
#include <dos/dosextens.h>
#include <resources/filesysres.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/partition.h>
#include <proto/bootloader.h>
#include <proto/dos.h>

#include LC_LIBDEFS_FILE

#include "dosboot_intern.h"
#include "../expansion/expansion_intern.h"

#ifdef __mc68000

/* These two functions are implemented in arch/m68k/all/dosboot/bootcode.c */

extern VOID_FUNC CallBootBlockCode(APTR bootcode, struct IOStdReq *io, struct ExpansionBase *ExpansionBase);
extern void dosboot_BootPoint(struct BootNode *bn);

#else

#define CallBootBlockCode(bootcode, io, ExpansionBase) NULL
#define dosboot_BootPoint(bn)

#endif

static BOOL GetBootNodeDeviceUnit(struct BootNode *bn, BPTR *device, IPTR *unit, ULONG *bootblocks)
{
    struct DeviceNode *dn;
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *de;

    if (bn == NULL)
        return FALSE;

    dn = bn->bn_DeviceNode;
    if (dn == NULL)
        return FALSE;

    fssm = BADDR(dn->dn_Startup);
    if (fssm == NULL)
        return FALSE;

    *unit = fssm->fssm_Unit;
    *device = fssm->fssm_Device;

    de = BADDR(fssm->fssm_Environ);
    /* Following check from Guru Book */
    if (de == NULL || (de->de_TableSize & 0xffffff00) != 0 || de->de_TableSize < DE_BOOTBLOCKS)
    	return FALSE;
    *bootblocks = de->de_BootBlocks * de->de_SizeBlock * sizeof(ULONG);
    if (*bootblocks == 0)
    	return FALSE;
    return TRUE;
}

static BOOL BootBlockCheckSum(UBYTE *bootblock, ULONG bootblock_size)
{
    ULONG crc = 0, crc2 = 0;
    UWORD i;

    for (i = 0; i < bootblock_size; i += 4) {
	ULONG v = AROS_LONG2BE(*(ULONG*)(bootblock + i));
	if (i == 4) {
	    crc2 = v;
	    v = 0;
	}
	if (crc + v < crc)
	    crc++;
	crc += v;
    }
    crc ^= 0xffffffff;
    D(bug("[Strap] bootblock %08x checksum %s (%08x %08x)\n",
	AROS_LONG2BE(*(ULONG*)bootblock), crc == crc2 ? "ok" : "error", crc, crc2));
    return crc == crc2;
}

static BOOL BootBlockCheck(UBYTE *bootblock, ULONG bootblock_size)
{
    struct FileSysResource *fsr;
    struct FileSysEntry *fse;
    ULONG dostype;

    if (!BootBlockCheckSum(bootblock, bootblock_size))
    	return FALSE;
    if (!(fsr = OpenResource("FileSystem.resource")))
	return FALSE;
    dostype = AROS_LONG2BE(*(ULONG*)bootblock);
    ForeachNode(&fsr->fsr_FileSysEntries, fse) {
	if (fse->fse_DosType == dostype)
	    return TRUE;
    }
    D(bug("[Strap] unknown bootblock dostype %08x\n", dostype));
    return FALSE;
}

static inline void SetBootNodeDosType(struct BootNode *bn, ULONG dostype)
{
    struct DeviceNode *dn;
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *de;

    dn = bn->bn_DeviceNode;
    if (dn == NULL)
        return;

    fssm = BADDR(dn->dn_Startup);
    if (fssm == NULL)
        return;

    de = BADDR(fssm->fssm_Environ);
    if (de == NULL || de->de_TableSize < DE_DOSTYPE)
        return;

    de->de_DosType = dostype;
}

/* Returns TRUE if it was a BootBlock style, but couldn't
 * be booted, FALSE if not a BootBlock style, and doesn't
 * return at all on a successful boot.
 */
static BOOL dosboot_BootBlock(struct BootNode *bn, struct ExpansionBase *ExpansionBase)
{
    ULONG bootblock_size;
    struct MsgPort *msgport;
    struct IOStdReq *io;
    BPTR device;
    IPTR unit;
    VOID_FUNC init = NULL;
    UBYTE *buffer;

    if (!GetBootNodeDeviceUnit(bn, &device, &unit, &bootblock_size))
        return FALSE;

    D(bug("%s: Probing for boot block on %b.%d\n", __func__, device, unit));
    /* memf_chip not required but more compatible with old bootblocks */
    buffer = AllocMem(bootblock_size, MEMF_CHIP);
    if (buffer != NULL)
    {
       D(bug("[Strap] bootblock address %p\n", buffer));
       if ((msgport = CreateMsgPort()))
       {
           if ((io = CreateIORequest(msgport, sizeof(struct IOStdReq))))
           {
               if (!OpenDevice(AROS_BSTR_ADDR(device), unit, (struct IORequest*)io, 0))
               {
                   /* Read the device's boot block */
                   io->io_Length = bootblock_size;
                   io->io_Data = buffer;
                   io->io_Offset = 0;
                   io->io_Command = CMD_READ;
                   D(bug("[Strap] %b.%d bootblock read (%d bytes)\n", device, unit, bootblock_size));
                   DoIO((struct IORequest*)io);

                   if (io->io_Error == 0)
                   {
                       D(bug("[Strap] %b.%d bootblock read to %p ok\n", device, unit, buffer));
                       if (BootBlockCheck(buffer, bootblock_size))
                       {
                           SetBootNodeDosType(bn, AROS_LONG2BE(*(LONG *)buffer));
                           CacheClearE(buffer, bootblock_size, CACRF_ClearI|CACRF_ClearD);
                           init = CallBootBlockCode(buffer + 12, io, ExpansionBase);
                       }
                       else
                       {
                           D(bug("[Strap] Not a valid bootblock\n"));
                       }
                   } else {
                       D(bug("[Strap] io_Error %d\n", io->io_Error));
                   }
                   io->io_Command = TD_MOTOR;
                   io->io_Length = 0;
                   DoIO((struct IORequest*)io);
                   CloseDevice((struct IORequest *)io);
               }
               DeleteIORequest((struct IORequest*)io);
           }
           DeleteMsgPort(msgport);
       }
       FreeMem(buffer, bootblock_size);
   }

   if (init != NULL)
   {
       D(bug("calling bootblock loaded code at %p\n", init));

	/*
	 * This is actually rt_Init calling convention for non-autoinit residents.
	 * Workbench floppy bootblocks return a pointer to dos.library init routine,
	 * and it needs SysBase in A6.
	 * We don't close boot screen and libraries here. We will close them after
	 * dos.library is succesfully initialized, using a second RTF_AFTERDOS ROMTag.
	 * This is needed because dos.library contains the second part of "bootable"
	 * test, trying to mount a filesystem and read the volume.
	 * We hope it won't do any harm for NDOS game disks.
	 */
       AROS_UFC3NR(void, init,
       		 AROS_UFCA(APTR, NULL, D0),
       		 AROS_UFCA(BPTR, BNULL, A0),
       		 AROS_UFCA(struct ExecBase *, SysBase, A6));
   }

#ifdef __mc68000
   /* Device *was* BootBlock style, but couldn't boot. */
   return TRUE;
#else
   /* Device *was* BootBlock style, but couldn't boot.
    * Non-m68k will try as DOS Boot anyway!
    */
   return FALSE;
#endif
}

/* Attempt to boot via dos.library directly
 */
static inline void dosboot_BootDos(void)
{
    struct Resident *DOSResident;

    /* Initialize dos.library manually. This is what Workbench floppy bootblocks do. */
    DOSResident = FindResident( "dos.library" );

    if( DOSResident == NULL )
    {
        Alert( AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_DOSLib );
    }

    /* InitResident() of dos.library will not return on success. */
    InitResident( DOSResident, BNULL );
}


/* Attempt to boot, first from the BootNode boot blocks,
 * then via the DOS handlers
 */
LONG dosboot_BootStrap(LIBBASETYPEPTR LIBBASE)
{
    struct ExpansionBase *ExpansionBase = LIBBASE->bm_ExpansionBase;
    struct BootNode *bn;
    int i, nodes;

    /*
     * Try to boot from any device in the boot list,
     * highest priority first.
     */
    ListLength(&ExpansionBase->MountList, nodes);
    for (i = 0; i < nodes; i++)
    {
        bn = (struct BootNode *)GetHead(&ExpansionBase->MountList);

        if (bn->bn_Node.ln_Type != NT_BOOTNODE ||
            bn->bn_Node.ln_Pri <= -128 ||
            bn->bn_DeviceNode == NULL)
        {
            D(bug("%s: Ignoring %p, not a bootable node\n", __func__, bn));
            ObtainSemaphore(&IntExpBase(ExpansionBase)->BootSemaphore);
            REMOVE(bn);
            ADDTAIL(&ExpansionBase->MountList, bn);
            ReleaseSemaphore(&IntExpBase(ExpansionBase)->BootSemaphore);
            continue;
        }

        /* For each attempt, this node is at the head
         * of the MountList, so that DOS will try to
         * use it as SYS: if the strap works
         */

        /* First try as a BootBlock.
         * dosboot_BootBlock returns TRUE if it *was*
         * a BootBlock device, but couldn't be booted.
         * Returns FALSE if not a bootblock device,
         * and doesn't return at all if the bootblock
         * was successful.
         */
        D(bug("%s: Attempting %b as BootBlock\n",__func__, ((struct DeviceNode *)bn->bn_DeviceNode)->dn_Name));
        if (!dosboot_BootBlock(bn, ExpansionBase)) {
            /* Then as a BootPoint node */
            D(bug("%s: Attempting %b as BootPoint\n", __func__, ((struct DeviceNode *)bn->bn_DeviceNode)->dn_Name));
            dosboot_BootPoint(bn);

            /* And finally with DOS */
            D(bug("%s: Attempting %b with DOS\n", __func__, ((struct DeviceNode *)bn->bn_DeviceNode)->dn_Name));
            dosboot_BootDos();
        }

        /* Didn't work. Next! */
        D(bug("%s: DeviceNode %b (%d) was not bootable\n", __func__,
                    ((struct DeviceNode *)bn->bn_DeviceNode)->dn_Name,
                    bn->bn_Node.ln_Pri));

        ObtainSemaphore(&IntExpBase(ExpansionBase)->BootSemaphore);
        REMOVE(bn);
        ADDTAIL(&ExpansionBase->MountList, bn);
        ReleaseSemaphore(&IntExpBase(ExpansionBase)->BootSemaphore);
    }

    D(bug("%s: No BootBlock, BootPoint, or BootDos nodes found\n",__func__));

    /* At this point we now know that we were unable to
     * strap any bootable devices.
     */

    return ERROR_NO_DISK;
}
