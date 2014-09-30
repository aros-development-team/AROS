/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <libraries/configvars.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <utility/tagitem.h>
#include <dos/filehandler.h>

#include <proto/exec.h>

/*
 * Execute the code in the boot block.
 * This can be custom defined for your architecture.
 *
 * Returns 0 on success, or an error code
 */
VOID_FUNC CallBootBlockCode(APTR bootcode, struct IOStdReq *io, struct ExpansionBase *ExpansionBase)
{
    UBYTE oldflags = ExpansionBase->Flags & EBF_SILENTSTART;
    LONG retval;
    VOID_FUNC init;

    D(bug("[Strap] Calling bootblock 0x%p!\n", bootcode));

    ExpansionBase->Flags &= ~EBF_SILENTSTART;

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

    if (retval != 0)
    {
        D(bug("[Strap] Boot block failed to boot.\n"));

        ExpansionBase->Flags |= oldflags;
        return NULL;
    }

    return init;
}

/* BootPoint booting, as describe in the Amiga Devices Manual */
void dosboot_BootPoint(struct BootNode *bn)
{
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
    if (bootblocks == 0 && bn->bn_Node.ln_Name != NULL)
    {
        struct ConfigDev *cd = (APTR)bn->bn_Node.ln_Name;

        if (cd->cd_Rom.er_DiagArea != NULL)
        {
            struct DiagArea *da = cd->cd_Rom.er_DiagArea;

            if (da->da_Config & DAC_CONFIGTIME)
            {
                /* Yes, it's actually a BootPoint node */
                void *func = (APTR)(((IPTR)da) + da->da_BootPoint);

                D(bug("dosboot_BootStrap: Calling %b BootPoint @%p\n", dn->dn_Name, func));

                /*
                 * Yet another crazy Amiga calling sequence.
                 * The ConfigDev is pushed on the stack, but
                 * the BootNode is in A2. Joy.
                 *
                 * Oh, and don't forget SysBase in A6!
                 */
                asm volatile (
                        "move.l %0,%%a0\n"
                        "move.l %1,%%a1\n"
                        "move.l %2,%%a2\n"
                        "move.l %3,%%a6\n"
                        "move.l %%a1,%%sp@-\n"
                        "jsr    %%a0@\n"
                        "addq.l #4,%%sp\n"
                        :
                        : "d" (func), "d" (cd), "d" (bn), "d" (SysBase)
                        : "d0", "d1", "a0", "a1", "a2", "a6"
                        );
            }
        }
    }
}
