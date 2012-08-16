/*
#define DEBUG 1
#define DEBUGCODE
*/

#include <devices/scsidisk.h>
#include <devices/trackdisk.h>
#include <dos/filehandler.h>
#include <exec/errors.h>
#include <exec/interrupts.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <devices/newstyle.h>      /* Doesn't include exec/types.h which is why it is placed here */

#ifdef __AROS__
#include <aros/asmcall.h>
#endif

#include "deviceio.h"
#include "deviceio_protos.h"
#include "debug.h"
#include "globals.h"
#include "req_protos.h"

static inline ULONG MULU64(ULONG m1, ULONG m2, ULONG *res_lo)
{
    unsigned long long res = (UQUAD)m1 * (UQUAD)m2;

    *res_lo = res;
    return res >> 32;
}

extern LONG getbuffer(UBYTE **tempbuffer, ULONG *maxblocks);
extern void starttimeout(void);

/* The purpose of the deviceio code is to provide a layer which
   supports block I/O to devices:

   - Devices are accessed based on Logical Blocks (The partition
     offset and SectorsPerBlock are taken into account).

   - Devices can be acessed normally or using NSD, TD64 or SCSI
     direct transparently.

   - MaxTransfer and Mask is handled automatically.

   - Makes sure no accesses outside the partition are allowed. */


void update(void)
{
    _TDEBUG(("UPDATE\n"));
    globals->ioreq->io_Command=CMD_UPDATE;
    globals->ioreq->io_Length=0;
    DoIO((struct IORequest *)globals->ioreq);
}

void motoroff(void)
{
    _TDEBUG(("MOTOR OFF\n"));
    globals->ioreq->io_Command=TD_MOTOR;
    globals->ioreq->io_Length=0;
    DoIO((struct IORequest *)globals->ioreq);
}

static UBYTE *errordescription(LONG errorcode)
{
    if(errorcode==TDERR_NotSpecified || errorcode==TDERR_SeekError) {
        return("General device failure.\nThe disk is in an unreadable format.\n\n");
    }
    else if(errorcode==TDERR_BadSecPreamble || errorcode==TDERR_BadSecID || errorcode==TDERR_BadSecHdr || errorcode==TDERR_BadHdrSum || errorcode==TDERR_BadSecSum) {
        return("Invalid or bad sector.\nThe disk is in an unreadable format.\n\n");
    }
    else if(errorcode==TDERR_TooFewSecs || errorcode==TDERR_NoSecHdr) {
        return("Not enough sectors found.\nThe disk is in an unreadable format.\n\n");
    }
    else if(errorcode==TDERR_NoMem) {
        return("The device ran out of memory.\n\n");
    }
    else if(errorcode==HFERR_SelfUnit) {
        return("\n\n");
    }
    else if(errorcode==HFERR_DMA) {
        return("DMA error.\nThe transfering of data to/from the device failed.\n\n");
    }
    else if(errorcode==HFERR_Parity) {
        return("Parity error.\nThe transfering of data to/from the device failed.\n\n");
    }
    else if(errorcode==HFERR_SelTimeout) {
        return("Selection timeout.\nThe device doesn't respond.\n\n");
    }
    else if(errorcode==HFERR_BadStatus || errorcode==HFERR_Phase) {
        return("General device failure.\nThe device is in an invalid state.\n\n");
    }

    return("Unknown error.\n\n");
}

LONG writeprotection(void)
{
    globals->ioreq2->io_Command=TD_PROTSTATUS;
    globals->ioreq2->io_Flags=IOF_QUICK;

    if(DoIO((struct IORequest *)globals->ioreq2)==0)
    {
        if(globals->ioreq2->io_Actual==0) {
            /* Not write protected */
            return(ID_VALIDATED);
        }
    }
    /* Write protected */
    return(ID_WRITE_PROTECTED);
}

UBYTE isdiskpresent(void)
{
    LONG errorcode;

    globals->ioreq2->io_Command=TD_CHANGESTATE;
    globals->ioreq2->io_Flags=IOF_QUICK;

    if((errorcode=DoIO((struct IORequest *)globals->ioreq2))==0)
    {
        if(globals->ioreq2->io_Actual==0) {
            /* Disk present */
            return(TRUE);
        }
        else
        {
            /* No disk inserted */
            return(FALSE);
        }
    }

    req("This device doesn't support disk change detection!\n"\
        "(errorcode = %ld).  SFS will now assume that a\n"\
        "disk IS present.", "Ok", errorcode);

    /* If anything went wrong, we assume a disk is inserted. */
    return(TRUE);
}

