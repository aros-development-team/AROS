/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot AROS
    Lang: english
*/

#define DEBUG 0

#include <string.h>
#include <stdlib.h>

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

#include <aros/debug.h>
#include <aros/macros.h>

#define uppercase(x) ((x >= 'a' && x <= 'z') ? (x & 0xdf) : x)

int __startup boot_entry()
{
	return -1;
}

static const UBYTE boot_end;
int AROS_SLIB_ENTRY(init,boot)();

const struct Resident boot_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&boot_resident,
    (APTR)&boot_end,
    RTF_COLDSTART,
    41,
    NT_TASK,
    -119,
    "Boot Strap",
    "AROS Boot Strap 41.0\r\n",
    (APTR)&boot_init
};

/* 
 * TODO: Check if DOSType lookup in partition.library really works
 * and remove this table and related code.
 */
static const struct _pt {
    IPTR    part,type;
} PartTypes[] = {
    { 0x01, AROS_MAKE_ID('F','A','T',' ')  },	/* DOS 12-bit FAT */
    { 0x04, AROS_MAKE_ID('F','A','T',' ')  },	/* DOS 16-bit FAT (up to 32M) */
    { 0x06, AROS_MAKE_ID('F','A','T',' ')  },	/* DOS 16-bit FAT (over 32M) */
    { 0x07, AROS_MAKE_ID('N','T','F','S')  },	/* Windows NT NTFS */
    { 0x0b, AROS_MAKE_ID('V','F','A','T')  },	/* W95 FAT32 */
    { 0x0c, AROS_MAKE_ID('V','F','A','T')  },	/* W95 LBA FAT32 */
    { 0x0e, AROS_MAKE_ID('F','A','T',' ')  },	/* W95 16-bit LBA FAT */
    { 0x2c, AROS_MAKE_ID('D','O','S','\0') },	/* AOS OFS */
    { 0x2d, AROS_MAKE_ID('D','O','S','\1') },	/* AOS FFS */
    { 0x2e, AROS_MAKE_ID('D','O','S','\3') },	/* AOS FFS-I */
    { 0x2f, AROS_MAKE_ID('S','F','S','\0') },	/* AOS SFS */
    { 0x80, AROS_MAKE_ID('M','N','X','\0') },	/* MINIX until 1.4a */
    { 0x81, AROS_MAKE_ID('M','N','X','\1') },	/* MINIX since 1.4b */
    { 0x83, AROS_MAKE_ID('E','X','T','\2') },	/* linux native partition */
    { 0x8e, AROS_MAKE_ID('L','V','M','\0') },	/* linux LVM partition */
    { 0x9f, AROS_MAKE_ID('B','S','D','\0') },	/* BSD/OS */
    { 0xa5, AROS_MAKE_ID('B','S','D','\1') },	/* NetBSD, FreeBSD */
    { 0xa6, AROS_MAKE_ID('B','S','D','\2') },	/* OpenBSD */
    { 0xdb, AROS_MAKE_ID('C','P','M','\2') },	/* CPM/M */
    { 0xeb, AROS_MAKE_ID('B','E','F','S')  },	/* BeOS FS */
    { 0xec, AROS_MAKE_ID('S','K','Y','\0') },	/* SkyOS FS */
    { 0xfd, AROS_MAKE_ID('R','A','I','D')  },	/* linux RAID with autodetect */
    { 0, 0 }
};

/* Find the most recent version of the matching filesystem
 */
static struct FileSysEntry *MatchFileSystemResourceHandler(struct FileSysResource *fsr, ULONG DosType)
{
    struct FileSysEntry *fse, *best_fse = NULL;

    ForeachNode(&fsr->fsr_FileSysEntries, fse) {
        if (fse->fse_DosType == DosType) {
            if (fse->fse_PatchFlags & (FSEF_HANDLER | FSEF_SEGLIST | FSEF_TASK)) {
                if (best_fse == NULL ||
                    fse->fse_Version > best_fse->fse_Version) {
                    best_fse = fse;
                }
            }
        }
    }
    D(bug("[Strap] Best fse for 0x%8x is: %p\n", DosType, best_fse));

    return best_fse;
}


/* See if the BootNode's DeviceNode needs to be patched by
 * an entry in FileSysResource
 *
 * If dostype != 0, then the node is forced to that ID
 */
