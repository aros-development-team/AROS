/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IDE lowlevel
    Lang: English

    This version handles ATA and ATAPI devices
*/

#include <dos/filehandler.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/initializers.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>
#include <devices/timer.h>
#include <devices/trackdisk.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/timer.h>

#include "include/cd.h"
#include "include/scsicmds.h"
#include "include/scsidisk.h"
//#include "include/hardblocks.h"

#include "ide_intern.h"

#define DEBUG 1
#include <aros/debug.h>

#undef kprintf

#define ioStd(x)  ((struct IOStdReq *)x)

/* IMMEDIATES_CMD = %10000000001111111111000111100011 */
#define IMMEDIATES_CMD      0x803ff1e3

#define BLOCK_SIZE          512
#define CONST_NUM        0x2222
#define ATA_TimeOut      500000
#define ATAPI_TimeOut   1000000

/* Prototypes */
ULONG ata_ReadSector(ULONG hd, ULONG sec, ULONG cyl, APTR buffer, struct ide_Unit *unit);
ULONG ActualAddr(ULONG port, struct ide_Unit *unit);
void IncBlockAddr(ULONG port, struct ide_Unit *unit);
int BlockAddr(ULONG block, struct ide_Unit *unit);
int WaitBusy(ULONG port, struct ide_Unit *unit);
int WaitBusyLong(ULONG port, struct ide_Unit *unit);
void ResumeError(ULONG port, struct ide_Unit *unit);


static const char name[] = "ide.device";

/* Add a bootnode using expansion.library */
BOOL AddVolume(ULONG StartCyl, ULONG EndCyl, struct ide_Unit *unit)
{
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    ULONG *pp;
    static int volnum;
    
    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",
							40L);

    if (ExpansionBase)
    {
	pp = AllocMem(24*4, MEMF_PUBLIC | MEMF_CLEAR);

	if (pp)
	{
	    /* This should be dealt with using some sort of volume manager or such. */
	    switch (unit->au_DevType)
	    {
		case DG_DIRECT_ACCESS:
		    pp[0] = "afs.handler";
		    break;
		case DG_CDROM:
		    pp[0] = "cdrom.handler";
		    break;
		default:
		    D(bug("IDE: AddVolume called on unknown devicetype\n"));
	    }
	    pp[1] = (ULONG)name;
	    pp[2] = unit->au_UnitNumber;
	    pp[DE_TABLESIZE + 4] = DE_BOOTBLOCKS;
	    pp[DE_SIZEBLOCK + 4] = unit->au_SectSize/4;
	    pp[DE_NUMHEADS + 4] = unit->au_Heads;
	    pp[DE_SECSPERBLOCK + 4] = 1;
	    pp[DE_BLKSPERTRACK + 4] = unit->au_SectorsT;
	    pp[DE_RESERVEDBLKS + 4] = 2;
	    pp[DE_LOWCYL + 4] = StartCyl;
	    pp[DE_HIGHCYL + 4] = EndCyl;
	    pp[DE_NUMBUFFERS + 4] = 10;
	    pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC | MEMF_CHIP;
	    pp[DE_MAXTRANSFER + 4] = 0x00200000;
	    pp[DE_MASK + 4] = 0x7FFFFFFE;
	    pp[DE_BOOTPRI + 4] = ((!unit->au_DevType) ? 0 : 10);
	    pp[DE_DOSTYPE + 4] = 0x444F5301;
	    pp[DE_BOOTBLOCKS + 4] = 2;
	    devnode = MakeDosNode(pp);

	    if (devnode)
	    {
		if ((devnode->dn_OldName =
		     MKBADDR(AllocMem(5, MEMF_PUBLIC | MEMF_CLEAR))))
		{
		    if( !unit->au_DevType )
		    {
		    	AROS_BSTR_putchar(devnode->dn_OldName, 0, 'D');
		    	AROS_BSTR_putchar(devnode->dn_OldName, 1, 'H');
		    	AROS_BSTR_putchar(devnode->dn_OldName, 2, '0' + volnum);
		    }
		    else
		    {
		    	AROS_BSTR_putchar(devnode->dn_OldName, 0, 'C');
		    	AROS_BSTR_putchar(devnode->dn_OldName, 1, 'D');
		    	AROS_BSTR_putchar(devnode->dn_OldName, 2, '0' + volnum);
		    }
		    AROS_BSTR_setstrlen(devnode->dn_OldName, 3);
		    devnode->dn_NewName = AROS_BSTR_ADDR(devnode->dn_OldName);

		    D(bug("-Adding volume %s with SC=%d, EC=%d\n",
			  &(devnode->dn_NewName[0]), StartCyl, EndCyl));
		    AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, 0);
		    volnum++;

		    return TRUE;
		}
	    }
	}

	CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}

