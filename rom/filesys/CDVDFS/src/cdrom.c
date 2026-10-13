/* cdrom.c:
 *
 * Basic interaction with the CDROM drive.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 *              (C) Copyright 2002-2012 The AROS Development Team
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 *
 * 28-Dec-12 neil    Pass corrected TOC length back from Read_TOC instead of
 *                   the raw TOC header. Errors in the length field of this
 *                   header were causing buffer overruns with many drives.
 * 18-Dec-11 twilen  Added media change interrupt support.
 * 11-Aug-10 sonic   Fixed comparison warning in Has_Audio_Tracks()
 * 01-Mar-10 neil    Do not read past end of disc.
 * 12-Jun-09 neil    If drive returns incorrect TOC length, calculate it
 *                   based on the number of tracks.
 * 09-May-09 weissms   - Let Do_SCSI_Command result depend on DoIO result.
 *                     - Removed redundant checks of io_Error, some code reuse
 *                       (Clear_Sector_Buffers).
 * 20-Mar-09 sonic     - Removed usage of AROS-specific include
 * 08-Mar-09 error     - Corrected Test_Unit_Ready returning only NO_DISC state
 * 06-Mar-09 error     - Removed madness, fixed insanity. Cleanup started
 * 06-Jun-08 sonic     - Fixed to compile with gcc v2
 * 30-Mar-08 error     - Updated 'Find_Last_Session' with a generic command
 *                       mandatory for all MMC devices; corrected major flaw
 *                       with uninitialized variables
 * 12-Aug-07 sonic     - Added some debug output.
 * 09-Apr-07 sonic     - Disabled DirectSCSI on AROS.
 * 08-Apr-07 sonic     - Removed redundant TRACKDISK option.
 *                     - Added trackdisk64 support.
 *                     - Removed unneeded dealing with block length.
 * 07-Jul-02 sheutlin  various changes when porting to AROS
 *                     - global variables are now in a struct Globals *global
 * 02-Sep-94   fmu   Display READ TOC for Apple CD 150 drives.
 * 01-Sep-94   fmu   Workaround for bad NEC 3X READ TOC command in
 *                   Has_Audio_Tracks() and Data_Tracks().
 * 20-Aug-94   fmu   New function Find_Last_Session ().
 * 23-Jul-94   fmu   Last index modified from 99 to 1 in Start_Play_Audio().
 * 18-May-94   fmu   New drive model: MODEL_CDU_8002 (= Apple CD 150).
 * 17-May-94   fmu   Sense length 20 instead of 18 (needed by ALF controller).
 * 17-Feb-94   fmu   Added support for Toshiba 4101.
 * 06-Feb-94   dmb   - Fixed bug in Test_Unit_Ready() trackdisk support.
 *                   - Fixed bug in Open_CDROM (size of request)
 *                   - Added function Clear_Sector_Buffers().
 * 01-Jan-94   fmu   Added function Data_Tracks() for multisession support.
 * 11-Dec-93   fmu   - Memory type can now be chosen by the user.
 *                   - Addional parameter p_direction for Do_SCSI_Command().
 *                   - Start_Play_Audio() now plays all tracks.
 *                   - Mode_Select() instead of Select_XA_Mode().
 *                   - Support for CDROM drives with 512, 1024 or 2048 bytes
 *                     per block.
 * 06-Dec-93   fmu   New drive type DRIVE_SCSI_2.
 * 09-Nov-93   fmu   Added Select_XA_Mode.
 * 23-Oct-93   fmu   Open_CDROM now returns an error code that tell what
 *                   went wrong.
 * 09-Oct-93   fmu   SAS/C support added.
 * 03-Oct-93   fmu   New buffering algorithm.
 * 27-Sep-93   fmu   Added support for multi-LUN devices.
 * 24-Sep-93   fmu   - SCSI buffers may now reside in fast or chip memory.
 *                   - TD_CHANGESTATE instead of CMD_READ in Test_Unit_Ready
 */

#include <proto/alib.h>
#include <proto/exec.h>
#include <devices/trackdisk.h>
#include <exec/tasks.h>

#ifndef TD_READ64
#include <devices/newstyle.h>
#define TD_READ64 NSCMD_TD_READ64
#endif

#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "cdrom.h"
#include "debug.h"
#include "globals.h"

#include "clib_stuff.h"
#include <exec/interrupts.h>

