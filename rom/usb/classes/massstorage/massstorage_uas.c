/*
 *----------------------------------------------------------------------------
 *                         massstorage class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "massstorage.class.h"

extern const STRPTR GM_UNIQUENAME(libname);

#undef  ps
#define ps ncm->ncm_Base

static void nUasFillLun(UBYTE *lun, UWORD lunnum)
{
    memset(lun, 0, 8);
    lun[0] = (UBYTE) (lunnum & 0x3f);
}

static inline BOOL nUasErrForgiven(LONG ioerr)
{
    return (ioerr == 0) || nIsOverflowErr(ioerr) || (ioerr == UHIOERR_RUNTPACKET);
}

/* The IU id of a status transfer. An empty read carries no IU at all, which
   is not id 0 on the wire - but no defined id is 0, so 0 is the "there was
   no IU" sentinel. */
static inline UBYTE nUasIUId(const UBYTE *buf, ULONG actual_len)
{
    return actual_len ? buf[0] : 0;
}

/* Pick the Sense IU apart: SCSI status at byte 6, inline sense data behind
   the 16-byte header. Callers preinitialize *status / *iu_id / *sense_actual
   to their defaults; short IUs leave them untouched.
   Only a Sense IU carries a SCSI status - byte 6 of any other IU means
   something else entirely (in a Response IU it is additional response info),
   so callers must check *iu_id before trusting *status. */
static void nUasParseStatusIU(const UBYTE *statusbuf, ULONG actual_len,
                              UBYTE *status, UBYTE *iu_id,
                              UBYTE *sense_data, ULONG sense_len, UWORD *sense_actual)
{
    UBYTE id = nUasIUId(statusbuf, actual_len);

    if(iu_id)
    {
        *iu_id = id;
    }
    if(status && (id == UAS_IU_ID_SENSE) &&
       (actual_len > offsetof(struct UasSenseIU, iu_Status)))
    {
        *status = ((const struct UasSenseIU *) statusbuf)->iu_Status;
    }
    if((id == UAS_IU_ID_SENSE) && sense_data && sense_actual)
    {
        ULONG header = offsetof(struct UasSenseIU, iu_Sense);

        if(actual_len > header)
        {
            const struct UasSenseIU *senseiu = (const struct UasSenseIU *) statusbuf;
            ULONG sense_avail = actual_len - header;
            ULONG copy_len = AROS_BE2WORD(senseiu->iu_Length);

            if(copy_len > sense_avail)
            {
                copy_len = sense_avail;
            }
            if(copy_len > sense_len)
            {
                copy_len = sense_len;
            }
            if(copy_len)
            {
                CopyMem((APTR) &statusbuf[header], sense_data, copy_len);
                *sense_actual = (UWORD) copy_len;
            }
        }
    }
}

/* The tag of a status-pipe IU: every IU carries it big-endian at bytes 2..3. */
static inline UWORD nUasIUTag(const UBYTE *buf, ULONG actual_len)
{
    return (actual_len >= 4) ? (UWORD) ((buf[2] << 8) | buf[3]) : 0;
}

/* One synchronous UAS command on the plain (stream-less) pipes, i.e. what a
   High Speed UAS device or any HCD without stream support can do: QD 1.

   Without streams (UAS for USB 2.0 operation) the device announces the data
   phase itself: once it is ready to move the data for this tag it sends a
   Read Ready / Write Ready IU on the status pipe, then the data moves, then a
   Sense IU closes the command. A device that rejects the command sends the
   Sense IU straight away and there is no data phase at all - so the status
   pipe is read first and decides whether a data transfer is started, instead
   of a data transfer that would sit there until the NAK timeout. */