#if 0
/*
   This version will read the MBR and then add all partitions of type 0x30
   as bootnodes. It could be a bit smarter though.
*/
void RDBInfo(struct ide_Unit *unit)
{
    UBYTE *mbr;
    ULONG temp,count,i,SSecs,SCyls,ESecs,ECyls;
    struct PartEntry *pe;

    /* For now we will only deal with harddisks */
    if (unit->au_DevType != DG_DIRECT_ACCESS)
      return;

    mbr = AllocMem(512,MEMF_PUBLIC | MEMF_CLEAR);
    if (mbr)
    {
        /* Read the MBR Sector */
        count = ReadBlocks(0,1,mbr,unit,&temp);
        if (!count)
        {
            /* Print partition table and add BootNodes */
            D(bug("====================== Partition table =======================\n"));
            D(bug("Act Type StartC StartH StartS EndC EndH EndS LBAStart LBACount\n"));
            for (i=0;i<=3;i++)
            {
                count = 0x1be + (i*16);
                pe = (struct PartEntry *)(&mbr[count]);
                SSecs = pe->StartS & 0x3f;
                SCyls = ((pe->StartS & 0xc0) << 2) + pe->StartC;
		if (SCyls == 1023)
		{
		    SCyls = pe->LBAStart/unit->au_Heads/unit->au_SectorsT;
		    SSecs = 1;
		    pe->StartH = 0;
		}

		ESecs = pe->EndS &0x3f;
                ECyls = ((pe->EndS & 0xc0) << 2) + pe->EndC;
		if (ECyls == 1023)
		{
		    ECyls = SCyls -1 + pe->LBACount/unit->au_Heads/unit->au_SectorsT;
		}

                D(bug(" %02x  %02x   %3d    %3d   %4d   %4d  %3d  %3d %8d %8d\n",
                    pe->Status,pe->PartType,SCyls,pe->StartH,SSecs,
                    ECyls,pe->EndH,ESecs,pe->LBAStart,pe->LBACount));

                /* If this is an AROS partition, add a BootNode */
                if (pe->PartType == 0x30)
                   AddVolume(SCyls,ECyls,unit);
		   //AddVolume(2006,ECyls,unit);
            }
            FreeMem(mbr,512);
        }
    }
    return;
}
#endif

/* Try to get CHS info from this drive */
void SearchCHS(struct ide_Unit *unit)
{
    /* Buffer */
    APTR    empty;
    ULONG   hd, sec, cyl, incr;
    
    /* Belive, we can AllocSome memory */
    empty = AllocMem(512, MEMF_PUBLIC);
    
    /* Search for first invalid sector number */
    for(hd = 0, sec = 1, cyl = 0;
        (sec < 65) & ata_ReadSector(hd, sec, cyl, empty, unit); sec++);
    
    if (sec == 65) sec = 36;

    sec--;

    /* store it! */
    unit->au_SectorsT = sec;
    
    /* Do the same for number of heads */
    for(hd = 0, sec = 1, cyl = 0;
        (hd < 16) & ata_ReadSector(hd, sec, cyl, empty, unit); hd++)

    unit->au_Heads = hd;

    /* Do binary search - guess total number of cylinders */
    hd = 0;
    sec = 1;
    cyl = 32768;
    incr = 32768;

    while (incr > 1)
    {
        incr >>= 1;     /* Make add/sub step smaller */
        /* Can you read such sector? */
        if (ata_ReadSector(hd, sec, cyl, empty, unit))
        {
            /* Yes, increment cyl number */
            cyl += incr;

        }
        else
        {
            /* No, decrement (go back) */
            cyl -= incr;
        }
    }

    if (ata_ReadSector(hd, sec, cyl, empty, unit))
        cyl++;

    /* Calculate drive geometry */
    unit->au_Cylinders = cyl;

    unit->au_SectorsC = unit->au_SectorsT * unit->au_Heads;
    unit->au_Blocks = unit->au_SectorsC * cyl;

    unit->au_DevType = DG_DIRECT_ACCESS;
    unit->au_SecShift = 9;
    unit->au_SectSize = 512;

    FreeMem(empty, 512);
}

/* Copy string and fix it */
void strcp(char *dest, char *src, int num)
{
    while (num)
    {
        *(char*)dest++ = *(char*)(src+1);
        *(char*)dest++ = *(char*)src++;
        src++;
        num -= 2;
    }
    dest--;
    /* Now, go back till got something other than \0 or space */
    while ((*dest == 0) || (*dest == ' '))
    {
        /* Replace it with '\0' */
        *(char*)dest-- = 0;
    }
}

struct ide_Unit *InitUnit(ULONG num, struct ideBase *ib)
{
    struct ide_Unit     *unit;

    /* Try to get memory for structure */
    unit = AllocMem(sizeof(struct ide_Unit), MEMF_PUBLIC | MEMF_CLEAR);

    if (unit)
    {
        UBYTE   dev;
        
        /* Init structure */
        unit->au_UnitNumber = num;
        unit->au_Device     = ib;
        unit->au_ReadSub    = &ata_Read;
        unit->au_WriteSub   = &ata_Write;

        NEWLIST(&unit->au_SoftList);

        dev = ib->ide_DevMaskArray[num];

        unit->au_NumLoop = ATA_TimeOut * 8;

        if (dev & ATAF_ATAPI)
        {
            unit->au_Flags |= AF_LBAMode | AF_AtapiDev;
            unit->au_NumLoop <<= 1;
        }

	/* We need to set some bits to 1 to keep bochs happy */
	dev |= 0xa0;

        unit->au_DevMask    = dev & 0xf0;
        unit->au_OldDevMask = dev & 0x70;
        unit->au_CtrlNumber = dev & 0x0f;
           
        unit->au_PortAddr   = ib->ide_BoardAddr[num];

        /* Fill structure with drive parameters */
        UnitInfo(unit);
        
        ib->ide_Units[num] = unit;
    }

    return unit;
}

