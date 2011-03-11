/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot AROS
    Lang: english
*/

#define DEBUG 0

#include <string.h>
#include <stdio.h>

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

int boot_entry()
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
    NT_PROCESS,
    -50,
    "Boot Strap",
    "AROS Boot Strap 41.0\r\n",
    (APTR)&boot_init
};

static const struct _dt {
    IPTR    mask,type;
    STRPTR  fs;
} DosTypes[] = {
    { 0xffffffff, AROS_MAKE_ID('B','E','F','S' ), "befs.handler"  },
    { 0xffffff00, AROS_MAKE_ID('B','S','D','\0'), "bsd.handler"   },
    { 0xffffff00, AROS_MAKE_ID('C','P','M','\0'), "cpm.handler"   },
    { 0xffffff00, AROS_MAKE_ID('D','O','S','\0'), "afs.handler"   },
    { 0xffffff00, AROS_MAKE_ID('E','X','T','\0'), "ext.handler"   },
    { 0xffffff00, AROS_MAKE_ID('F','A','T','\0'), "fat.handler"   },
    { 0xffffff00, AROS_MAKE_ID('L','V','M','\0'), "lvm.handler"   },
    { 0xffffff00, AROS_MAKE_ID('M','N','X','\0'), "minix.handler" },
    { 0xffffffff, AROS_MAKE_ID('N','T','F','S' ), "ntfs.handler"  },
    { 0xffffffff, AROS_MAKE_ID('R','A','I','D' ), "raid.handler"  },
    { 0xffffff00, AROS_MAKE_ID('S','F','S','\0'), "sfs.handler"   },
    { 0xffffff00, AROS_MAKE_ID('S','K','Y','\0'), "skyfs.handler" },
    { 0xffffffff, AROS_MAKE_ID('V','F','A','T' ), "fat.handler"   },
    { 0,0,NULL }
};

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

static struct FileSysEntry *MatchFileSystemResourceHandler(IPTR DosType)
{
    struct FileSysResource *fsr;
    struct FileSysEntry *fsrnode;

    fsr = OpenResource("FileSystem.resource");
    if (!fsr)
    	return NULL;
    ForeachNode(&fsr->fsr_FileSysEntries, fsrnode) {
        if (fsrnode->fse_DosType == DosType && fsrnode->fse_SegList != BNULL)
            return fsrnode;
    }
    return NULL;
}

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)
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
       D(bug("bootblock checksum %s (%08x %08x)\n", crc == crc2 ? "ok" : "error", crc, crc2));
       return crc == crc2;
}

struct InitTable {
	ULONG dSize;
	APTR  vectors;
	APTR  structure;
	ULONG_FUNC init;
};

static void FloppyBootNode(
        struct ExpansionBase *ExpansionBase,
        CONST_STRPTR driver, int unit, ULONG type, BOOL hddisk, BOOL bootable)
{
    TEXT dosdevname[4] = "DF0";
    IPTR pp[4 + sizeof(struct DosEnvec)/sizeof(IPTR)] = {};
    struct DeviceNode *devnode;

    dosdevname[2] += unit;
    D(bug("strap: Adding bootnode %s: dostype=%08x DDHD=%d\n", dosdevname, type, hddisk ? 1 : 0));

    pp[0] = (IPTR)dosdevname;
    pp[1] = (IPTR)driver;
    pp[2] = unit;
    pp[DE_TABLESIZE + 4] = DE_BOOTBLOCKS;
    pp[DE_SIZEBLOCK + 4] = 128;
    pp[DE_NUMHEADS + 4] = 2;
    pp[DE_SECSPERBLOCK + 4] = 1;
    pp[DE_BLKSPERTRACK + 4] = hddisk ? 22 : 11;
    pp[DE_RESERVEDBLKS + 4] = 2;
    pp[DE_LOWCYL + 4] = 0;
    pp[DE_HIGHCYL + 4] = 79;
    pp[DE_NUMBUFFERS + 4] = 10;
    pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC;
    pp[DE_MAXTRANSFER + 4] = 0x00200000;
    pp[DE_MASK + 4] = 0x7FFFFFFE;
    pp[DE_BOOTPRI + 4] = bootable ? 5 - (unit * 10) : -128;
    pp[DE_DOSTYPE + 4] = type;
    pp[DE_BOOTBLOCKS + 4] = 2;
    devnode = MakeDosNode(pp);

    if (devnode) {
    	struct FileSysEntry *fse = MatchFileSystemResourceHandler(type);
	/* NULL dn_Handler is ok, rn_FileHandlerSegment is supported now */
    	if (fse && (fse->fse_PatchFlags & FSEF_SEGLIST))
    	    devnode->dn_Handler = fse->fse_SegList;
   	AddBootNode(pp[DE_BOOTPRI + 4], ADNF_STARTPROC, devnode, 0);
    }
}