static void PatchBootNode(struct FileSysResource *fsr, struct BootNode *bn, ULONG dostype)
{
    struct DeviceNode *dn;
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *de;
    struct FileSysEntry *fse;

    /* If the caller was lazy, open fsr for them */
    if (!fsr) {
        fsr = OpenResource("FileSystem.resource");
        if (!fsr)
            return;
    }

    dn = bn->bn_DeviceNode;
    if (!dn)
        return;

    /* If we're not overriding, don't clobber
     * already configured nodes
     */
    if (dostype == 0) {
        /* If we already have a task installed,
         * then we're done.
         */
        if (dn->dn_Task != NULL)
            return;

        /* If we already have a handler installed,
         * then we're done.
         */
        if (dn->dn_SegList != BNULL)
            return;
    }

    fssm = BADDR(dn->dn_Startup);
    if (fssm == NULL)
        return;

    de = BADDR(fssm->fssm_Environ);
    if (de == NULL)
        return;

    /* Allow overriding the de_DosType */
    if (dostype == 0) {
        dostype = de->de_DosType;
    } else {
        de->de_DosType = dostype;
        dn->dn_Handler = BNULL;
        dn->dn_SegList = BNULL;
        dn->dn_GlobalVec = 0;
    }

    if (!dostype)
        return;

    D(bug("[Boot] Looking for patches for DeviceNode %p\n", dn));

    /*
     * MatchFileSystemResourceHandler looks up the filesystem
     */
    fse = MatchFileSystemResourceHandler(fsr, dostype);
    if (fse != NULL)
    {
        D(bug("[Boot] found 0x%08x in FileSystem.resource\n", dostype));
        dn->dn_SegList = fse->fse_SegList;
        /* other fse_PatchFlags bits are quite pointless */
        if (fse->fse_PatchFlags & FSEF_TASK)
            dn->dn_Task = (APTR)fse->fse_Task;
        if (fse->fse_PatchFlags & FSEF_LOCK)
            dn->dn_Lock = fse->fse_Lock;

        /* Adjust the stack size for 64-bits if needed.
         */
        if (fse->fse_PatchFlags & FSEF_STACKSIZE)
            dn->dn_StackSize = (fse->fse_StackSize/sizeof(ULONG))*sizeof(IPTR);
        if (fse->fse_PatchFlags & FSEF_PRIORITY)
            dn->dn_Priority = fse->fse_Priority;
        if (fse->fse_PatchFlags & FSEF_GLOBALVEC)
            dn->dn_GlobalVec = fse->fse_GlobalVec;
    }
}

static BOOL GetBootNodeDeviceUnit(struct BootNode *bn, BPTR *device, ULONG *unit)
{
    struct DeviceNode *dn;
    struct FileSysStartupMsg *fssm;

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

    return TRUE;
}
 
static BOOL BootBlockChecksum(UBYTE *bootblock)
{
       ULONG crc = 0, crc2 = 0;
       UWORD i;
       for (i = 0; i < 1024; i += 4) {
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
        retval = -1;
        init = NULL;
    }
#endif
    *initcode = init;
    return retval;
}