/*
  Attempt to calulate the geometry of the harddisk
  This will use the MBR mainly to dig out how other
  OSes see the drive.

  If there is no valid partition table, we will use
  the HDDs reported CHS but increase the number of
  cylinders to match the size of the drive
*/
void CalculateGeometry(struct ide_Unit *unit, struct iDev *id)
{
   UBYTE *mbr;
   ULONG CHSCapacity,LBACapacity,MBRCapacity,MAXCapacity;
   ULONG MBRCyls,MBRHeads,MBRSecs;
   struct PartEntry *pe;

   /* For now we will only deal with fixed harddisks */
   if ( (unit->au_DevType != DG_DIRECT_ACCESS) && !(unit->au_Flags & AF_Removable) )
      return;

   /* For starters, assume the drive is smaller then 8Gb */
   unit->au_Cylinders = id->idev_Cylinders;
   unit->au_Heads     = id->idev_Heads;
   unit->au_SectorsT  = id->idev_Sectors;
   unit->au_SectorsC  = id->idev_Sectors * id->idev_Heads;
   unit->au_Blocks    = id->idev_Cylinders * unit->au_SectorsC;

   D(bug("-Drive says PCHS capacity is %d (%d MB)\n",unit->au_Blocks,unit->au_Blocks/2048));

   /* If we are capable of LBA, get LBA size */
   if (unit->au_Flags & AF_LBAMode)
   {
      LBACapacity = id->ideva_LBASectors;
      D(bug("-Drive says  LBA capacity is %d (%d MB)\n",LBACapacity,LBACapacity/2048));
   }
   else
      LBACapacity = 0;

   /* If the translated CHS fields is valid, get them */
   if (id->idev_NextAvail & ATAF_AVAIL_TCHS)
   {
      CHSCapacity = ((id->ideva_Capacity1)+(id->ideva_Capacity2 << 16));
      D(bug("-Drive says LCHS capacity is %d (%d MB)\n",CHSCapacity,CHSCapacity/2048));
   }
   else
      CHSCapacity = 0;

   /* Which of these is the biggest? */
   MAXCapacity = unit->au_Blocks;
   if (CHSCapacity > MAXCapacity) MAXCapacity = CHSCapacity;
   if (LBACapacity > MAXCapacity) MAXCapacity = LBACapacity;

   /* If cylcount is larger than 1024, attempt to adjust */
   if (unit->au_Cylinders > 1023)
   {
       while ((unit->au_Cylinders > 1024)&(unit->au_Heads<=128))
       {
	   unit->au_Cylinders = (unit->au_Cylinders) >> 1;
	   unit->au_Heads = (unit->au_Heads) << 1;
       }
       if (unit->au_Heads > 255)
       {
	   /* This needs some more massage */
	   unit->au_Heads = 255;
	   unit->au_Cylinders = ( (MAXCapacity)/(unit->au_Heads*unit->au_SectorsT))-1;
       }
   }

   /* Whew, finally done here */
   D(bug("-Using L-CHS %d/%d/%d with the size %d blocks (%d MB)\n",
         unit->au_Cylinders,
         unit->au_Heads,
         unit->au_SectorsT,
         unit->au_Blocks,unit->au_Blocks/2048));
}   

void UnitInfo(struct ide_Unit *unit)
{
    struct iDev     id;
    UBYTE           tmp;

    unit->au_RDBSector = 0xff;  /* Invalidate RDB sector */

    /* Try to identify drive */
    if (ata_Identify(&id, unit))
    {
        /* Copy name/version/serial */
        strcp(&unit->au_ModelID[0], &id.idev_ModelNumber[0], 32);
        strcp(&unit->au_RevNumber[0], &id.idev_RevisionNumber[0], 4);
        strcp(&unit->au_SerNumber[0], &id.idev_SerialNumber[0], 8);

        /* Are we LBA capable */
        if (id.idev_Features & 0x0100)
        {
            unit->au_Flags |= AF_LBAMode;
            unit->au_DevMask |= ATAF_LBA;
        }
        
        D(bug("IDE Unit: %s, using %s mode\n",&unit->au_ModelID[0],
                    ((unit->au_Flags & AF_LBAMode) ? "LBA":"CHS")));
        
        /* is drive removable? */
        if (id.idev_DInfo & 0x80)
        {
            unit->au_Flags |= AF_Removable;
            unit->au_SenseKey = 6;          /* Unit attention */
        }

        /* if Atapi then get drive type */
        tmp = (id.idev_DInfo >> 8) & 0x0f;
        if (!(unit->au_Flags & AF_AtapiDev))
            tmp = DG_DIRECT_ACCESS;     /* Set type HDD otherwise */

        unit->au_DevType = tmp;

        /* If DG_DIRECT_ACCESS then sector is 512 bytes long */
        if(!tmp)
        {
            unit->au_SecShift = 9;
        }
        else    /* Non DG_DIRECT_ACCESS drive - sector is 2048 bytes long */
        {
            unit->au_SecShift = 11;
        }

        unit->au_SectSize = 1 << unit->au_SecShift;

        switch (unit->au_DevType)
        {
            case DG_DIRECT_ACCESS:
                unit->au_Flags |= AF_DiskPresent;
                CalculateGeometry(unit,&id);
                if (unit->au_DevType == DG_DIRECT_ACCESS)
                    AddVolume(0, unit->au_Cylinders, unit);
                break;
            case DG_CDROM:
                AddVolume(0,0,unit);
                break;
            default:
                D(bug("IDE: Unknown/unhandled unit\n"));
                break;
        }
        return;
    }
}