#ifdef __AROS__
static AROS_INTH1(changeintserver, struct IntData *, intdata)
{
    AROS_INTFUNC_INIT
 
    *intdata->diskchanged=1;
    Signal(intdata->task, intdata->signal);
    return(0);
     
    AROS_INTFUNC_EXIT
}
#else
static LONG __interrupt __saveds __asm changeintserver(register __a1 struct IntData *intdata)
{
    *intdata->diskchanged=1;
    Signal(intdata->task, intdata->signal);
    return(0);
}
#endif

LONG addchangeint(struct Task *task, ULONG signal)
{
    globals->intdata.diskchanged=&globals->diskchanged;
    globals->intdata.task=task;
    globals->intdata.signal=signal;

    globals->changeint.is_Data=&globals->intdata;
    globals->changeint.is_Code=(VOID_FUNC)changeintserver;

    globals->ioreqchangeint->io_Command=TD_ADDCHANGEINT;
    globals->ioreqchangeint->io_Flags=0;
    globals->ioreqchangeint->io_Length=sizeof(struct Interrupt);
    globals->ioreqchangeint->io_Data=&globals->changeint;
    globals->ioreqchangeint->io_Error=0;

    SendIO((struct IORequest *)globals->ioreqchangeint);

    return(globals->ioreqchangeint->io_Error);
}

void removechangeint(void)
{
    if(globals->ioreqchangeint!=0 && globals->ioreqchangeint->io_Error==0)
    {
        globals->ioreqchangeint->io_Command=TD_REMCHANGEINT;
        SendIO((struct IORequest *)globals->ioreqchangeint);
    }
}

ULONG getchange(void)
{
    ULONG d;
    
    Disable();
    d=globals->diskchanged;
    globals->diskchanged=0;
    Enable();

    return(d);
}

ULONG deviceapiused(void)
{
    if(globals->scsidirect==TRUE) {
        return(DAU_SCSIDIRECT);
    }

    if(globals->newstyledevice==TRUE) {
        return(DAU_NSD);
    }

    if(globals->does64bit==TRUE) {
        return(DAU_TD64);
    }

    return(DAU_NORMAL);
}

LONG getgeometry(ULONG *sectors, ULONG *sectorsize)
{
    struct DriveGeometry dg;
    LONG errorcode;

    if((!strcmp(globals->ioreq->io_Device->dd_Library.lib_Node.ln_Name, "scsi.device") && globals->ioreq->io_Device->dd_Library.lib_Version<=39) ||
       !strcmp(globals->ioreq->io_Device->dd_Library.lib_Node.ln_Name, "HardFrame.device") ||
       !strcmp(globals->ioreq->io_Device->dd_Library.lib_Node.ln_Name, "scsidev.device") ||
       !strcmp(globals->ioreq->io_Device->dd_Library.lib_Node.ln_Name, "hddisk.device") ||
       !strcmp(globals->ioreq->io_Device->dd_Library.lib_Node.ln_Name, "statram.device") ||
       !strcmp(globals->ioreq->io_Device->dd_Library.lib_Node.ln_Name, "ramdrive.device")) {
        return(IOERR_NOCMD);
    }

    globals->ioreq->io_Command=TD_GETGEOMETRY;
    globals->ioreq->io_Length=sizeof(struct DriveGeometry);
    globals->ioreq->io_Flags=0;
    globals->ioreq->io_Actual=0;
    globals->ioreq->io_Data=&dg;

    dg.dg_SectorSize=0;

    if((errorcode=DoIO((struct IORequest *)globals->ioreq))==0)
    {
        if(dg.dg_SectorSize==512 || dg.dg_SectorSize==1024 || dg.dg_SectorSize==2048 || dg.dg_SectorSize==4096 || dg.dg_SectorSize==8192 || dg.dg_SectorSize==16384 || dg.dg_SectorSize==32768) {
            *sectors=dg.dg_TotalSectors;
            *sectorsize=dg.dg_SectorSize;
        }
        else {
            errorcode=IOERR_NOCMD;
        }
    }

    return(errorcode);
}