static void BootBlock(struct ExpansionBase *ExpansionBase, struct BootNode *bn)
{
    const ULONG BOOTBLOCK_SIZE = 1024;
    struct MsgPort *msgport;
    struct IOStdReq *io;
    BPTR device;
    ULONG unit;
    LONG retval;
    VOID_FUNC init = NULL;
    UBYTE *buffer;

    /* BootNodes that don't have a ConfigDev are not bootable */
    if (bn->bn_Node.ln_Name == NULL)
        return;

    if (!GetBootNodeDeviceUnit(bn, &device, &unit))
        return;

    /* memf_chip not required but more compatible with old bootblocks */
    buffer = AllocMem(BOOTBLOCK_SIZE, MEMF_CHIP);
    if (buffer != NULL) {
       D(bug("[Strap] bootblock address %p\n", buffer));
       if ((msgport = CreateMsgPort())) {
           if ((io = CreateIORequest(msgport, sizeof(struct IOStdReq)))) {
               if (!OpenDevice(AROS_BSTR_ADDR(device), unit, (struct IORequest*)io, 0)) {
                   /* Read the device's boot block
                    */
                   io->io_Length = BOOTBLOCK_SIZE;
                   io->io_Data = buffer;
                   io->io_Offset = 0;
                   io->io_Command = CMD_READ;
                   D(bug("[Strap] %b.%d bootblock read (%d bytes)\n", device, unit, BOOTBLOCK_SIZE));
                   DoIO((struct IORequest*)io);
                   if (io->io_Error == 0) {
                       D(bug("[Strap] %b.%d bootblock read to %p ok\n", device, unit, buffer));
                       if (BootBlockChecksum(buffer)) {
                           APTR bootcode = buffer + 12;

                           /* Force the handler for this device */
                           PatchBootNode(NULL, bn, *(ULONG *)buffer);

                           ExpansionBase->Flags &= ~EBF_SILENTSTART;

                           D(bug("[Strap] Calling bootblock!\n", buffer));
                           retval = CallBootBlockCode(bootcode, io, &init);
                           if (retval != 0)
                               Alert(AN_BootError);
                       } else {
                           D(bug("[Strap] Not a valid bootblock\n"));
                       }
                   } else {
                       D(bug("[Strap] io_Error %d\n", io->io_Error));
                   }
                   CloseDevice((struct IORequest *)io);
               }
               DeleteIORequest((struct IORequest*)io);
           }
           DeleteMsgPort(msgport);
       }
       FreeMem(buffer, BOOTBLOCK_SIZE);
   }

   if (init != NULL) {
       CloseLibrary((APTR)ExpansionBase);
       D(bug("calling bootblock loaded code at %p\n", init));
       init();
   }
}

static IPTR MatchPartType(UBYTE PartType)
{
    int i;
    IPTR type = 0;

    for (i = 0; i < (sizeof(PartTypes) / sizeof(struct _pt)); i++)
    {
	if ((IPTR)PartType == PartTypes[i].part)
	{
	    type = PartTypes[i].type;
	    break;
	}
    }
    return type;
}

static ULONG GetOffset(struct Library *PartitionBase, struct PartitionHandle *ph)
{
    IPTR tags[3];
    struct DosEnvec de;
    ULONG offset = 0;

    tags[0] = PT_DOSENVEC;
    tags[1] = (IPTR)&de;
    tags[2] = TAG_DONE;
    ph = ph->root;
    while (ph->root)
    {
        GetPartitionAttrs(ph, (struct TagItem *)tags);
        offset += de.de_LowCyl * de.de_Surfaces * de.de_BlocksPerTrack;
        ph = ph->root;
    }
    return offset;
}

static VOID AddPartitionVolume(struct ConfigDev *cfgdev,
			       struct ExpansionBase *ExpansionBase, struct Library *PartitionBase,
			       struct FileSysStartupMsg *fssm, struct PartitionHandle *table,
			       struct PartitionHandle *pn, struct ExecBase *SysBase)
{
    UBYTE name[32];
    ULONG i, blockspercyl;
    const struct PartitionAttribute *attrs;
    IPTR tags[7];
    IPTR pp[4 + DE_BOOTBLOCKS + 1] = { };
    struct DeviceNode *devnode;
    struct PartitionType ptyp;
    LONG ppos;
    TEXT *devname;
    LONG bootable;
    ULONG pttype = PHPTT_UNKNOWN;


    /*
     * TODO: Try to locate RDB filesystem for this volume and make it bootable.
     * Use FindFileSystem() and AddBootFileSystem() for this.
     */

    D(bug("[Boot] AddPartitionVolume\n"));
    GetPartitionTableAttrsTags(table, PTT_TYPE, &pttype, TAG_DONE);