/* Take a small coffebreak */
void Delay400ns( void )
{
   /* Ugly but works */
   ide_in(ata_Control,0x1f0);
   ide_in(ata_Control,0x1f0);
   ide_in(ata_Control,0x1f0);
   ide_in(ata_Control,0x1f0);
}

/* Perform a softreset on an ATA port */
void ResetBus(struct ide_Bus *bus)
{
   UWORD port;
   UBYTE status;
   
   port = bus->ib_Port;
   
   /* Set device parameters */
   ide_out(0x0a,ata_Control,port);
   
   /* Perform the actual resetting */
   ide_out(0x0e,ata_Control,port);
   Delay400ns();
   ide_out(0x0a,ata_Control,port);
   Delay400ns();
   
   /* Wait for dev0 to come online */
   if ( bus->ib_Dev0 != IDE_DEVTYPE_NONE )
   {
      while (1)
      {
         status = ide_in(ata_Status,port);
         if ((status & ATAF_BUSY) == 0) break;
      }
   }
   
   /* Wait for dev1 to come online */
   if ( bus->ib_Dev1 != IDE_DEVTYPE_NONE )
   {
      ide_out(0xb0,ata_DevHead,port);
      Delay400ns();
      while(1)
      {
         status = ide_in(ata_Status,port);
         if ((status & ATAF_BUSY) == 0) break;
      }
   }
}

/* Scan an ATA bus for devices */
void ScanBus(struct ide_Bus *bus)
{
    UWORD port;
    UBYTE secnum,seccnt,status;
    
    port = bus->ib_Port;
    
    /* We assume there are no devices */
    bus->ib_Dev0 = IDE_DEVTYPE_NONE;
    bus->ib_Dev1 = IDE_DEVTYPE_NONE;

    /* Set device parameters (We are not using IRQs yet) */
    ide_out(0x0a,ata_Control,port);

    /* Is there an device 0? */
    ide_out(0xa0,ata_DevHead,port);
    Delay400ns();
    ide_out(0x55,ata_SectorCnt,port);
    ide_out(0xaa,ata_SectorNum,port);
    ide_out(0xaa,ata_SectorCnt,port);
    ide_out(0x55,ata_SectorNum,port);
    ide_out(0x55,ata_SectorCnt,port);
    ide_out(0xaa,ata_SectorNum,port);
    secnum = ide_in(ata_SectorNum,port);
    seccnt = ide_in(ata_SectorCnt,port);
    if ( (secnum == 0xaa) && (seccnt == 0x55) )
        bus->ib_Dev0 = IDE_DEVTYPE_UNKNOWN;

    /* Is there an device 1? */
    ide_out(0xb0,ata_DevHead,port);
    Delay400ns();
    ide_out(0x55,ata_SectorCnt,port);
    ide_out(0xaa,ata_SectorNum,port);
    ide_out(0xaa,ata_SectorCnt,port);
    ide_out(0x55,ata_SectorNum,port);
    ide_out(0x55,ata_SectorCnt,port);
    ide_out(0xaa,ata_SectorNum,port);
    secnum = ide_in(ata_SectorNum,port);
    seccnt = ide_in(ata_SectorCnt,port);
    if ( (secnum == 0xaa) && (seccnt == 0x55) )
        bus->ib_Dev1 = IDE_DEVTYPE_UNKNOWN;

    /* OK, we have a rough idea. Lets reset the bus and scan again */
    ide_out(0xa0,ata_DevHead,port);
    Delay400ns();
    ResetBus(bus);

    /* Is there really a device 0? */
    ide_out(0xa0,ata_DevHead,port);
    Delay400ns();
    seccnt = ide_in(ata_SectorCnt,port);
    secnum = ide_in(ata_SectorNum,port);
    if ( (seccnt == 0x01) && (secnum == 0x01) )
    {
        bus->ib_Dev0 = IDE_DEVTYPE_UNKNOWN;
        seccnt = ide_in(ata_CylinderL,port);
        secnum = ide_in(ata_CylinderH,port);
        status = ide_in(ata_Status,port);
        if ( (seccnt == 0x14) && (secnum == 0xeb) )
        {
            bus->ib_Dev0 = IDE_DEVTYPE_ATAPI;
        }
        else
        {
            /* Clear the error flag in the status message */
            status &= 0xfe;
            if ( (seccnt == 0x00) && (secnum == 0x00) && (status != 0x00) )
                bus->ib_Dev0 = IDE_DEVTYPE_ATA;
        }
    }

    /* Is there really a device 1? */
    ide_out(0xb0,ata_DevHead,port);
    Delay400ns();
    seccnt = ide_in(ata_SectorCnt,port);
    secnum = ide_in(ata_SectorNum,port);
    if ( (seccnt == 0x01) && (secnum == 0x01) )
    {
        bus->ib_Dev1 = IDE_DEVTYPE_UNKNOWN;
        seccnt = ide_in(ata_CylinderL,port);
        secnum = ide_in(ata_CylinderH,port);
        status = ide_in(ata_Status,port);
        if ( (seccnt == 0x14) && (secnum == 0xeb) )
        {
            bus->ib_Dev1 = IDE_DEVTYPE_ATAPI;
        }
        else
        {
            /* Clear the error flag in the status message */
            status &= 0xfe;
            if ( (seccnt == 0x00) && (secnum == 0x00) && (status != 0x00) )
                bus->ib_Dev1 = IDE_DEVTYPE_ATA;
        }
    }
}


