/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
 
static BOOL BootBlockChecksum(UBYTE *bootblock, ULONG bootblock_size)
{
       ULONG crc = 0, crc2 = 0;
       UWORD i;
       for (i = 0; i < bootblock_size; i += 4) {
           ULONG v = (bootblock[i] << 24) | (bootblock[i + 1] << 16) |
(bootblock[i + 2] << 8) | bootblock[i + 3];
           if (i == 4) {
               crc2 = v;
               v = 0;
           }
           if (crc + v < crc)
               crc++;
           crc += v;
       }
       crc ^= 0xffffffff;
       D(bug("[Strap] bootblock checksum %s (%08x %08x)\n", crc == crc2 ? "ok" : "error", crc, crc2));
       return crc == crc2;
}


/* Execute the code in the boot block.
 * This can be custom defined for your architecture.
 *
 * Returns 0 on success, or an error code
 */
static LONG CallBootBlockCode(APTR bootcode, struct IOStdReq *io, VOID_FUNC *initcode)
{
    LONG retval;
    VOID_FUNC init;
#ifdef __mc68000
    /* Lovely. Double return values. What func. */
    asm volatile (
                 "move.l %2,%%a1\n"
                 "move.l %4,%%a0\n"
                 "move.l %%a6,%%sp@-\n"
                 "move.l %3,%%a6\n"
                 "jsr.l (%%a0)\n"
                 "move.l %%sp@+,%%a6\n"
                 "move.l %%d0,%0\n"
                 "move.l %%a0,%1\n"
                 : "=m" (retval), "=m" (init)
                 : "m" (io), "r" (SysBase),
                   "m" (bootcode)
                 : "%d0", "%d1", "%a0", "%a1");
    D(bug("bootblock: D0=0x%08x A0=%p\n", retval, init));
#else
    /* A more architecture independent way of doing things.
     */
    if (0) { /* Disabled for now */
        retval = AROS_UFC3(ULONG, bootcode,
                            AROS_UFCA(VOID_FUNC *,    &init, A0),
                            AROS_UFCA(struct IOStdReq *, io, A1),
                            AROS_UFCA(struct ExecBase *, SysBase, A6));
    } else {
        retval = ERROR_INVALID_RESIDENT_LIBRARY;
        init = NULL;
    }
#endif
    *initcode = init;
    return retval;
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

static void dosboot_BootBlock(struct BootNode *bn, struct ExpansionBase *ExpansionBase)
{
    ULONG bootblock_size;
    struct MsgPort *msgport;
    struct IOStdReq *io;
    BPTR device;
    IPTR unit;
    LONG retval;
    VOID_FUNC init = NULL;
    UBYTE *buffer;

    if (!GetBootNodeDeviceUnit(bn, &device, &unit, &bootblock_size))
        return;

    D(bug("%s: Probing for boot block on %s %p\n", __func__, device, (APTR)unit));
    /* memf_chip not required but more compatible with old bootblocks */
    buffer = AllocMem(bootblock_size, MEMF_CHIP);
    if (buffer != NULL) {
       D(bug("[Strap] bootblock address %p\n", buffer));
       if ((msgport = CreateMsgPort())) {
           if ((io = CreateIORequest(msgport, sizeof(struct IOStdReq)))) {
               if (!OpenDevice(AROS_BSTR_ADDR(device), unit, (struct IORequest*)io, 0)) {
                   /* Read the device's boot block
                    */
                   io->io_Length = bootblock_size;
                   io->io_Data = buffer;
                   io->io_Offset = 0;
                   io->io_Command = CMD_READ;
                   D(bug("[Strap] %b.%d bootblock read (%d bytes)\n", device, unit, bootblock_size));
                   DoIO((struct IORequest*)io);
                   if (io->io_Error == 0) {
                       D(bug("[Strap] %b.%d bootblock read to %p ok\n", device, unit, buffer));
                       if (BootBlockChecksum(buffer, bootblock_size)) {
                           APTR bootcode = buffer + 12;
                           UBYTE oldflags = ExpansionBase->Flags & EBF_SILENTSTART;

                           ExpansionBase->Flags &= ~EBF_SILENTSTART;

                           SetBootNodeDosType(bn, AROS_LONG2BE(*(LONG *)buffer));

                           D(bug("[Strap] Calling bootblock!\n", buffer));
                           retval = CallBootBlockCode(bootcode, io, &init);
                           if (retval != 0) {
                               init = NULL;
                               ExpansionBase->Flags |= oldflags;

                               D(bug("[Strap] Boot block failed to boot.\n"));
                           }
                       } else {
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

   if (init != NULL) {
       CloseLibrary((APTR)ExpansionBase);
       D(bug("calling bootblock loaded code at %p\n", init));
       init();
   }
}

/* BootPoint booting, as describe in the Amiga Devices Manual
 *
 * Only expected to work on m68k.
 */
static void dosboot_BootPoint(struct BootNode *bn)
{
#ifdef __mc68000
    struct DeviceNode *dn;
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *de;
    IPTR bootblocks;

    dn = bn->bn_DeviceNode;
    if (dn == NULL || dn->dn_Name == BNULL)
        return;

    fssm = BADDR(dn->dn_Startup);
    if (fssm == NULL)
        return;

    de = BADDR(fssm->fssm_Environ);
    if (de == NULL)
        return;

    bootblocks = (de->de_TableSize < DE_BOOTBLOCKS) ? 0 : de->de_BootBlocks;

    /* BootPoint nodes */
    if (bootblocks == 0 && bn->bn_Node.ln_Name != NULL) {
        struct ConfigDev *cd = (APTR)bn->bn_Node.ln_Name;
        if (cd->cd_Rom.er_DiagArea != NULL) {
            struct DiagArea *da = cd->cd_Rom.er_DiagArea;
            if (da->da_Config & DAC_CONFIGTIME) {
                /* Yes, it's actually a BootPoint node */
                void *func = (APTR)(((IPTR)da) + da->da_BootPoint);

                D(bug("dosboot_BootStrap: Calling %b BootPoint @%p\n", dn->dn_Name, func));
                /* Yet another crazy Amiga calling sequence.
                 * The ConfigDev is pushed on the stack, but
                 * the BootNode is in A2. Joy.
                 */
                asm volatile (
                        "move.l %0,%%a0\n"
                        "move.l %1,%%a1\n"
                        "move.l %2,%%a2\n"
                        "move.l %%a1,%%sp@-\n"
                        "jsr    %%a0@\n"
                        "addq.l #4,%%sp\n"
                        :
                        : "d" (func), "d" (cd), "d" (bn)
                        : "d0", "d1", "a0", "a1", "a2"
                        );
            }
        }
    }
#endif
}

/* Attempt to boot via dos.library directly
 */
static void dosboot_BootDos(struct BootNode *bn)
{
    APTR DOSBase;
    struct Resident *DOSResident;

    /* First, see if previous attempt was able to initialize
     * dos.library, but dos.library failed to find a
     * bootable paritition.
     */
    DOSBase = OpenLibrary("dos.library", 0);
    if (DOSBase) {
        /* Since DOSBase is now (mostly) set up,
         * just call CliInit(), which will try
         * to boot off of this node.
         */
        if (CliInit(NULL) == RETURN_OK)
            RemTask(NULL);
    } else {
        /*
         * Initialize dos.library manually.
         */
        DOSResident = FindResident( "dos.library" );

        if( DOSResident == NULL )
        {
            Alert( AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_DOSLib );
        }

        /* InitResident() of dos.library will not return 
         * on success.
         */
        InitResident( DOSResident, BNULL );
    }
}


/* Attempt to boot, first from the BootNode boot blocks,
 * then via the DOS handlers
 */
LONG dosboot_BootStrap(LIBBASETYPEPTR LIBBASE)
{
    struct BootNode *bn;
    struct ExpansionBase *ExpansionBase;
    int i, nodes;

    ExpansionBase = (APTR)OpenLibrary("expansion.library", 0);
    if (ExpansionBase == NULL)
        Alert( AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_ExpansionLib );

    /*
     * Try to boot from any device in the boot list,
     * highest priority first.
     */
    ListLength(&ExpansionBase->MountList, nodes);
    for (i = 0; i < nodes; i++) {
        bn = (struct BootNode *)GetHead(&ExpansionBase->MountList);

        if (bn->bn_Node.ln_Type != NT_BOOTNODE ||
            bn->bn_Node.ln_Pri <= -128 ||
            bn->bn_DeviceNode == NULL) {
            D(bug("%s: Ignoring %p, not a bootable node\n", __func__, bn));
            REMOVE(bn);
            ADDTAIL(&ExpansionBase->MountList, bn);
            continue;
        }

        /* For each attempt, this node is at the head
         * of the MountList, so that DOS will try to
         * use it as SYS: if the strap works
         */

        /* First try as a BootPoint node */
        dosboot_BootPoint(bn);

        /* Then as a BootBlock */
        dosboot_BootBlock(bn, ExpansionBase);

        /* And finally with DOS */
        dosboot_BootDos(bn);

        /* Didn't work. Next! */
        D(bug("%s: DeviceNode %b (%d) was not bootable\n", __func__,
                    ((struct DeviceNode *)bn->bn_DeviceNode)->dn_Name,
                    bn->bn_Node.ln_Pri));

        REMOVE(bn);
        ADDTAIL(&ExpansionBase->MountList, bn);
    }

    CloseLibrary((APTR)ExpansionBase);

    D(bug("%s: No BootBlock, BootPoint, or BootDos nodes found\n",__func__));

    /* At this point we now know that we were unable to
     * strap any bootable devices.
     */

    return ERROR_NO_DISK;
}