    attrs = QueryPartitionAttrs(table);
    while ((attrs->attribute != PTA_DONE) && (attrs->attribute != PTA_NAME))
        attrs++;  /* look for name attr */
    if (attrs->attribute != PTA_DONE)
    {
        D(bug("[Boot] RDB/GPT partition\n"));

        /* partition has a name => RDB/GPT partition */
        tags[0] = PT_NAME;
        tags[1] = (IPTR)name;
        tags[2] = PT_DOSENVEC;
        tags[3] = (IPTR)&pp[4];
        tags[4] = PT_BOOTABLE;
        tags[5] = (IPTR)&bootable;
        tags[6] = TAG_DONE;
        GetPartitionAttrs(pn, (struct TagItem *)tags);
        D(bug("[Boot] Partition name: %s bootable: %d\n", name, bootable));

        /*
         * CHECKME: This should not be needed at all. Partition.library knows what it does,
         * and it knows DosEnvec size. RDB partitions should have complete DosEnvec. GPT
         * partitions (also processed here) have only fields up to de_DosType filled in,
         * and this is correctly reflected in the DosEnvec.
        pp[4 + DE_TABLESIZE] = DE_BOOTBLOCKS;
         */
    }
    else
    {
        D(bug("[Boot] MBR partition\n"));

        /* partition doesn't have a name => MBR partition */
        tags[0] = PT_POSITION;
        tags[1] = (IPTR)&ppos;
        tags[2] = PT_TYPE;
        tags[3] = (IPTR)&ptyp;
        tags[4] = PT_DOSENVEC;
        tags[5] = (IPTR)&pp[4];
        tags[6] = TAG_DONE;
        GetPartitionAttrs(pn, (struct TagItem *)tags);
        bootable = TRUE;

        /* make the name */
        devname = AROS_BSTR_ADDR(fssm->fssm_Device);
        for (i = 0; i < 26; i++)
        {
            if (*devname == '.' || *devname == '\0')
                break;
            name[i] = (UBYTE)uppercase(*devname);
            devname++;
        }
        if ((fssm->fssm_Unit / 10))
            name[i++] = '0' + (UBYTE)(fssm->fssm_Unit / 10);
        name[i++] = '0' + (UBYTE)(fssm->fssm_Unit % 10);
        name[i++] = 'P';
        if (table->table->type == PHPTT_EBR)
            ppos += 4;
        if ((ppos / 10))
            name[i++] = '0' + (UBYTE)(ppos / 10);
        name[i++] = '0' + (UBYTE)(ppos % 10);
        name[i] = '\0';
        D(bug("[Boot] Partition name: %s type: %u\n", name, ptyp.id[0]));

        /*
         * FIXME: These MBR-related DosEnvec patches should already be correctly done
         * by partition.library. Test this and remove the unneeded code from here.
         */

        /* set DOSTYPE based on the partition type */
        pp[4 + DE_DOSTYPE] = MatchPartType(ptyp.id[0]);
        /* set some common DOSENV fields */
        pp[4 + DE_TABLESIZE] = DE_BOOTBLOCKS;
        pp[4 + DE_NUMBUFFERS] = 20;
        pp[4 + DE_BUFMEMTYPE] = MEMF_PUBLIC;
        /* set some fs specific fields */
        switch(ptyp.id[0])
        {
            case 0x2c:	/* OFS */
            case 0x2d:	/* FFS */
            case 0x2e:	/* FFS I */
            case 0x2f:	/* SFS */
                pp[4 + DE_SECSPERBLOCK] = 1;
                pp[4 + DE_RESERVEDBLKS] = 2;
                pp[4 + DE_BOOTBLOCKS] = 2;
                break;
        }
    }

    if (pttype != PHPTT_RDB)
    {
        /*
         * Only RDB partitions can store the complete DosEnvec.
         * For other partition types partition.library puts some defaults
         * into these fields, however they do not have anything to do with
         * real values, which are device-dependent.
         * However, the device itself knows them. Here we inherit these settings
         * from the original DeviceNode which represents the whole drive.
         * Note that we don't change DosEnvec size. If these fields are not included,
         * it will stay this way.
         * Copy members only if they are present in device's DosEnvec.
         */
        struct DosEnvec *devenv = BADDR(fssm->fssm_Environ);

        if (devenv->de_TableSize >= DE_MAXTRANSFER)
        {
            pp[4 + DE_MAXTRANSFER] = devenv->de_MaxTransfer;

            if (devenv->de_TableSize >= DE_MASK)
                pp[4 + DE_MASK] = devenv->de_Mask;
        }
    }

    /*
     * BHFormat complains if this bit is not set, and it's really wrong to have it unset.
     * So we explicitly set it here. Pavel Fedin <pavel.fedin@mail.ru>
     */
    pp[4 + DE_BUFMEMTYPE] |= MEMF_PUBLIC;

    pp[0] = (IPTR)name;
    pp[1] = (IPTR)AROS_BSTR_ADDR(fssm->fssm_Device);
    pp[2] = fssm->fssm_Unit;
    pp[3] = fssm->fssm_Flags;