/**** ATAPI commands section *************************************************/

ULONG atapi_ErrCmd()
{
    return CDERR_ABORTED;
}

ULONG ErrorMap[] = {
        CDERR_NotSpecified,         // NO SENSE
        CDERR_NoSecHdr,             // RECOVERED ERROR
        CDERR_NoDisk,               // NOT READY
        CDERR_NoSecHdr,             // MEDIUM ERROR
        CDERR_NoSecHdr,             // HARDWARE ERROR
        CDERR_NOCMD,                // ILLEGAL REQUEST
        CDERR_NoDisk,               // UNIT ATTENTION
        CDERR_WriteProt,            // DATA PROTECT
        CDERR_NotSpecified,         // Reserved
        CDERR_NotSpecified,         // Reserved
        CDERR_NotSpecified,         // Reserved
        CDERR_ABORTED,              // ABORTED COMMAND
        CDERR_NotSpecified,         // Reserved
        CDERR_NotSpecified,         // Reserved
        CDERR_NoSecHdr,             // MISCOMPARE
        CDERR_NotSpecified          // Reserved
};

ULONG atapi_EndCmd(struct ide_Unit *unit, ULONG port)
{
    unit->au_Flags |= AF_Used;
    if (!(ide_in(atapi_Status, port) & ATAPIF_CHECK))
        return 0;

    return ErrorMap[ide_in(atapi_Error, port) >> 4];
}

ULONG atapi_TestUnit(struct ide_Unit *unit)
{
    ULONG port;
    ULONG cmd[3] = {0,0,0};
    UBYTE sense;
    
    port = unit->au_PortAddr;

    if (SendPacket(unit, port, &cmd))
    {
        if (WaitBusySlow(port, unit))
        {
            sense = ide_in(ata_Error, port) >> 4;
            if (sense)
                unit->au_SenseKey = sense;
            return sense;
        }
    }
    return atapi_ErrCmd();
}

ULONG atapi_Read(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *cnt)
{
    ULONG                   port;
    struct atapi_Read10     cmd = {};
    
    *cnt = 0;
    port = unit->au_PortAddr;
    cmd.opcode  = SCSI_READ10;
    cmd.LBA[0]  = block >> 24;
    cmd.LBA[1]  = block >> 16;
    cmd.LBA[2]  = block >> 8;
    cmd.LBA[3]  = block;
    cmd.Len[0]  = count >> 8;
    cmd.Len[1]  = count;

    if (SendPacket(unit, port, &cmd))
    {
        while (1)
        {
            if (WaitBusySlow(port, unit))
            {
                if (ide_in(atapi_Status, port) & ATAF_DATAREQ)
                {
                    ULONG size;
                
                    if ((ide_in(atapi_Reason, port) & ATAPIF_MASK) != ATAPIF_READ)
                        return atapi_ErrCmd();
                    size = ide_in(atapi_ByteCntH, port) << 8 |
                           ide_in(atapi_ByteCntL, port);
                    insw(port, buffer, size / 2);
                    
                    buffer += size;
                    *cnt += size;
                } else return atapi_EndCmd(unit, port);
            } else return atapi_ErrCmd();
        }
    }
    return atapi_ErrCmd();
}

ULONG atapi_Write(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *cnt)
{
    ULONG                   port;
    struct atapi_Write10    cmd = {};
    
    *cnt = 0;
    port = unit->au_PortAddr;
    cmd.opcode  = SCSI_WRITE10;
    cmd.LBA[0]  = block >> 24;
    cmd.LBA[1]  = block >> 16;
    cmd.LBA[2]  = block >> 8;
    cmd.LBA[3]  = block;
    cmd.Len[0]  = count >> 8;
    cmd.Len[1]  = count;
    
    if (SendPacket(unit, port, &cmd))
    {
        while (1)
        {
            if (WaitBusySlow(port, unit))
            {
                if (ide_in(atapi_Status, port) & ATAF_DATAREQ)
                {
                    ULONG size;

                    if ((ide_in(atapi_Reason, port) & ATAPIF_MASK) != ATAPIF_WRITE)
                        return atapi_ErrCmd();

                    size = ide_in(atapi_ByteCntH, port) << 8 |
                           ide_in(atapi_ByteCntL, port);

                    outsw(port, buffer, size / 2);

                    buffer += size;
                    *cnt += size;
                } else return atapi_EndCmd(unit, port);
            } else return atapi_ErrCmd();
        }
    }
    
    return atapi_ErrCmd();
}

ULONG atapi_Seek(ULONG block, struct ide_Unit *unit)
{
    ULONG                   port;
    struct atapi_Seek10     cmd = {};
    
    port = unit->au_PortAddr;
    cmd.opcode  = SCSI_SEEK10;
    cmd.LBA[0]  = block >> 24;
    cmd.LBA[1]  = block >> 16;
    cmd.LBA[2]  = block >> 8;
    cmd.LBA[3]  = block;

    if (SendPacket(unit, port, &cmd))
    {
        if (WaitBusySlow(port, unit))
        {
            return atapi_EndCmd(unit, port);
        }
    }
    
    return atapi_ErrCmd();
}