void changegeometry(struct DosEnvec *de)
{
    ULONG sectorspercilinder;
    UWORD bs;

    globals->sectors_block = de->de_SectorPerBlock;
    globals->bytes_sector  = de->de_SizeBlock<<2;
    globals->bytes_block   = globals->bytes_sector * de->de_SectorPerBlock;

    _DEBUG(("%u sectors per block, %u bytes per sector, %u bytes per block\n", globals->sectors_block, globals->bytes_sector, globals->bytes_block));

    bs = globals->bytes_block;

    globals->shifts_block = 0;
    while ((bs >>= 1) !=0 )
    {
        globals->shifts_block++;
    }

    globals->mask_block = globals->bytes_block-1;
    
    _DEBUG(("Block shift: %u, mask: 0x%08X\n", globals->shifts_block, globals->mask_block));

    /* Absolute offset on the entire disk are expressed in Sectors;
       Offset relative to the start of the partition are expressed in Blocks */

    sectorspercilinder = de->de_Surfaces*de->de_BlocksPerTrack;           // 32 bit

    /* Get bounds of our device */
    globals->sector_low  = sectorspercilinder * de->de_LowCyl;
    globals->sector_high = sectorspercilinder * (de->de_HighCyl+1);

    _DEBUG(("%u sectors per cylinder, start sector %u, end sector %u\n", sectorspercilinder, globals->sector_low, globals->sector_high));

    /*
     * If our device starts from sector 0, we assume we are serving the whole device.
     * In this case this can be a removable drive, and we may need to query the drive
     * about its current geometry (this way we can support floppies where geometry may
     * vary (DD or HD)
     */
    if (globals->sector_low == 0)
    {
        ULONG totalsectors=0,sectorsize=0;

        if(getgeometry(&totalsectors, &sectorsize)!=0) {
            totalsectors=0;
        }

        if(totalsectors!=0)
        {
            D(bug("[SFS] Given Sector size %lu, detected %lu\n", sectorsize, globals->bytes_sector));
            if(sectorsize==globals->bytes_sector) {
                globals->sector_high=totalsectors;
            }
            else {
                req("Geometry could not be used because\n"\
                    "sectorsize does not match that of the\n"\
                    "mountlist (%ld<>%ld).\n"\
                    "Please notify the author.", "Continue", sectorsize, globals->bytes_sector);
            }
        }
    }

    /* Set some more characteristics */
    globals->sectors_total = globals->sector_high - globals->sector_low;
    globals->blocks_total  = globals->sectors_total / globals->sectors_block;
    globals->byte_low      = (UQUAD)globals->sector_low  * globals->bytes_sector;
    globals->byte_high     = (UQUAD)globals->sector_high * globals->bytes_sector;

    _DEBUG(("Total: %u sectors, %u blocks\n", globals->sectors_total, globals->blocks_total));
    _DEBUG(("Start offset 0x%llu, end offset 0x%llu\n", globals->byte_low, globals->byte_high));
}

#ifdef DEBUGCODE
static struct fsIORequest *createiorequest(void)
{
    struct fsIORequest *fsi;

    if((fsi=AllocMem(sizeof(struct fsIORequest), MEMF_CLEAR))!=0)
    {
        if((fsi->ioreq=(struct IOStdReq *)CreateIORequest(globals->msgport, sizeof(struct IOStdReq)))!=0)
        {
            fsi->ioreq->io_Device=globals->ioreq->io_Device;
            fsi->ioreq->io_Unit=globals->ioreq->io_Unit;
        }
        else
        {
            FreeMem(fsi, sizeof(struct fsIORequest));
        }
    }

    return(fsi);
}
#endif