static LONG nUasDoCommand(struct NepClassMS *ncm, const UBYTE *cdb, UWORD cdb_len,
                           UBYTE *data, ULONG data_len, BOOL read,
                           ULONG *actual, UBYTE *status,
                           UBYTE *sense_data, ULONG sense_len, UWORD *sense_actual)
{
    struct UasCommandIU cmdiu;
    UBYTE statusbuf[64];
    struct PsdPipe *statuspipe = ncm->ncm_EPStatusPipe;
    struct PsdPipe *datapipe = read ? ncm->ncm_EPInPipe : ncm->ncm_EPOutPipe;
    LONG ioerr;
    ULONG len;
    UBYTE id;
    UWORD tag;

    memset(&cmdiu, 0, sizeof(cmdiu));
    cmdiu.iu_Id = UAS_IU_ID_COMMAND;
    cmdiu.iu_TaskAttr = 0;
    /* a 16-bit tag; 0 is not a valid one */
    if(!(++ncm->ncm_TagCount & 0xffff))
    {
        ++ncm->ncm_TagCount;
    }
    tag = (UWORD) ncm->ncm_TagCount;
    cmdiu.iu_Tag = AROS_WORD2BE(tag);
    nUasFillLun(cmdiu.iu_Lun, ncm->ncm_UnitLUN);
    CopyMem((APTR) cdb, cmdiu.iu_Cdb, (cdb_len > 16) ? 16 : cdb_len);

    /* Status armed before the command IU: the read is already outstanding
       when the device answers. */
    psdSendPipe(statuspipe, statusbuf, sizeof(statusbuf));
    ioerr = psdDoPipe(ncm->ncm_EPCmdPipe, &cmdiu, sizeof(cmdiu));
    if(ioerr)
    {
        psdAbortPipe(statuspipe);
        psdWaitPipe(statuspipe);
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "UAS command IU transfer failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return ioerr;
    }

    /* the first status IU: a READY IU opening the data phase, or the Sense IU
       of a command that has no data phase (or was rejected before it) */
    ioerr = psdWaitPipe(statuspipe);
    if(!nUasErrForgiven(ioerr))
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "UAS status IU transfer failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return ioerr;
    }
    len = psdGetPipeActual(statuspipe);
    id = nUasIUId(statusbuf, len);
    if((id == UAS_IU_ID_READ_READY) || (id == UAS_IU_ID_WRITE_READY))
    {
        LONG dataerr;

        if((!data_len) || ((id == UAS_IU_ID_READ_READY) != read) ||
           (nUasIUTag(statusbuf, len) != tag))
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "UAS unexpected %s Ready IU (tag 0x%04lx, expected 0x%04lx, %ld data bytes)",
                           (id == UAS_IU_ID_READ_READY) ? "Read" : "Write",
                           (ULONG) nUasIUTag(statusbuf, len), (ULONG) tag, data_len);
            return HFERR_Phase;
        }
        /* re-arm the status read for the Sense IU, then move the data */
        psdSendPipe(statuspipe, statusbuf, sizeof(statusbuf));
        dataerr = psdDoPipe(datapipe, data, data_len);
        if(actual)
        {
            *actual = psdGetPipeActual(datapipe);
        }
        if(!nUasErrForgiven(dataerr))
        {
            /* no data, no Sense IU: don't wait for one */
            psdAbortPipe(statuspipe);
            psdWaitPipe(statuspipe);
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "UAS data transfer failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, dataerr, "unknown"), dataerr);
            return dataerr;
        }
        ioerr = psdWaitPipe(statuspipe);
        if(!nUasErrForgiven(ioerr))
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "UAS status IU transfer failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            return ioerr;
        }
        len = psdGetPipeActual(statuspipe);
        id = nUasIUId(statusbuf, len);
    }
    if((id != UAS_IU_ID_SENSE) || (nUasIUTag(statusbuf, len) != tag))
    {
        /* Only a Sense IU for this tag closes the command. Anything else
           here (a Response IU, a second READY IU, an empty read, a stale
           IU from an earlier command) is a protocol violation, not a status. */
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "UAS unexpected status IU 0x%02lx (tag 0x%04lx, expected 0x%04lx, %ld bytes)",
                       (ULONG) id, (ULONG) nUasIUTag(statusbuf, len), (ULONG) tag, len);
        return HFERR_Phase;
    }
    nUasParseStatusIU(statusbuf, len, status, NULL, sense_data, sense_len, sense_actual);
    return 0;
}