ULONG atapi_Eject(struct ide_Unit *unit)
{
    ULONG                   port;
    struct atapi_StartStop  cmd = {};

    port = unit->au_PortAddr;
    cmd.opcode  = SCSI_STARTSTOP;
    cmd.immed   = 1;    /* Immediate command */
    cmd.flgs    = ATAPI_SS_EJECT;
    
    if (SendPacket(unit, port, &cmd))
    {
        if (WaitBusySlow(port, unit))
        {
            return atapi_EndCmd(unit, port);
        }
    }
    
    return atapi_ErrCmd();
}

ULONG SendPacket(struct ide_Unit *unit, ULONG port, APTR cmd)
{
    ide_out(unit->au_DevMask, atapi_DriveSel, port);
    if (WaitBusy(port, unit))
    {
        ide_out(0, atapi_Features, port);
        ide_out(0, atapi_ByteCntL, port);
        ide_out(0, atapi_ByteCntH, port);
        ide_out(ATAPI_PACKET, atapi_Command, port);
        if (WaitBusySlow(port, unit))
        {
            if (ide_in(atapi_Status, port) & ATAF_DATAREQ)
            {
                UBYTE stat;
                stat = ide_in(atapi_Reason, port);
                stat &= ATAPIF_MASK;
                if (stat == ATAPIF_COMMAND)
                {
                    outsw(port, cmd, 6);
                    return 1;
                }
            }
        }
    }
    return 0;       
}

struct wait { ULONG time; ULONG cnt; };

struct wait WaitTable[] = {
        {   1000, 20 },     //   1 ms x 20
        {   5000, 16 },     //   5 ms x 16
        {  10000, 20 },     //  10 ms x 20
        {  20000, 10 },     //  20 ms x 10
        {  50000, 10 },     //  50 ms x 10
        { 100000, 90 },     // 100 ms x 90
        {      0,  0 }      //------------ = 10000 ms
};

ULONG WaitBusySlow(ULONG port, struct ide_Unit *unit)
{
    int i=1000;
    
    if (unit->au_Flags & AF_SlowDevice)
    {
        int t=0;
        
        do
        {
            if (!(ide_in(ata_Status, port) & ATAF_BUSY))
                return i;
        } while (--i);
        
        while (WaitTable[t].time)
        {
            int                 loop;
            struct timerequest  *tr;

            loop = WaitTable[t].cnt;
            
            tr = unit->au_Device->ide_TimerIO;
            
            while(loop--)
            {
                tr->tr_node.io_Command = TR_ADDREQUEST;
                tr->tr_time.tv_secs = 0;
                tr->tr_time.tv_micro = WaitTable[t].time;
                DoIO((struct IORequest *)tr);
                if (!(ide_in(ata_Status, port) & ATAF_BUSY))
                    return 1;
            }
            t++;
        }
        return 0;
    }
    return WaitBusy(port, unit);
}

/**** ATA commands section ***************************************************/

ULONG ata_ReadSector(ULONG hd, ULONG sec, ULONG cyl, APTR buffer, struct ide_Unit *unit)
{
    ULONG port;
    UBYTE err;

    port = unit->au_PortAddr;

    ide_out(hd | unit->au_OldDevMask, ata_DevHead, port);
    if (WaitBusy(port, unit))
    {
        ide_out(sec, ata_SectorNum, port);
        ide_out(cyl & 0xff, ata_CylinderL, port);
        ide_out(cyl >> 8, ata_CylinderH, port);
        ide_out(1, ata_SectorCnt, port);
        ide_out(ATA_READ, ata_Command, port);
        if (WaitBusy(port, unit))
        {
            err = ide_in(ata_Status, port);
            
            insw(port, buffer, 512 / 2);

            if (!(err & ATAF_ERROR))
            {
                if (!(ide_in(ata_Status, port) & ATAF_DATAREQ))
                {
                    return 1;
                }
            }
        }
    }

    ResumeError(port, unit);
    return 0;
}

ULONG ata_Read(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act)
{
    struct ideBase  *ib;
    ULONG           port;

    ib = unit->au_Device;
    port = unit->au_PortAddr;

    *act = count;

    /* if block addr is valid (less than total blocks) */
    if (block < unit->au_Blocks)
    {
        /* If count is valid (there is something to read) */
        if (count)
        {
            /* Try to set up block addr */
            if (!BlockAddr(block, unit))
                return TDERR_NotSpecified;

            /* Do whole count. Repeat until count >0. At end of each loop
             * increment block address */
            for(;*act;IncBlockAddr(port, unit))
            {
                /* Sector count (0 == 256 sectors) */
                ide_out(count, ata_SectorCnt, port);
                /* Do read! */
                ide_out(ATA_READ, ata_Command, port);

                do
                {
                    /* Wait for completion */
                    if (!WaitBusy(port, unit))
                        return TDERR_NotSpecified;

                    /* Data buffer ready? */
                    if (!(ide_in(ata_Status, port) & ATAF_DATAREQ))
                        return TDERR_NotSpecified;

                    /* Yes. Copy ide buffer */
                    insw(port, buffer, 512 / 2);

                    /* Handle swapped bit here!!!!!!!! */

                    /* Wait for completion */
                    if (!WaitBusy(port, unit))
                        return TDERR_NotSpecified;

                    /* Exit if there was any error */
                    if (ide_in(ata_Status, port) & ATAF_ERROR)
                        return TDERR_NotSpecified;
                /* Loop 'till lobyte of count !=0 */
                } while((--(*act)) & 0xff);
            }
            
            *act = count;
            /* No errors. Return */
            return 0;       
        }
    }
    
    ResumeError(port, unit);
    
    /* Fail. */
    return TDERR_NotSpecified;
}