#define BOOTBLOCK_SIZE 1024
static void BootBlock(struct ExpansionBase *ExpansionBase)
{
       struct MsgPort *msgport;
       struct IOExtTD *io;
       struct DriveGeometry dg;
       CONST_STRPTR driver = "trackdisk.device";
       UWORD i;
       WORD bootdrive = -1;
       LONG retval = -1;
       void (*init)(void) = NULL;

       /* memf_chip not required but more compatible with old bootblocks */
       UBYTE *buffer = AllocMem(BOOTBLOCK_SIZE, MEMF_CHIP);
       D(bug("bootblock address %8x\n", buffer));
       if (buffer) {
           for (i = 0; i < 4; i++) {
               if ((msgport = CreateMsgPort())) {
                   if ((io = (struct IOExtTD*)CreateIORequest(msgport, sizeof(struct IOExtTD)))) {
                       if (!OpenDevice(driver, i, (struct IORequest*)io, 0)) {
                           ULONG dostype = 0x444f5300;
                           BOOL dg_ok = FALSE;
                           D(bug("%s:%d open\n", driver, i));
                           io->iotd_Req.io_Command = TD_GETGEOMETRY;
                           io->iotd_Req.io_Data = &dg;
                           io->iotd_Req.io_Length = sizeof dg;
                           DoIO((struct IOReqest*)io);
                           if (io->iotd_Req.io_Error == 0)
                               dg_ok = TRUE;
                           if (bootdrive < 0) {
                               io->iotd_Req.io_Length = BOOTBLOCK_SIZE;
                               io->iotd_Req.io_Data = buffer;
                               io->iotd_Req.io_Offset = 0;
                               io->iotd_Req.io_Command = CMD_READ;
                               DoIO((struct IORequest*)io);
                               if (io->iotd_Req.io_Error == 0) {
                                   D(bug("bootblock read ok\n"));
                                   if (BootBlockChecksum(buffer)) {
                               	       APTR bootcode = buffer + 12;
                               	       ExpansionBase->Flags &= ~EBF_SILENTSTART;
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)
                               	       D(bug("calling bootblock!\n", buffer));
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
                               	       dostype = *(ULONG *)buffer;
                               	       if (retval == 0)
                               	       	   bootdrive = i;
                                       else
                                           Alert(AN_BootError);
#endif
                                   }
                               } else {
                                   D(bug("ioerror %d\n", io->iotd_Req.io_Error));
                               }
                           }
                           CloseDevice((struct IORequest*)io);
                           FloppyBootNode(ExpansionBase, driver, i, dostype, dg.dg_TotalSectors == 22 && dg_ok, bootdrive == i);
                       }
                       DeleteIORequest((struct IORequest*)io);
                   }
                   DeleteMsgPort(msgport);
               }
           }
           FreeMem(buffer, BOOTBLOCK_SIZE);
       }

       if (bootdrive >= 0 && init != NULL) {
           CloseLibrary((APTR)ExpansionBase);
           D(bug("calling bootblock\n"));
           init();
       }
}

#endif