LONG initdeviceio(UBYTE *devicename, IPTR unit, ULONG flags, struct DosEnvec *de)
{
    LONG errorcode=ERROR_NO_FREE_STORE;

    if((globals->msgport=CreateMsgPort())!=0) {
        if((globals->ioreq=(struct IOStdReq *)CreateIORequest(globals->msgport, sizeof(struct IOStdReq)))!=0) {
            if((globals->ioreq2=(struct IOStdReq *)CreateIORequest(globals->msgport, sizeof(struct IOStdReq)))!=0) {
                if((globals->ioreqchangeint=(struct IOStdReq *)CreateIORequest(globals->msgport, sizeof(struct IOStdReq)))!=0) {
                    if((errorcode=OpenDevice(devicename, unit, (struct IORequest *)globals->ioreq, flags))==0) {
                        globals->deviceopened=TRUE;

                        globals->fsioreq.ioreq=globals->ioreq;

                        globals->ioreq2->io_Device=globals->ioreq->io_Device;
                        globals->ioreq2->io_Unit=globals->ioreq->io_Unit;

                        globals->ioreqchangeint->io_Device=globals->ioreq->io_Device;
                        globals->ioreqchangeint->io_Unit=globals->ioreq->io_Unit;

                        changegeometry(de);

                        if(de->de_TableSize>=12) {
                            globals->bufmemtype=de->de_BufMemType;

                            if(de->de_TableSize>=13) {
                                globals->blocks_maxtransfer=de->de_MaxTransfer>>globals->shifts_block;

                                if(globals->blocks_maxtransfer==0) {
                                    globals->blocks_maxtransfer=1;
                                }

                                if(de->de_TableSize>=14) {
                                    globals->mask_mask=de->de_Mask;
                                }
                            }
                        }

                        /* If the partition's high byte is beyond the 4 GB border we will
                           try and detect a 64-bit device. */

                        if (globals->byte_high >> 32 != 0)
                        {
                            /* Check for 64-bit support (NSD/TD64/SCSI direct) */
                            struct NSDeviceQueryResult nsdqr;
                            UWORD *cmdcheck;

                            nsdqr.SizeAvailable=0;
                            nsdqr.DevQueryFormat=0;

                            globals->ioreq->io_Command=NSCMD_DEVICEQUERY;
                            globals->ioreq->io_Length=sizeof(nsdqr);
                            globals->ioreq->io_Data=(APTR)&nsdqr;

                            if(DoIO((struct IORequest *)globals->ioreq)==0 && (globals->ioreq->io_Actual >= 16) && (nsdqr.SizeAvailable == globals->ioreq->io_Actual) && (nsdqr.DeviceType == NSDEVTYPE_TRACKDISK)) {
                                /* This must be a new style trackdisk device */

//                              req("New style device", "Ok");

                                globals->newstyledevice=TRUE;

//                              _DEBUG(("Device is a new style device\n"));

                                /* Is it safe to use 64 bits with this driver?  We can reject
                                   bad mounts pretty easily via this check. */

                                for(cmdcheck=nsdqr.SupportedCommands; *cmdcheck; cmdcheck++) {
                                    if(*cmdcheck == NSCMD_TD_READ64) {
                                        /* This trackdisk style device supports the complete 64-bit
                                           command set without returning IOERR_NOCMD! */

//                                      _DEBUG(("Device supports 64-bit commands\n"));

                                        globals->does64bit=TRUE;
                                        globals->cmdread=NSCMD_TD_READ64;
                                        globals->cmdwrite=NSCMD_TD_WRITE64;
                                    }
                                }
                            }
                            else {  /* Check for TD64 supporting devices */
                                LONG td64errorcode;

                                globals->ioreq->io_Command=24;  /* READ64 */
                                globals->ioreq->io_Length=0;
                                globals->ioreq->io_Actual=0;
                                globals->ioreq->io_Offset=0;
                                globals->ioreq->io_Data=0;

                                td64errorcode=DoIO((struct IORequest *)globals->ioreq);
                                if(td64errorcode!=-1 && td64errorcode!=IOERR_NOCMD) {
//                                  req("TD64 device", "Ok");

                                    globals->does64bit=TRUE;
                                    globals->cmdread=24;
                                    globals->cmdwrite=25;
                                }
                                else {  /* Check for SCSI direct supporting devices */
                                    UWORD *destbuffer;
    
                                    if((destbuffer=AllocVec(globals->bytes_sector,globals->bufmemtype))!=0) {
                                        globals->ioreq->io_Command=HD_SCSICMD;
                                        globals->ioreq->io_Length=sizeof(struct SCSICmd);
                                        globals->ioreq->io_Data=&globals->scsicmd;
    
                                        globals->scsicmd.scsi_Data=(UWORD *)destbuffer;
                                        globals->scsicmd.scsi_Length=globals->bytes_sector;
                                        globals->scsicmd.scsi_Command=(UBYTE *)&globals->scsi10cmd;
                                        globals->scsicmd.scsi_CmdLength=sizeof(struct SCSI10Cmd);
    
                                        globals->scsicmd.scsi_Flags=SCSIF_READ;
    
                                        globals->scsicmd.scsi_SenseData=NULL;
                                        globals->scsicmd.scsi_SenseLength=0;
                                        globals->scsicmd.scsi_SenseActual=0;
                    
                                        globals->scsi10cmd.Opcode=SCSICMD_READ10;
                                        globals->scsi10cmd.Lun=0;
                                        globals->scsi10cmd.LBA=globals->sector_low;
                                        globals->scsi10cmd.Reserved=0;
                                        globals->scsi10cmd.Control=0;
    
                                        {
                                            UWORD *ptr=(UWORD *)&globals->scsi10cmd.Length[0];               
                                            //  scsi10cmd.Length=blocks*sectors_block;       /* Odd aligned... */
                                            *ptr=1;
                                        }
    
                                        if((td64errorcode=DoIO((struct IORequest *)globals->ioreq))==0) {
    //                                      req("SCSI direct command returned no error.", "Ok");
    
                                            globals->does64bit=TRUE;
                                            globals->scsidirect=TRUE;
                                            globals->cmdread=SCSICMD_READ10;
                                            globals->cmdwrite=SCSICMD_WRITE10;
                                        }
    
                                        FreeVec(destbuffer);
                                    }
                                    else {
    //                                  req("Not enough memory.", "Ok");
                                        errorcode=ERROR_NO_FREE_STORE;
                                    }
                                }
                            }
    
                            if(globals->does64bit==FALSE) {
                                errorcode=ERROR_NO_64BIT_SUPPORT;
    /*                          req("There is no 64-bit support, but your\n"\
                                    "partition was (partially) located above 4 GB.", "Ok");
    */
                            }
                        }
                    }
                }
            }
        }
    }

    if(errorcode!=0) {
        cleanupdeviceio();
    }

    return(errorcode);
}