ULONG ata_Write(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act)
{
    ULONG port;

    port = unit->au_PortAddr;

    *act = count;
    
    if (block < unit->au_Blocks)
    {
        if (count)
        {
            if (!BlockAddr(block, unit))
                return TDERR_NotSpecified;
            
            for (; *act; IncBlockAddr(port, unit))
            {
                ide_out(count, ata_SectorCnt, port);
                ide_out(ATA_WRITE, ata_Command, port);

                /* Handle swapped right now!!!! */

                do
                {
                    if (!WaitBusy(port, unit))
                        return TDERR_NotSpecified;
                    
                    if ((ide_in(ata_Status, port) & (ATAF_ERROR | ATAF_DATAREQ)) !=
                                    ATAF_DATAREQ)
                        return TDERR_NotSpecified;

                    outsw(port, buffer, 512 / 2);
                } while((--(*act)) & 0xff);
                
                if (!WaitBusy(port, unit))
                    return TDERR_NotSpecified;
            }

            if (ide_in(ata_Status, port) & ATAF_ERROR)
                return TDERR_NotSpecified;
            
            *act = count;
            return 0;
        }
    }

    ResumeError(port, unit);
    
    return TDERR_NotSpecified;
}

ULONG ata_Seek(ULONG block, struct ide_Unit *unit)
{
    ULONG port;
    ULONG err;

    port = unit->au_PortAddr;

    if (block < unit->au_Blocks)
    {
        if (BlockAddr(port, unit))
        {
            ide_out(ATA_SEEK, ata_Command, port);

            if (WaitBusy(port, unit))
                if (!ide_in(ata_Status, port) & ATAF_ERROR)
                    return 0;
            
            err = 0x0b02;       /* Aborted command + No seek complete */
            block = ActualAddr(port, unit);
        } else err = 0x0205;    /* Not ready + logical unit not respond */
    } else err = 0x0521;        /* Illegal request + LBA out of range */

    unit->au_LBASense = block;
    unit->au_SenseKey = err;

    return TDERR_NotSpecified;
}

ULONG ata_Eject(struct ide_Unit *unit)
{
    ULONG port;

    port = unit->au_PortAddr;

    ide_out(unit->au_DevMask, ata_DevHead, port);
    if (WaitBusy(port, unit))
    {
        ide_out(ATA_MEDIAEJECT, ata_Command, port);
        if (WaitBusy(port, unit))
        {
            return 0;
        }
    }

    unit->au_LBASense = 0;
    unit->au_SenseKey = 0x0205; /* Not ready + logical unit not respond */
    
    return TDERR_NotSpecified;
}

ULONG ata_Identify(APTR buffer, struct ide_Unit *unit)
{
    ULONG port;
    UBYTE comm;
    
    port = unit->au_PortAddr;

    comm = (unit->au_Flags & AF_AtapiDev) ? ATAPI_IDENTDEV : ATA_IDENTDEV;

    ide_out(unit->au_DevMask, ata_DevHead, port);
    if (WaitBusy(port, unit))
    {
        ide_out(comm, ata_Command, port);
        if (WaitBusy(port, unit))
        {
            if (ide_in(ata_Status, port) & ATAF_DATAREQ)
            {
                struct iDev *id = buffer;

                insw(port, buffer, 512 / 2);

                if (!(ide_in(ata_Status, port) & ATAF_ERROR))
                {
                    if (unit->au_Flags & AF_AtapiDev)
                    {
                        UBYTE type;
                        type = (id->idev_DInfo >> 8) & 0x1f;
                        
                        switch (type)
                        {
                            case DG_CDROM:
                            case DG_WORM:
                            case DG_OPTICAL_DISK:
                                id->idev_Heads = 1;
                                id->idev_Sectors = 75;
                                id->idev_Cylinders = 4440;
                                unit->au_Flags |= AF_SlowDevice;
                                break;

                            case DG_DIRECT_ACCESS:
                                if (!strcmp("LS-120", &id->idev_ModelNumber[0]))
                                {
                                    id->idev_Heads = 2;
                                    id->idev_Sectors = 18;
                                    id->idev_Cylinders = 6848;
                                    unit->au_Flags |= AF_SlowDevice;
                                }
                                else if (!strcmp("ZIP 100 ", &id->idev_ModelNumber[8]))
                                {
                                    id->idev_Heads = 1;
                                    id->idev_Sectors = 64;
                                    id->idev_Cylinders = 3072;
                                    unit->au_Flags &= ~AF_SlowDevice;
                                }
                                break;
                        }
                    }
                    return 1;
                }
            }
        }               
    }

    ResumeError(port, unit);
    return 0;
}

LONG ata_ScsiCmd(struct SCSICmd *scsicmd, struct ide_Unit *unit)
{
    ULONG port;
    UBYTE comm;
    
    port = unit->au_PortAddr;

    comm = scsicmd->scsi_Command[0];

    ide_out(unit->au_DevMask, ata_DevHead, port);
    if (WaitBusy(port, unit))
    {
        ide_out(comm, ata_Command, port);
        if (WaitBusy(port, unit))
        {
            scsicmd->scsi_Status = ide_in(ata_Status, port);
				if (scsicmd->scsi_Status & ATAF_DATAREQ)
            {
                insw(port, scsicmd->scsi_Data, scsicmd->scsi_Length / 2);

                scsicmd->scsi_Status = ide_in(ata_Status, port);
                if (!(scsicmd->scsi_Status & ATAF_ERROR))
                {
                    return 0;
                }
            }
        }               
    }

    ResumeError(port, unit);
    return HFERR_BadStatus;
}
/**** Helper functions ********************************************************/