AROS_INTH1(CDChangeHandler, struct CDVDBase *, global)
{
    AROS_INTFUNC_INIT

    Signal(&global->DosProc->pr_Task, global->g_changeint_sigbit);
    return 0;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(CDCacheLowMemHandler, CDROM *, cd)
{
    AROS_INTFUNC_INIT

    if (!cd->cache_io_busy &&
        cd->global->DosProc->pr_Task.tc_State == TS_WAIT)
    {
        if (cd->split_cache && cd->cache_reclaimed == 0)
        {
            APTR file2 = cd->buffer_file2;
            APTR metadata = cd->buffer_metadata;

            cd->buffer_file2 = NULL;
            cd->buffer_metadata = NULL;
            cd->file_slots = 1;
            Clear_Sector_Buffers(cd);
            FreeVec(file2);
            FreeVec(metadata);
            cd->cache_reclaimed = 1;
            return MEM_TRY_AGAIN;
        }
        else if (cd->split_cache && cd->cache_reclaimed == 1 &&
                 cd->buffer_data != NULL)
        {
            APTR cache = cd->buffer_data;

            cd->buffer_data = NULL;
            cd->file_slots = 0;
            cd->buffer_sectors = 1;
            Clear_Sector_Buffers(cd);
            FreeVec(cache);
            cd->cache_reclaimed = 2;
            return MEM_TRY_AGAIN;
        }
        else if (!cd->split_cache && cd->cache_reclaimed == 0 &&
                 cd->buffer_data != NULL)
        {
            APTR cache = cd->buffer_data;

            cd->buffer_data = NULL;
            cd->buffer_sectors = 1;
            Clear_Sector_Buffers(cd);
            FreeVec(cache);
            cd->cache_reclaimed = 1;
            return MEM_TRY_AGAIN;
        }
    }

    return MEM_DID_NOTHING;

    AROS_INTFUNC_EXIT
}

/*
 * i decided to change few things to make this code less insane.
 * biggest change is - i don't care any more if anyone reads one sector at a time
 * reading disc 1 sector at a time is totally insane.
 * The cache allocation remains STD_BUFFERS * 16 * 2048 bytes. Normally each
 * slot spans 16 sectors. The minimum one-buffer configuration reserves two
 * 2-sector slots for sequential file data and twelve one-sector slots for
 * directory and metadata traffic. Exec's low-memory handler retains one
 * 2-sector file slot, releases the other slot and metadata cache (28 KiB),
 * and uses a permanent one-sector buffer for uncached metadata reads.
 */

static UBYTE *Sector_Buffer(CDROM *cd, int index)
{
    UBYTE *base;

    if (cd->split_cache)
    {
        if (index == 0)
            base = cd->buffer_data;
        else if (index == 1)
            base = cd->buffer_file2;
        else
            base = cd->buffer_metadata;
    }
    else
        base = cd->buffer_data;

    base = (UBYTE *)(((IPTR)base + 15) & ~15);
    return base + cd->cache_slots[index].buffer_sector * SCSI_BUFSIZE;
}

static UBYTE *IO_Buffer(CDROM *cd)
{
    /* SCSI control commands share the fallback read buffer. */
    cd->io_sector = -1;
    return cd->buffer_io;
}

CDROM *Open_CDROM
        (
                struct CDVDBase *global,
                char *p_device,
                int p_scsi_id,
                uint32_t p_memory_type,
                int p_std_buffers,
                int p_file_buffers
        )
{
    CDROM *cd;
    int i;
    int err = CDROMERR_OK;

    /*
     * fake try-catch
     */
    do
    {
        err = CDROMERR_NO_MEMORY;
        cd = AllocVec(sizeof (CDROM),
            MEMF_PUBLIC | MEMF_CLEAR | p_memory_type);

        if (NULL == cd)
            break;

        cd->global = global;
        cd->io_sector = -1;
        /* Preserve the historical 32 KiB minimum cache allocation. */
        if (p_std_buffers == 1)
        {
            cd->buffer_sectors = 2;
            cd->file_slots = 2;
            cd->buffers_cnt = 14;
            cd->split_cache = TRUE;
        }
        else
        {
            cd->buffer_sectors = 16;
            cd->file_slots = p_std_buffers;
            cd->buffers_cnt = p_std_buffers;
        }

        if (cd->split_cache)
            cd->buffer_data = AllocVec(2 * SCSI_BUFSIZE + 15,
                MEMF_PUBLIC | p_memory_type);
        else
            cd->buffer_data = AllocVec(
                ((SCSI_BUFSIZE * p_std_buffers) << 4) + 15,
                MEMF_PUBLIC | p_memory_type);
        if (NULL == cd->buffer_data)
            break;

        if (cd->split_cache)
        {
            cd->buffer_file2 = AllocVec(2 * SCSI_BUFSIZE + 15,
                MEMF_PUBLIC | p_memory_type);
            if (NULL == cd->buffer_file2)
                break;

            cd->buffer_metadata = AllocVec(12 * SCSI_BUFSIZE + 15,
                MEMF_PUBLIC | p_memory_type);
            if (NULL == cd->buffer_metadata)
                break;
        }

        cd->buffer_io = AllocVec(SCSI_BUFSIZE,
            MEMF_PUBLIC | p_memory_type);
        if (NULL == cd->buffer_io)
            break;

        cd->cache_slots = AllocVec(sizeof (*cd->cache_slots) *
            cd->buffers_cnt, MEMF_PUBLIC | MEMF_CLEAR);
        if (NULL == cd->cache_slots)
            break;


        /*
         * make the buffer quad-word aligned. This greatly helps
         * performance on '040-powered systems with DMA SCSI
         * controllers.
         */
        for (i = 0; i < cd->buffers_cnt; i++)
        {
            cd->cache_slots[i].current_sector = -1;
            if (p_std_buffers == 1)
            {
                if (i < cd->file_slots)
                {
                    cd->cache_slots[i].buffer_sector = 0;
                    cd->cache_slots[i].sector_count = 2;
                }
                else
                {
                    cd->cache_slots[i].buffer_sector = i - cd->file_slots;
                    cd->cache_slots[i].sector_count = 1;
                }
            }
            else
            {
                cd->cache_slots[i].buffer_sector = i * 16;
                cd->cache_slots[i].sector_count = 16;
            }
        }

        err = CDROMERR_MSGPORT;
        cd->port = CreateMsgPort ();
        if (NULL == cd->port)
            break;

        err = CDROMERR_IOREQ;
        cd->scsireq = (struct IOStdReq *)CreateIORequest (cd->port, sizeof (struct IOExtTD));
        if (NULL == cd->scsireq)
            break;

        err = CDROMERR_DEVICE;
        if (OpenDevice ((UBYTE *) p_device, p_scsi_id, (struct IORequest *) cd->scsireq, 0))
            break;

        cd->cache_mem_handler.is_Node.ln_Name = "CDVDFS cache";
        cd->cache_mem_handler.is_Code = (VOID_FUNC)CDCacheLowMemHandler;
        cd->cache_mem_handler.is_Data = cd;
        AddMemHandler(&cd->cache_mem_handler);
        cd->cache_mem_handler_added = TRUE;

        cd->device_open = TRUE;

        if (global->g_scan_interval < 0) {
            /* Add media change interrupts */
            cd->iochangeint = (struct IOStdReq *)AllocVec(sizeof (struct IOExtTD), MEMF_PUBLIC);
            if (NULL == cd->iochangeint)
                break;
            CopyMem(cd->scsireq, cd->iochangeint, sizeof (struct IOExtTD));
            cd->changeint.is_Node.ln_Type = NT_INTERRUPT;
            cd->changeint.is_Node.ln_Name = "CDFS ChangeInt";
            cd->changeint.is_Data = (APTR)global;
            cd->changeint.is_Code = (VOID_FUNC)CDChangeHandler;
            cd->iochangeint->io_Length  = sizeof(struct Interrupt);
            cd->iochangeint->io_Data    = &cd->changeint;
            cd->iochangeint->io_Command = TD_ADDCHANGEINT;
            SendIO((struct IORequest *)cd->iochangeint);
        }

        cd->scsireq->io_Command = CMD_CLEAR;
        DoIO ((struct IORequest *) cd->scsireq);

        cd->t_changeint = -1;
        cd->t_changeint2 = -2;

        /* The LUN is the 2nd digit of the SCSI id number: */
        cd->lun = (p_scsi_id / 10) % 10;

        /* 'tick' is incremented every time a sector is accessed. */
        cd->tick = 0;
        err = CDROMERR_OK;
    } while (0);

    if (CDROMERR_OK != err)
    {
        Cleanup_CDROM(cd);
        cd = NULL;
    }

    return cd;
}

int Do_SCSI_Command
        (
                CDROM *p_cd,
                unsigned char *p_buf,
                long p_buf_length,
                unsigned char *p_command,
                int p_length,
                int p_direction
        )
{
    p_cd->scsireq->io_Length   = sizeof (struct SCSICmd);
    p_cd->scsireq->io_Data     = (APTR) &p_cd->cmd;
    p_cd->scsireq->io_Command  = HD_SCSICMD;

    p_cd->cmd.scsi_Data        = (UWORD *) p_buf;
    p_cd->cmd.scsi_Length      = p_buf_length;
    p_cd->cmd.scsi_Flags       = SCSIF_AUTOSENSE | p_direction;
    p_cd->cmd.scsi_SenseData   = (UBYTE *) p_cd->sense;
    p_cd->cmd.scsi_SenseLength = 20;
    p_cd->cmd.scsi_SenseActual = 0;
    p_cd->cmd.scsi_Command     = (UBYTE *) p_command;
    p_cd->cmd.scsi_CmdLength   = p_length;

    p_command[1] |= p_cd->lun << 5;

    if (0 != DoIO((struct IORequest *) p_cd->scsireq) ||
        0 != p_cd->cmd.scsi_Status)
    {
        Clear_Sector_Buffers(p_cd);
        return 0;
    }
    else
        return 1;
}

int Read_From_Drive
        (
                CDROM *p_cd,
                unsigned char *p_buf,
                long p_buf_length,

                long p_sector,
                int p_number_of_sectors
        )
{
    p_cd->scsireq->io_Length   = 2048 * p_number_of_sectors;
    p_cd->scsireq->io_Data     = (APTR) p_buf;
    p_cd->scsireq->io_Offset   = (ULONG) p_sector << 11;
    p_cd->scsireq->io_Actual   = (ULONG) p_sector >> 21;
    p_cd->scsireq->io_Command  = p_cd->scsireq->io_Actual ? TD_READ64 : CMD_READ;

    D(bug("[CDVDFS]\tAccessing sectors %ld:%ld\n", (long)p_sector, (long)p_number_of_sectors));

    if (0 != DoIO((struct IORequest*) p_cd->scsireq))
    {
        D(bug("[CDVDFS]\tTransfer failed: %ld\n", (long)p_cd->scsireq->io_Error));
        Clear_Sector_Buffers(p_cd);
        return 0;
    }

    D(bug("[CDVDFS]\tTransfer successful.\n"));
    return 1;
}

/*
 * USAGE NOTE >> VERY IMPORTANT <<
 * File reads use slots of p_cd->buffer_sectors sectors. Metadata reads only
 * require the requested sector to be valid.
 */
static int Find_Cache_Slot(CDROM *p_cd, int first, int last)
{
    int i;
    int loc;

    for (loc = first; loc < last; loc++)
        if (p_cd->cache_slots[loc].current_sector == -1)
            return loc;

    {
        uint32_t oldest_tick = UINT_MAX;

        for (loc = first, i = first; i < last; i++)
        {
            uint32_t tick = p_cd->cache_slots[i].last_used;

            if (tick < oldest_tick)
                loc = i, oldest_tick = tick;
        }
    }

    return loc;
}

static int Read_Chunk_Internal(CDROM *p_cd, long p_sector, int file_data)
{
    struct CDVDBase *global = p_cd->global;
    int status;
    int i;
    int loc;
    long vol_size;
    long start;
    int count;
    int first;
    int last;

    D(bug("[CDVDFS]\tClient requested sector %ld\n", p_sector));

    if (p_cd->buffer_data == NULL ||
        (!file_data && p_cd->split_cache && p_cd->buffer_metadata == NULL))
    {
        if (p_cd->io_sector == p_sector)
        {
            p_cd->buffer = p_cd->buffer_io;
            return 1;
        }

        p_cd->io_sector = -1;
        p_cd->cache_io_busy = TRUE;
        status = Read_From_Drive(p_cd, p_cd->buffer_io, SCSI_BUFSIZE,
            p_sector, 1);
        p_cd->cache_io_busy = FALSE;
        if (status)
        {
            p_cd->buffer = p_cd->buffer_io;
            p_cd->io_sector = p_sector;
        }
        return status;
    }

    for (i=0; i<p_cd->buffers_cnt; i++)
    {
        start = p_cd->cache_slots[i].current_sector;
        count = p_cd->cache_slots[i].sector_count;
        if (start == -1 || p_sector < start || p_sector >= start + count)
            continue;
        if (file_data && count < p_cd->buffer_sectors)
            continue;

        /*
         * get buffer offset
         */
        D(bug("[CDVDFS]\tSector already cached\n"));
        p_cd->buffer = Sector_Buffer(p_cd, i) +
            ((p_sector - start) << 11);

        /*
         * try most frequently used
         */
        p_cd->cache_slots[i].last_used += 2;
        for (i=0; i<p_cd->buffers_cnt; i++)
        {
            if (p_cd->cache_slots[i].last_used > 0)
                p_cd->cache_slots[i].last_used -= 1;
        }

        return 1;
    }

    /* Age entries on misses too. Without this, a full cache containing slots
     * with identical scores repeatedly replaces slot zero while stale entries
     * remain frozen forever.
     */
    for (i = 0; i < p_cd->buffers_cnt; i++)
    {
        if (p_cd->cache_slots[i].last_used > 0)
            p_cd->cache_slots[i].last_used--;
    }

    if (p_cd->file_slots < p_cd->buffers_cnt)
    {
        first = file_data ? 0 : p_cd->file_slots;
        last = file_data ? p_cd->file_slots : p_cd->buffers_cnt;
    }
    else
    {
        first = 0;
        last = p_cd->buffers_cnt;
    }

    loc = Find_Cache_Slot(p_cd, first, last);

    /*
     * Read one cache slot. Most CDs have enough end padding for the default
     * 16-sector slot, but not all, so clamp the final transfer below.
     */
    count = p_cd->cache_slots[loc].sector_count;
    start = p_sector & ~(count - 1);
    if (global->g_volume != NULL)
        vol_size = Volume_Size(global->g_volume);
    else
        vol_size = 0;
    if (vol_size != 0 && vol_size - start < count)
        count = vol_size - start;
    p_cd->cache_io_busy = TRUE;
    status = Read_From_Drive(p_cd, Sector_Buffer(p_cd, loc), SCSI_BUFSIZE,
        start, count);
    p_cd->cache_io_busy = FALSE;

    if (status)
    {
        p_cd->cache_slots[loc].current_sector = start;
        p_cd->buffer = Sector_Buffer(p_cd, loc) +
            ((p_sector - start) << 11);
        p_cd->cache_slots[loc].last_used = 1000;
    }

    return status;
}

int Read_Chunk(CDROM *p_cd, long p_sector)
{
    return Read_Chunk_Internal(p_cd, p_sector, FALSE);
}

int Read_File_Chunk(CDROM *p_cd, long p_sector)
{
    return Read_Chunk_Internal(p_cd, p_sector, TRUE);
}

int Test_Unit_Ready(CDROM *p_cd)
{
    p_cd->scsireq->io_Command = TD_CHANGENUM;

    if (0 != DoIO ((struct IORequest *) p_cd->scsireq))
        return FALSE;

    p_cd->t_changeint = p_cd->scsireq->io_Actual;

    p_cd->scsireq->io_Command = TD_CHANGESTATE;
    if ((0 != DoIO ((struct IORequest *) p_cd->scsireq)) ||
        (0 != p_cd->scsireq->io_Actual))
        return FALSE;

    return TRUE;
}

int Mode_Select
        (
                CDROM *p_cd,
                int p_mode,
                int p_block_length
        )
{
    UBYTE *buf;
    uint8_t cmd[6] = { };
    unsigned char mode[12] = { };

    cmd[0] = 0x15;
    cmd[1] = 0x10;
    cmd[4] = 0x0a;

    mode[3] = 8;
    mode[4] = p_mode;
    mode[9] = p_block_length >> 16;
    mode[10] = (p_block_length >> 8) & 0xff;
    mode[11] = p_block_length & 0xff;

    buf = IO_Buffer(p_cd);

    CopyMem(mode, buf, sizeof (mode));
    return Do_SCSI_Command(p_cd, buf, sizeof(mode), cmd, 6, SCSIF_WRITE);
}

int Inquire (CDROM *p_cd, t_inquiry_data *p_data)
{
    UBYTE *buf;
    uint8_t cmd[6] = { };
    cmd[0] = 0x12;
    cmd[4] = 96;

    buf = IO_Buffer(p_cd);

    if (!Do_SCSI_Command(p_cd, buf, 96, cmd, 6, SCSIF_READ))
        return FALSE;

    CopyMem(buf, p_data, sizeof (*p_data));
    return 1;
}


t_toc_data *Read_TOC
        (
                CDROM *p_cd,
                uint32_t *p_toc_len
        )
{
    uint8_t cmd[10] = { };
    uint32_t toc_len = 0;
    uint8_t *buf = IO_Buffer(p_cd);

    /*
     * check toc len
     */
    cmd[0] = 0x43;
    cmd[7] = 0;
    cmd[8] = 4;
    if (0 == Do_SCSI_Command(p_cd, buf, 4, cmd, 10, SCSIF_READ))
        return NULL;

    /*
     * toc len = len field + field contents
     * Some drives don't return the full TOC length when the buffer is too
     * small for the whole TOC: in this case, calculate it based on the
     * number of tracks
     */
    toc_len = 2 + ((buf[0] << 8) | (buf[1]));
    if (toc_len < 12)
        toc_len = 4 + (buf[3] - buf[2] + 2) * 8;

    /*
     * make sure it never happens (shouldn't)
     */
    if (toc_len > SCSI_IO_BUFSIZE)
        return NULL;

    /*
     * read complete TOC
     */
    cmd[7] = toc_len >> 8;
    cmd[8] = toc_len & 0xff;
    if (0 == Do_SCSI_Command(p_cd, buf, toc_len, cmd, 10, SCSIF_READ))
        return NULL;

    /*
     * We pass back the TOC length that the caller would expect to find in
     * the header, if it could be trusted!
     */
    *p_toc_len = toc_len - 2;

    return (t_toc_data *) (buf + 4);
}

int Has_Audio_Tracks(CDROM *p_cd)
{
    uint32_t toc_len;
    t_toc_data *toc;
    int i, len;

    toc = Read_TOC (p_cd, &toc_len);
    if (!toc)
        return FALSE;

    /*
     * calc num TOC entries
     * last entry is usually LEADOUT (0xAA)
     */
    len = toc_len >> 3;

    /*
     * traverse all tracks, check for audio?
     */
    for (i=0; i<len; i++)
    {
        if ((99 >= toc[i].track_number) &&
            (0  == (toc[i].flags & 4)))
            return toc[i].track_number;
    }
    return FALSE;
}

/*
 * Create a buffer containing the start addresses of all data tracks
 * on the disk.
 *
 * Returns:
 *  number of tracks or -1 on error.
 */

int Data_Tracks(CDROM *p_cd, uint32_t** p_buf)
{
    int cnt=0;
    uint32_t toc_len;
    t_toc_data *toc;
    int i, j, len;

    /*
     * collect TOC
     */
    toc = Read_TOC(p_cd, &toc_len);
    if (!toc)
        return -1;

    /*
     * calc TOC entries count
     */
    len = toc_len >> 3;

    /*
     * count number of data tracks:
     */
    for (i=0; i<len; i++)
    {
        if ((99 >= toc[i].track_number) &&
            (0  != (toc[i].flags & 4)))
            cnt++;
    }

    if (cnt == 0)
        return 0;

    /*
     * allocate memory for output buffer:
     */
    *p_buf = (uint32_t*) AllocVec (cnt * sizeof (uint32_t*), MEMF_PUBLIC);
    if (!*p_buf)
        return -1;

    /*
     * fill output buffer:
     */
    for (i=0, j=0; i<len; i++)
    {
        if ((99 >= toc[i].track_number) &&
            (0  != (toc[i].flags & 4)))
            (*p_buf)[j++] = toc[i].address;
    }

    return cnt;
}

inline void block2msf (uint32_t blk, unsigned char *msf)
{
    blk = (blk+150) & 0xffffff;
    msf[0] = blk / 4500;        /* 4500 = 60 seconds * 75 frames */
    blk %= 4500;
    msf[1] = blk / 75;
    msf[2] = blk % 75;
}

int Start_Play_Audio(CDROM *p_cd)
{
    uint8_t cmd[10] = { };
    uint32_t start = 0xffffffff,end;
    uint32_t toc_len;
    t_toc_data *toc;
    int i, len;

    if (!p_cd)
        return FALSE;

    cmd[0] = 0x47;

    /*
     * read TOC
     */
    toc = Read_TOC (p_cd, &toc_len);
    if (!toc)
        return FALSE;

    /*
     * calc len
     */
    len = toc_len >> 3;

    /*
     * find beginning of audio track
     */
    for (i=0; i<len; i++)
    {
        if ((99 >= toc[i].track_number) &&
            (0  == (toc[i].flags & 4)))
        {
            start=toc[i].address;
            break;
        }
    }

    /*
     * no audio - leave.
     */
    if (0xffffffff == start)
        return FALSE;

    /*
     * find end of audio track
     */
    for (; i<len; i++)
    {
        if (((99 < toc[i].track_number) && (toc[i].track_number > 99)) ||
            (0  != (toc[i].flags & 4)))
            break;
    }
    end=toc[i].address;

    /*
     * fill up request and send
     */
    block2msf(start, &cmd[3]);
    block2msf(end-1, &cmd[6]);

    return Do_SCSI_Command(p_cd, 0, 0, cmd, 10, SCSIF_READ);
}

int Stop_Play_Audio(CDROM *p_cd)
{
    uint8_t cmd[6] = { };

    if (!p_cd)
        return FALSE;

    cmd[0] = 0x1b;
    return Do_SCSI_Command(p_cd, 0, 0, cmd, 6, SCSIF_READ);
}

void Cleanup_CDROM (CDROM *p_cd)
{
    if (!p_cd)
        return;
    if (p_cd->cache_mem_handler_added)
        RemMemHandler(&p_cd->cache_mem_handler);
    if (p_cd->iochangeint) {
        p_cd->iochangeint->io_Length  = sizeof(struct Interrupt);
        p_cd->iochangeint->io_Data    = &p_cd->changeint;
        p_cd->iochangeint->io_Command = TD_REMCHANGEINT;
        DoIO((struct IORequest *)p_cd->iochangeint);
        FreeVec(p_cd->iochangeint);
    }
    if (p_cd->device_open)
        CloseDevice ((struct IORequest *) p_cd->scsireq);
    if (p_cd->scsireq)
        DeleteIORequest((struct IORequest *)p_cd->scsireq);
    if (p_cd->port)
        DeleteMsgPort (p_cd->port);
    if (p_cd->cache_slots)
        FreeVec (p_cd->cache_slots);
    if (p_cd->buffer_io)
        FreeVec (p_cd->buffer_io);
    if (p_cd->buffer_metadata)
        FreeVec (p_cd->buffer_metadata);
    if (p_cd->buffer_file2)
        FreeVec (p_cd->buffer_file2);
    if (p_cd->buffer_data)
        FreeVec (p_cd->buffer_data);
    FreeVec (p_cd);
}

void Clear_Sector_Buffers (CDROM *p_cd)
{
    int i;

    p_cd->io_sector = -1;
    for (i=0; i<p_cd->buffers_cnt; i++)
        p_cd->cache_slots[i].current_sector = -1;
}

/* Finds offset of last session. (Not supported by all CDROM drives)
 *
 * Returns: - FALSE if there is no special SCSI command to determine the
 *            offset of the last session.
 *          - TRUE if the offset of the last session has been determined.
 */

int Find_Last_Session(CDROM *p_cd, uint32_t *p_result)
{
    uint8_t cmd[10] = { };
    uint8_t *data = IO_Buffer(p_cd);

    cmd[0] = 0x43;
    cmd[2] = 0x01;

    /*
     * first: READTOC IS MANDATORY
     * drives not conforming to MMC2 died in 1995.
     */
    *p_result = 0;

    /*
     * Ask the scsi device for the length of this TOC
     */
    cmd[7] = 0;
    cmd[8] = 12;
    if (!Do_SCSI_Command(p_cd, data, 12, cmd, sizeof(cmd), SCSIF_READ))
        return FALSE;

    /*
     * check if we are dealing with a DATA track here.
     * nobody would like to spend an hour trying to get his mixed mode cd read
     */
    D(bug("[CDVDFS]\tFirst track in last session has type %lx\n", data[5]));
    if ((data[5] & 0xfc) != 0x14)
    {
       D(bug("[CDVDFS]\tThis track is not a DATA track. Will default to track 1.\n"));

       /*
        * this indeed sets the address of first track in first session, but we have no idea if this really is a data track.
        * we don't care anyways. it will be detected by higher layer
        */
       return TRUE;
    }

    /*
     * the ReadTOC has MSF field set to 0 so we treat obtained values as logical block address
     */

    *p_result = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | (data[11]);
    return TRUE;
}