void cleanupdeviceio()
{
    if(globals->deviceopened) {
        CloseDevice((struct IORequest *)globals->ioreq);
    }

    if(globals->ioreq!=0) {
        DeleteIORequest((struct IORequest *)globals->ioreq);
    }

    if(globals->ioreq2!=0) {
        DeleteIORequest((struct IORequest *)globals->ioreq2);
    }

    if(globals->ioreqchangeint!=0) {
        DeleteIORequest((struct IORequest *)globals->ioreqchangeint);
    }

    if(globals->msgport!=0) {
        DeleteMsgPort(globals->msgport);
    }
}

LONG handleioerror(LONG errorcode, UWORD action, struct IOStdReq *ioreq)
{
    if(errorcode==TDERR_DiskChanged) {
        if(req("You MUST reinsert this volume!\n"\
               "There still was some data which needs to be transferred.", "Retry|Cancel")<=0) {
            return(errorcode);
        }
    }
    else if(errorcode==TDERR_WriteProt || (action==DIO_WRITE && writeprotection()==ID_WRITE_PROTECTED)) {
        if(req("This volume is write protected.", "Retry|Cancel")<=0) {
            return(ERROR_DISK_WRITE_PROTECTED);
        }
    }
    else {
        if(globals->retries-->0) {
            // Delay(2);
            return(0);
        }

        if(req("There was an error while accessing this volume:\n\n%s"\
               "errorcode = %ld\n"\
               "io_Command = %ld\n"\
               "io_Offset = %ld\n"\
               "io_Length = %ld\n"\
               "io_Actual = %ld\n", "Retry|Cancel",
               errordescription(errorcode), errorcode,
               (ULONG)ioreq->io_Command, ioreq->io_Offset,
               ioreq->io_Length, ioreq->io_Actual)<=0) {
            return(errorcode);
        }
    }

    return(0);
}