void ResumeError(ULONG port, struct ide_Unit *unit)
{
    if (unit->au_Flags & AF_AtapiDev)
    {
        ide_out(ATAPI_RESET, ata_Command, port);
    }
    else
    {
        ide_out(ATA_RECALIBRATE, ata_Command, port);
    }
    WaitBusyLong(port, unit);
}

void IncBlockAddr(ULONG port, struct ide_Unit *unit)
{
    if (unit->au_Flags & AF_LBAMode)
    {
        UBYTE tmp;
        tmp = ide_in(ata_SectorNum, port);
        ide_out(++tmp, ata_SectorNum, port);
        if (tmp)
            return;
        tmp = ide_in(ata_CylinderL, port);
        ide_out(++tmp, ata_CylinderL, port);
        if (tmp)
            return;
        tmp = ide_in(ata_CylinderH, port);
        ide_out(++tmp, ata_CylinderH, port);
        if (tmp)
            return;
        tmp = ide_in(ata_DevHead, port);
        ide_out(++tmp, ata_DevHead, port);
    }
    else
    {
        UBYTE tmp;

        tmp = ide_in(ata_SectorNum, port);
        if (tmp == unit->au_SectorsT)
        {
            ide_out(1, ata_SectorNum, port);
            tmp = (ide_in(ata_DevHead, port) && 0x0f) + 1;
            if (tmp == unit->au_Heads)
            {
                ide_out(ide_in(ata_DevHead, port) && 0xf0,
                                ata_DevHead, port);
                tmp = ide_in(ata_CylinderL, port);
                ide_out(++tmp, ata_CylinderL, port);
                if (!tmp)
                    ide_out(ide_in(ata_CylinderH, port) + 1,
                                    ata_CylinderH, port);
            } else ide_out(ide_in(ata_DevHead, port) + 1,
                            ata_DevHead, port);
        }
        else ide_out(++tmp, ata_SectorNum, port);
    }
}

int BlockAddr(ULONG block, struct ide_Unit *unit)
{
    ULONG port = unit->au_PortAddr;
    
    /* LBA mode active? */
    if (unit->au_Flags & AF_LBAMode)
    {
        /* LBA[27..24] + dev select */
        ide_out((block >> 24) | unit->au_DevMask, ata_DevHead, port);
        if (!WaitBusy(port, unit))
            return 0;
        ide_out(block >> 16, ata_CylinderH, port);  /* LBA[23..16] */
        ide_out(block >> 8, ata_CylinderL, port);   /* LBA[15..8] */
        ide_out(block, ata_SectorNum, port);        /* LBA[7..0] */
    }
    /* Use CHS mode instead */
    else
    {
        ULONG sec, cyl, hd;

        sec     =   block;                      /* sec = block */
        cyl     =   sec / unit->au_SectorsC;    /* cyl = real cyn number */
        sec     %=  unit->au_SectorsC;          /* sec = real sec number */
        hd      =   sec / unit->au_SectorsT;    /* hd = real hd number */
        sec     %=  unit->au_SectorsT;          /* sec = sec number -1 */
        sec++;                                  /* End of translation */

        /* We have calculated all needed valuse.
         * Send them right now */
        ide_out(hd | unit->au_DevMask, ata_DevHead, port);
        if (!WaitBusy(port, unit))
            return 0;
        ide_out(sec, ata_SectorNum, port);
        ide_out(cyl, ata_CylinderL, port);
        ide_out(cyl >> 8, ata_CylinderH, port);
    }

    return 1;
}

ULONG ActualAddr(ULONG port, struct ide_Unit *unit)
{
    ULONG block;
    
    if (unit->au_Flags & AF_LBAMode)
    {
        block = ide_in(ata_DevHead, port) & 0x0f;
        block <<= 8;
        block |= ide_in(ata_CylinderH, port);
        block <<= 8;
        block |= ide_in(ata_CylinderL, port);
        block <<= 8;
        block |= ide_in(ata_SectorNum, port);
    }
    else
    {
        block = ide_in(ata_CylinderH, port) << 8 |
                ide_in(ata_CylinderL, port);
        block *= unit->au_SectorsC;
        block += (ide_in(ata_DevHead, port) & 0x0f) * unit->au_SectorsT;
        block += ide_in(ata_SectorNum, port) - 1;
    }
    
    return block;
}

int WaitBusy(ULONG port, struct ide_Unit *unit)
{
    ULONG cnt = unit->au_NumLoop;
    UBYTE status;
    
    do
    {
        status = ide_in(ata_Status, port);
    } while((status & ATAF_BUSY) && --cnt);
    
    return cnt;
}

int WaitBusyLong(ULONG port, struct ide_Unit *unit)
{
    ULONG cnt = unit->au_NumLoop << 2;
    UBYTE status;

    do
    {
        status = ide_in(ata_Status, port);
    } while ((status & ATAF_BUSY) && --cnt);
    
    return cnt;
}