    i = GetOffset(PartitionBase, pn);
    blockspercyl = pp[4 + DE_BLKSPERTRACK] * pp[4 + DE_NUMHEADS];
    if (i % blockspercyl != 0)
    {
        D(bug("[Boot] Start block of subtable not on cylinder boundary: "
            "%ld (Blocks per Cylinder = %ld)\n", i, blockspercyl));
        return;
    }
    i /= blockspercyl;
    pp[4 + DE_LOWCYL] += i;
    pp[4 + DE_HIGHCYL] += i;

    devnode = MakeDosNode(pp);
    if (devnode != NULL) {
        AddBootNode(bootable ? pp[4 + DE_BOOTPRI] : -128, 0, devnode, cfgdev);
        D(bug("[Boot] AddBootNode(%b, 0, 0x%p, NULL)\n",  devnode->dn_Name, pp[4 + DE_DOSTYPE]));
        return;
    }
}

static BOOL CheckTables(struct ConfigDev *cfgdev,
			struct ExpansionBase *ExpansionBase, struct Library *PartitionBase,
			struct FileSysStartupMsg *fssm,	struct PartitionHandle *table,
			struct ExecBase *SysBase)
{
    BOOL retval = FALSE;
    struct PartitionHandle *ph;

    /* Traverse partition tables recursively, and attempt to add a BootNode
       for any non-subtable partitions found */
    if (OpenPartitionTable(table) == 0)
    {
        ph = (struct PartitionHandle *)table->table->list.lh_Head;
        while (ph->ln.ln_Succ)
        {
            /* Attempt to add partition to system if it isn't a subtable */
            if (!CheckTables(cfgdev, ExpansionBase, PartitionBase, fssm, ph, SysBase))
                AddPartitionVolume(cfgdev, ExpansionBase, PartitionBase, fssm, table,
                    ph, SysBase);
            ph = (struct PartitionHandle *)ph->ln.ln_Succ;
        }
        retval = TRUE;
        ClosePartitionTable(table);
    }
    return retval;
}

static BOOL IsRemovable(struct ExecBase *SysBase, struct IOExtTD *ioreq)
{
    struct DriveGeometry dg;

    /*
     * On AROS m68k, CF cards are removable, but are
     * bootable. Also, we should support CDROMs that
     * have RDB blocks. So, in all cases, allow the
     * RDB support.
     * UPD: this is not so easy. If you remove one disk with RDB on it
     * and insert another one, number of partitions and their parameters
     * will change. This means that we actually have to dismount all DeviceNodes
     * for old disk and mount new ones.
     * This is technically possible, however rather complex (we need to track down
     * which DeviceNodes are currently in use, and be able to reuse them when the
     * disk is inserted again in a response to "Insert disk XXX in any drive".
     * MorphOS has this mechanism implemented in mount.library.
     * An alternative is to bind mounted DeviceNodes to a particular disk and mount
     * a new set for every new one. Perhaps it's simpler, but anyway, needs to be
     * handled in some special way.
     */
    if (!strcmp(ioreq->iotd_Req.io_Device->dd_Library.lib_Node.ln_Name, "carddisk.device"))
	return FALSE;

    ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
    ioreq->iotd_Req.io_Data = &dg;
    ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
    DoIO((struct IORequest *)ioreq);

    return (dg.dg_Flags & DGF_REMOVABLE) ? TRUE : FALSE;
}

static VOID CheckPartitions(struct ExpansionBase *ExpansionBase, struct Library *PartitionBase, struct ExecBase *SysBase, struct BootNode *bn)
{
    struct DeviceNode *dn = bn->bn_DeviceNode;
    BOOL res = FALSE;
    struct ConfigDev *cfgdev = (struct ConfigDev *)bn->bn_Node.ln_Name;

    D(bug("CheckPartition('%b') handler = %x\n", dn->dn_Name, dn->dn_SegList));
    
    /* If we already have filesystem handler, don't do anything */
    if (dn->dn_SegList == BNULL)
    {
    	struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);

	if (fssm && fssm->fssm_Device)
	{
            struct PartitionHandle *pt = OpenRootPartition(AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit);

	    if (pt)
            {
            	/* don't check removable devices for partition tables */
            	if (!IsRemovable(SysBase, pt->bd->ioreq))
                    res = CheckTables(cfgdev, ExpansionBase, PartitionBase, fssm, pt, SysBase);

           	CloseRootPartition(pt);
           }
        }
    }

    if (!res)    
        /* If no partitions were found for the DeviceNode, put it back */
        Enqueue(&ExpansionBase->MountList, &bn->bn_Node);
}