void setiorequest(struct fsIORequest *fsi, UWORD action, UBYTE *buffer, ULONG blockoffset, ULONG blocks)
{
    struct IOStdReq *ioreq=fsi->ioreq;

    _DEBUG(("setiorequest(0x%p, %u, %u)\n", buffer, blockoffset, blocks));

    fsi->next=0;
    fsi->action=action;

    _DEBUG(("Use DirectSCSI: %d\n", globals->scsidirect));

    if (globals->scsidirect==TRUE)
    {
        ioreq->io_Command=HD_SCSICMD;
        ioreq->io_Length=sizeof(struct SCSICmd);
        ioreq->io_Data=&fsi->scsicmd;

        fsi->scsicmd.scsi_Data        = (UWORD *)buffer;
        fsi->scsicmd.scsi_Length      = blocks<<globals->shifts_block;
        fsi->scsicmd.scsi_Command     = (UBYTE *)&fsi->scsi10cmd;
        fsi->scsicmd.scsi_CmdLength   = sizeof(struct SCSI10Cmd);
        fsi->scsicmd.scsi_Flags       = (action == DIO_READ) ? SCSIF_READ : 0;
        fsi->scsicmd.scsi_SenseData   = NULL;
        fsi->scsicmd.scsi_SenseLength = 0;
        fsi->scsicmd.scsi_SenseActual = 0;

        fsi->scsi10cmd.Opcode   = action==DIO_WRITE ? globals->cmdwrite : globals->cmdread;       // SCSICMD_READ10 or SCSICMD_WRITE10;
        fsi->scsi10cmd.Lun      = 0;
        fsi->scsi10cmd.LBA      = globals->sector_low+blockoffset*globals->sectors_block;
        fsi->scsi10cmd.Reserved = 0;
        fsi->scsi10cmd.Control  = 0;

        {
            UWORD *ptr=(UWORD *)&fsi->scsi10cmd.Length[0];

            //  scsi10cmd.Length=blocks*sectors_block;       /* Odd aligned... */
            *ptr=blocks*globals->sectors_block;
        }
    }
    else
    {
    	ULONG startblock = globals->sector_low / globals->sectors_block;
    	UQUAD startoffset = (UQUAD)(startblock + blockoffset) << globals->shifts_block;

        ioreq->io_Data    = buffer;
        ioreq->io_Command = action==DIO_WRITE ? globals->cmdwrite : globals->cmdread;
        ioreq->io_Length  = blocks<<globals->shifts_block;
        ioreq->io_Offset  = startoffset;
        ioreq->io_Actual  = startoffset >> 32;
    }
}

LONG transfer_buffered(UWORD action, UBYTE *buffer, ULONG blockoffset, ULONG blocklength)
{
    UBYTE *tempbuffer;
    ULONG maxblocks=globals->blocks_maxtransfer;
    LONG errorcode;

    /* This function does a buffered transfer (in cases when the Mask
       enforces it).  It does not check if the parameters are within
       the partition.  Check those and the Mask before calling this
       function. */

    if ((errorcode = getbuffer(&tempbuffer, &maxblocks)) != 0)
    {
    	_DEBUG(("Buffered transfer: getbuffer() error %d\n", errorcode));

        return errorcode;
    }

    while(blocklength!=0)
    {
        ULONG blocks=blocklength>maxblocks ? maxblocks : blocklength;

        setiorequest(&globals->fsioreq, action, tempbuffer, blockoffset, blocks);

        if (action==DIO_WRITE)
            CopyMemQuick(buffer, tempbuffer, blocks<<globals->shifts_block);

      /* We're about to do a physical disk access.  (Re)set timeout.  Since
         the drive's motor will be turned off with the timeout as well we
         always (re)set the timeout even if doio() would return an error. */

        starttimeout();
        globals->retries=MAX_RETRIES;

        while ((errorcode=DoIO((struct IORequest *)globals->fsioreq.ioreq)) != 0)
        {
	    _DEBUG(("Buffered transfer: I/O error %d\n", errorcode));

            if ((errorcode = handleioerror(errorcode, action, globals->fsioreq.ioreq)) != 0)
            {
                _DEBUG(("Buffered transfer: SFS error %d\n", errorcode));
                return(errorcode);
            }
        }

        if (action==DIO_READ)
            CopyMemQuick(tempbuffer, buffer, blocks<<globals->shifts_block);

        blocklength-=blocks;
        blockoffset+=blocks;
        buffer+=blocks<<globals->shifts_block;
    }

    return(0);
}

LONG waittransfers(void)
{
    LONG firsterrorcode=0;
    LONG errorcode;

    /* This function waits for all the transfers in the iolist to
       be completed.  If any of them fails, the error is returned
       (and perhaps a requester is put up).  In any case, all of
       the requests will be processed, errors or not, so you won't
       have to call this function again in case of errors. */

    while(globals->iolist!=0) {
        struct fsIORequest *next=globals->iolist->next;

        if((errorcode=WaitIO((struct IORequest *)globals->iolist->ioreq))!=0 && firsterrorcode==0) {
            while((errorcode=handleioerror(errorcode, globals->iolist->action,
                globals->iolist->ioreq))==0) {
                if((errorcode=DoIO((struct IORequest *)globals->iolist->ioreq))==0) {
                    break;
                }
            }
    
            firsterrorcode=errorcode;
        }
    
        FreeMem(globals->iolist, sizeof(struct fsIORequest));
        globals->iolist=next;
    }

    return(firsterrorcode);
}