static STRPTR MatchHandler(IPTR DosType)
{
    int i;
    STRPTR fs = NULL;

    for (i = 0; i < (sizeof(DosTypes) / sizeof(struct _dt)); i++)
    {
	if ((DosType & DosTypes[i].mask) == DosTypes[i].type)
	{
	    fs = DosTypes[i].fs;
	    break;
	}
    }
    return fs;
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

static ULONG GetOffset(struct PartitionBase *PartitionBase,
    struct PartitionHandle *ph)
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

static VOID AddPartitionVolume
    (
	struct ExpansionBase *ExpansionBase,
	struct PartitionBase *PartitionBase,
	struct FileSysStartupMsg *fssm,
	struct PartitionHandle *table,
	struct PartitionHandle *pn,
	struct ExecBase * SysBase
    )
{
    UBYTE name[32];
    ULONG i, blockspercyl;
    const struct PartitionAttribute *attrs;
    IPTR tags[7];
    IPTR *pp;
    struct DeviceNode *devnode;
    struct PartitionType ptyp;
    LONG ppos;
    TEXT *devname, *handler;
    LONG bootable;
    struct FileSysEntry *fse;

    D(bug("[Boot] AddPartitionVolume\n"));
    pp = AllocVec(sizeof(struct DosEnvec) + sizeof(IPTR) * 4,
        MEMF_PUBLIC | MEMF_CLEAR);
    if (pp)
    {
        attrs = QueryPartitionAttrs(table);
        while ((attrs->attribute != PTA_DONE) && (attrs->attribute != PTA_NAME))
            attrs++;  /* look for name attr */
        if (attrs->attribute != PTA_DONE)
        {
	    D(bug("[Boot] RDB partition\n"));
            /* partition has a name => RDB partition */
	    tags[0] = PT_NAME;
	    tags[1] = (IPTR)name;
	    tags[2] = PT_DOSENVEC;
	    tags[3] = (IPTR)&pp[4];
	    tags[4] = PT_BOOTABLE;
	    tags[5] = (IPTR)&bootable;
	    tags[6] = TAG_DONE;
	    GetPartitionAttrs(pn, (struct TagItem *)tags);
	    D(bug("[Boot] Partition name: %s bootable: %d\n", name, bootable));
	    /* BHFormat complains if this bit is not set, and it's really wrong to have it unset. So we explicitly set it here.
	       Pavel Fedin <sonic_amiga@rambler.ru> */
            pp[4 + DE_TABLESIZE] = DE_BOOTBLOCKS;
	    pp[4 + DE_BUFMEMTYPE] |= MEMF_PUBLIC;
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
	    D(bug("[Boot] Partition name: %s type: %lu bootable: %d\n", name, ptyp.id[0], bootable));
            /* set DOSTYPE based on the partition type */
            pp[4 + DE_DOSTYPE] = MatchPartType(ptyp.id[0]);
            /* set some common DOSENV fields */
            pp[4 + DE_TABLESIZE] = DE_BOOTBLOCKS;
            pp[4 + DE_NUMBUFFERS] = 20;
            pp[4 + DE_BUFMEMTYPE] = MEMF_PUBLIC;
            pp[4 + DE_MAXTRANSFER] = 0x00200000;
            pp[4 + DE_MASK] = 0x7ffffffe;
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

	D(bug("[Boot] Looking up handler for 0x%08lX\n", pp[4+DE_DOSTYPE]));
        handler = MatchHandler(pp[4 + DE_DOSTYPE]);

        /* Skip unknown partition types */
        if (handler != NULL)
        {
	    D(bug("[Boot] found handler: %s\n", handler));
            devnode = MakeDosNode(pp);
            if (devnode != NULL)
            {
                devnode->dn_Handler = MKBADDR(AllocVec(AROS_BSTR_MEMSIZE4LEN(
                    strlen(handler)), MEMF_PUBLIC | MEMF_CLEAR));
                if (devnode->dn_Handler)
                {
                    i = 0;
                    while (handler[i] != '\0')
                    {
                        AROS_BSTR_putchar(devnode->dn_Handler, i, handler[i]);
                        i++;
                    }
                    AROS_BSTR_setstrlen(devnode->dn_Handler, i);
                    AddBootNode(bootable ? pp[4 + DE_BOOTPRI] : -128, 0, devnode, 0);
                    D(bug("[Boot] AddBootNode(%s,0x%lx,'%s')\n",
                        AROS_DOSDEVNAME(devnode),
                        pp[4 + DE_DOSTYPE], handler));
                    return;
                }
            }
        }

	fse = MatchFileSystemResourceHandler(pp[4 + DE_DOSTYPE]);
	if (fse != NULL)
	{
	    D(bug("[Boot] found in FileSystem.resource\n"));
            devnode = MakeDosNode(pp);
            if (devnode != NULL)
            {
            	devnode->dn_SegList = fse->fse_SegList;
            	/* other fse_PatchFlags bits are quite pointless */
            	if (fse->fse_PatchFlags & 16)
            	    devnode->dn_StackSize = fse->fse_StackSize;
            	if (fse->fse_PatchFlags & 32)
            	    devnode->dn_Priority = fse->fse_Priority;
          	if (fse->fse_PatchFlags & 256)
            	    devnode->dn_GlobalVec = fse->fse_GlobalVec;
	    
                AddBootNode(bootable ? pp[4 + DE_BOOTPRI] : -128, 0, devnode, 0);
                D(bug("[Boot] AddBootNode(%s,0x%lx,%p)\n",
                    AROS_DOSDEVNAME(devnode),
                    pp[4 + DE_DOSTYPE], fse));
                return;
	    }
	}

        FreeVec(pp);
    }
}

static BOOL CheckTables
    (
	struct ExpansionBase *ExpansionBase,
	struct PartitionBase *PartitionBase,
	struct FileSysStartupMsg *fssm,
	struct PartitionHandle *table,
	struct ExecBase *SysBase
    )
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
            if (!CheckTables(ExpansionBase, PartitionBase, fssm, ph, SysBase))
                AddPartitionVolume(ExpansionBase, PartitionBase, fssm, table,
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
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)
    /* On AROS m68k, CF cards are removable, but are
     * bootable. Also, we should support CDROMs that
     * have RDB blocks. So, in all cases, allow the
     * RDB support.
     */
    return FALSE;
#else
    struct DriveGeometry dg;

    ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
    ioreq->iotd_Req.io_Data = &dg;
    ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
    DoIO((struct IORequest *)ioreq);
    return (dg.dg_Flags & DGF_REMOVABLE) ? TRUE : FALSE;
#endif
}

static VOID CheckPartitions
	(
		struct ExpansionBase *ExpansionBase,
		struct ExecBase *SysBase,
		struct BootNode *bn
	)
{
    struct PartitionBase *PartitionBase;
    struct PartitionHandle *pt;
    struct FileSysStartupMsg *fssm;
    struct DeviceNode *dn = (struct DeviceNode *)bn->bn_DeviceNode;

    if (dn->dn_SegList != BNULL) {
    	D(bug("CheckPartition('%s') handler = %x\n", AROS_DOSDEVNAME(dn), dn->dn_SegList));
        /* we already have filesystem handler */
        Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
        return;
    }

    D(bug("CheckPartition('%s') checking..\n", AROS_DOSDEVNAME(dn)));
    PartitionBase =
        (struct PartitionBase *)OpenLibrary("partition.library", 1);
    if (PartitionBase)
    {
        fssm = BADDR(dn->dn_Startup);
        pt = OpenRootPartition(AROS_BSTR_ADDR(fssm->fssm_Device),
            fssm->fssm_Unit);
        if (pt)
        {
            if (IsRemovable(SysBase, pt->bd->ioreq))
            {
                /* don't check removable devices for partition tables */
                Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
            }
            else
            {
                if (!CheckTables(ExpansionBase, PartitionBase, fssm, pt,
                    SysBase))
                {
                    /* no partition table found, so reinsert node */
                  Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
                }
            }
            CloseRootPartition(pt);
        }
        else
        {
            /* amicdrom fails here because of non-initialized libraries */
            Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
        }
        CloseLibrary((struct Library *)PartitionBase);
    } else {
        /* If no partition.library, put it back. */
        Enqueue(&ExpansionBase->MountList, (struct Node *)bn);
    }
}

AROS_UFH3(int, AROS_SLIB_ENTRY(init, boot),
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(ULONG, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT
    struct ExpansionBase *ExpansionBase;
    struct BootNode *bootNode;
    struct List list;
    struct Resident *DOSResident;
    void *BootLoaderBase;

    ExpansionBase =
        (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
    if( ExpansionBase == NULL )
    {
        D(bug( "Could not open expansion.library, something's wrong!\n"));
        Alert(AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_ExpansionLib);
    }

    /* Call the expansion initializations */
    ConfigChain(NULL);
    ExpansionBase->Flags |= EBF_SILENTSTART;

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)
    /* Try to get a boot-block from the trackdisk.device */
    BootBlock(ExpansionBase);
#endif

#if !(AROS_FLAVOUR & AROS_FLAVOUR_EMULATION)
    /* Try to open bootloader.resource */
    if ((BootLoaderBase = OpenResource("bootloader.resource")) != NULL)
    {
    	struct List *args;
    	struct Node *node;
    	ULONG delay = 0;

    	args = GetBootInfo(BL_Args);

    	if (args)
    	{
    		/*
    		 * Search the kernel parameters for the bootdelay=%d string. It determines the
    		 * delay in seconds.
    		 */
    		ForeachNode(args, node)
    		{
    			if (strncmp(node->ln_Name, "bootdelay=", 10) == 0)
    			{
    				struct MsgPort *port = CreateMsgPort();
    				struct timerequest *tr = (struct timerequest *)
                        CreateIORequest(port, sizeof(struct timerequest));

    				OpenDevice("timer.device", UNIT_VBLANK,
                        (struct IORequest *)tr, 0);

    				sscanf(node->ln_Name, "bootdelay=%lu", &delay);
    				D(bug("[Boot] delay of %d seconds requested.", delay));

    				tr->tr_node.io_Command = TR_ADDREQUEST;
    				tr->tr_time.tv_sec = delay;
    				tr->tr_time.tv_usec = 0;

    				DoIO((struct IORequest *)tr);

    				CloseDevice((struct IORequest *)tr);
    				DeleteIORequest((struct IORequest *)tr);
    				DeleteMsgPort(port);
    			}
    		}
    	}
    }
#endif

    /* move all boot nodes into another list */
    NEWLIST(&list);
    while ((bootNode = (struct BootNode *)RemHead(&ExpansionBase->MountList)))
        AddTail(&list, &bootNode->bn_Node);

    /* check boot nodes for partition tables */
    while ((bootNode = (struct BootNode *)RemHead(&list)))
        CheckPartitions(ExpansionBase, SysBase, bootNode);

    CloseLibrary((struct Library *)ExpansionBase);

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