AROS_UFH3(int, AROS_SLIB_ENTRY(init, boot),
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(ULONG, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct ExpansionBase *ExpansionBase;
    struct Resident *DOSResident;
    void *BootLoaderBase;
    struct Library *PartitionBase;
    struct BootNode *bootNode;
    struct FileSysResource *fsr;

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0);

    D(bug("[Strap] ExpansionBase 0x%p\n", ExpansionBase));
    if( ExpansionBase == NULL )
    {
        D(bug( "Could not open expansion.library, something's wrong!\n"));
        Alert(AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_ExpansionLib);
    }

    /* Call the expansion initializations */
    ConfigChain(NULL);
    ExpansionBase->Flags |= EBF_SILENTSTART;

    /*
     * Search the kernel parameters for the bootdelay=%d string. It determines the
     * delay in seconds.
     */
    if ((BootLoaderBase = OpenResource("bootloader.resource")) != NULL)
    {
    	struct List *args = GetBootInfo(BL_Args);

    	if (args)
    	{
    	    struct Node *node;

    	    ForeachNode(args, node)
    	    {
    		if (strncmp(node->ln_Name, "bootdelay=", 10) == 0)
    		{
    		    ULONG delay = atoi(&node->ln_Name[10]);

		    D(bug("[Boot] delay of %d seconds requested.", delay));
		    if (delay)
		    {
    			struct MsgPort *port = CreateMsgPort();
    			if (port)
    			{
    			    struct timerequest *tr = (struct timerequest *)CreateIORequest(port, sizeof(struct timerequest));

    			    if (tr)
    			    {
    				if (!OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0))
    				{
    				    tr->tr_node.io_Command = TR_ADDREQUEST;
    				    tr->tr_time.tv_sec = delay;
    				    tr->tr_time.tv_usec = 0;

    				    DoIO((struct IORequest *)tr);

    				    CloseDevice((struct IORequest *)tr);
    				}
    				DeleteIORequest((struct IORequest *)tr);
    			    }
    			    DeleteMsgPort(port);
    			}
    		    }

    		    break;
    		}
    	    }
    	}
    }

    /* If we have partition.library, we can look for partitions */
    PartitionBase = OpenLibrary("partition.library", 2);
    if (PartitionBase)
    {
    	/*
    	 * Remove the whole chain of BootNodes from the list and re-initialize it.
    	 * We will insert new nodes into it, based on old ones.
    	 * What is done here is safe as long is we don't move the list itself.
    	 * ln_Succ of the last node in chain points to the lh_Tail of our list
    	 * which always contains NULL.
    	 */
	bootNode = (struct BootNode *)ExpansionBase->MountList.lh_Head;

	NEWLIST(&ExpansionBase->MountList);

	while (bootNode->bn_Node.ln_Succ)
	{
	    /* Keep ln_Succ because it can be clobbered by reinsertion */
	    struct BootNode *nextNode = (struct BootNode *)bootNode->bn_Node.ln_Succ;

	    CheckPartitions(ExpansionBase, PartitionBase, SysBase, bootNode);
	    bootNode = nextNode;
	}

	CloseLibrary(PartitionBase);
    }

    /* Try to get a boot-block from any device in
     * the boot list.
     */
    ForeachNode(&ExpansionBase->MountList, bootNode) {
        BootBlock(ExpansionBase, bootNode);
    }

    /* Ok, we've collected all the Boot nodes. Now,
     * go through the list and patch them (if needed)
     * from the FileSysResource patches
     */
    fsr = OpenResource("FileSystem.resource");
    if (fsr) {
        ForeachNode(&ExpansionBase->MountList, bootNode) {
            PatchBootNode(fsr, bootNode, 0);
        }
    }

    CloseLibrary(&ExpansionBase->LibNode);

    /*
     * Initialize dos.library manually.
     * It is done for binary compatibility with original m68k Amiga
     * Workbench floppy bootblocks. This is what they do.
     */
    DOSResident = FindResident( "dos.library" );

    if( DOSResident == NULL )
    {
        Alert( AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_DOSLib );
    }

    InitResident( DOSResident, BNULL );

    /* We don't get here if everything went well */
    return 0;

    AROS_USERFUNC_EXIT
}

static const UBYTE boot_end = 0;