#if 0
static LONG asynctransfer(UWORD action, UBYTE *buffer, ULONG blockoffset, ULONG blocklength)
{
    /* Does asynchronous transfers, and returns before the transfer is completed.
       Use waittransfers() to wait for all transfers done with this function to
       complete.  If this function returns an error, then you still must call
       waittransfers().  Failure to do so will give unexpected results. */

    if(blockoffset < globals->blocks_total && blockoffset+blocklength <= globals->blocks_total) {
        ULONG maxblocks=globals->blocks_maxtransfer;

        if(((ULONG)buffer & ~globals->mask_mask)!=0) {   // Buffered transfer needed?
            return(transfer_buffered(action, buffer, blockoffset, blocklength));
        }

        while(blocklength!=0) {
            struct fsIORequest *fsi;

            if((fsi=createiorequest())!=0) {
                ULONG blocks=blocklength>maxblocks ? maxblocks : blocklength;
        
                setiorequest(fsi, action, buffer, blockoffset, blocks);
        
                /* We're about to do a physical disk access.  (Re)set timeout. */
        
                starttimeout();
        
                SendIO((struct IORequest *)fsi->ioreq);
                fsi->next=globals->iolist;
                globals->iolist=fsi;
        
                blocklength-=blocks;
                blockoffset+=blocks;
                buffer+=blocks<<globals->shifts_block;
            }
            else {
                /* Some requests could already be in progress, so you must call
                   waittransfers() even if an error is returned. */
                return(ERROR_NO_FREE_STORE);
            }
        }

        return(0);
    }
    else {
        req("This volume tried to access a block\noutside its partition.\n(blockoffset = %ld, blocklength = %ld)\n", "Ok", blockoffset, blocklength);

        return(ERROR_OUTSIDE_PARTITION);
    }
}
#endif

LONG transfer(UWORD action, UBYTE *buffer, ULONG blockoffset, ULONG blocklength)
{
    _TDEBUG(("TRANSFER: %ld, buf=0x%p, block=%ld, blocks=%ld...\n", action, buffer, blockoffset, blocklength));

    if ((blockoffset < globals->blocks_total) && (blockoffset + blocklength <= globals->blocks_total))
    {
        ULONG maxblocks = globals->blocks_maxtransfer;
        LONG errorcode;

	_DEBUG(("MaxTransfer: %d\n", maxblocks));

	// Buffered transfer needed?
        if (((IPTR)buffer & ~globals->mask_mask) !=0 )
        {
            _DEBUG(("Buffer 0x%p, mask 0x%p. Buffered transfer needed.\n", buffer, globals->mask_mask));

            return transfer_buffered(action, buffer, blockoffset, blocklength);
        }

        while (blocklength!=0)
        {
            ULONG blocks = (blocklength > maxblocks) ? maxblocks : blocklength;

            setiorequest(&globals->fsioreq, action, buffer, blockoffset, blocks);

            /* We're about to do a physical disk access.  (Re)set timeout.  Since
               the drive's motor will be turned off with the timeout as well we
               always (re)set the timeout even if doio() would return an error. */

            starttimeout();
            globals->retries=MAX_RETRIES;

            while ((errorcode=DoIO((struct IORequest *)globals->fsioreq.ioreq)) != 0)
            {
            	_DEBUG(("Direct transfer: I/O error %d\n", errorcode));

                if ((errorcode=handleioerror(errorcode, action, globals->fsioreq.ioreq)) != 0)
                {
	            _DEBUG(("Direct transfer: SFS error %d\n", errorcode));

                    return errorcode;
                }
            }

            blocklength-=blocks;
            blockoffset+=blocks;
            buffer+=blocks<<globals->shifts_block;
        }

	_DEBUG(("..."));
        _TDEBUG(("\n"));

        return(0);
    }
    else
    {
        req("This volume tried to access a block\noutside its partition.\n(blockoffset = %ld, blocklength = %ld)\n", "Ok", blockoffset, blocklength);
        return(ERROR_OUTSIDE_PARTITION);
    }
}