LONG nScsiDirectUAS(struct NepClassMS *ncm, struct SCSICmd *scsicmd)
{
    LONG ioerr;
    LONG rioerr = 0;
    ULONG datalen = scsicmd->scsi_Length;
    UBYTE status = SCSI_GOOD;
    /* Autoretry: one repeat, spent only on a UNIT ATTENTION - the one sense
       that reports discarded device state (every device answers it on its
       first command after a bus reset, hence after every reboot) instead of
       answering the command. Deliberately narrower than Bulk-Only, which
       retries every key except NOT READY and ILLEGAL REQUEST: this is a
       passthrough path, and MEDIUM ERROR or DATA PROTECT are answers that
       belong to the caller. */
    LONG retrycnt = (scsicmd->scsi_Flags & 0x80) ? 1 : 0;

    scsicmd->scsi_CmdActual = (scsicmd->scsi_CmdLength > 16) ? 16 : scsicmd->scsi_CmdLength;

    nLockXFer(ncm);
    do
    {
        rioerr = 0;
        status = SCSI_GOOD;
        scsicmd->scsi_Status = SCSI_GOOD;
        scsicmd->scsi_Actual = 0;
        scsicmd->scsi_SenseActual = 0;

        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }

        ioerr = nUasDoCommand(ncm, scsicmd->scsi_Command, scsicmd->scsi_CmdLength,
                              (UBYTE *) scsicmd->scsi_Data, datalen,
                              (scsicmd->scsi_Flags & SCSIF_READ) != 0,
                              &scsicmd->scsi_Actual, &status,
                              (scsicmd->scsi_Flags & SCSIF_AUTOSENSE) ? scsicmd->scsi_SenseData : NULL,
                              scsicmd->scsi_SenseLength, &scsicmd->scsi_SenseActual);
        if(ioerr)
        {
            /* nUasDoCommand already reported the failing phase */
            scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
            rioerr = HFERR_Phase;
            break;
        }

        scsicmd->scsi_Status = status;
        if(status)
        {
            rioerr = HFERR_BadStatus;
            if((scsicmd->scsi_Flags & SCSIF_AUTOSENSE) && (!scsicmd->scsi_SenseActual))
            {
                UBYTE sensecmd[6];
                UBYTE sense_status = SCSI_GOOD;
                ULONG sense_actual = 0;

                sensecmd[0] = SCSI_REQUEST_SENSE;
                sensecmd[1] = 0;
                sensecmd[2] = 0;
                sensecmd[3] = 0;
                sensecmd[4] = (UBYTE) scsicmd->scsi_SenseLength;
                sensecmd[5] = 0;

                ioerr = nUasDoCommand(ncm, sensecmd, 6,
                                      scsicmd->scsi_SenseData, scsicmd->scsi_SenseLength,
                                      TRUE, &sense_actual, &sense_status,
                                      NULL, 0, NULL);
                if(!ioerr)
                {
                    scsicmd->scsi_SenseActual = (UWORD) sense_actual;
                } else {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "UAS request sense failed: %s (%ld)",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }
            }

            /* What the retry swallows this layer must record: a media-change
               UNIT ATTENTION consumed here never reaches the TUR poll's sense
               parser, so bump the change counter ourselves, as Bulk-Only does. */
            if((scsicmd->scsi_SenseActual >= 14) &&
               ((scsicmd->scsi_SenseData[2] & SK_MASK) == SK_UNIT_ATTENTION) &&
               (ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT) &&
               ((scsicmd->scsi_SenseData[12] == 0x28) || (scsicmd->scsi_SenseData[12] == 0x3a)))
            {
                ncm->ncm_ChangeCount++;
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                {
                    psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                   "Diskchange: Unit Attention (count = %ld)",
                                   ncm->ncm_ChangeCount);
                }
            }

            /* NOT READY and ILLEGAL REQUEST are answers rather than accidents,
               and sense we could not read tells us nothing - only UNIT
               ATTENTION earns the retry. */
            if(scsicmd->scsi_SenseActual < 14)
            {
                retrycnt = 0;
            }
            else if((scsicmd->scsi_SenseData[2] & SK_MASK) != SK_UNIT_ATTENTION)
            {
                retrycnt = 0;
            }
        } else {
            retrycnt = 0; /* command succeeded */
        }

    } while(retrycnt--);
    nUnlockXFer(ncm);
    return(rioerr);
}
/* \\ */
