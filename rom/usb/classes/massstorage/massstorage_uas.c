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

static LONG nUasDoCommand(struct NepClassMS *ncm, const UBYTE *cdb, UWORD cdb_len,
                           UBYTE *data, ULONG data_len, BOOL read,
                           ULONG *actual, UBYTE *status, UBYTE *iu_id,
                           UBYTE *sense_data, ULONG sense_len, UWORD *sense_actual)
{
    struct UasCommandIU cmdiu;
    UBYTE statusbuf[64];
    struct PsdPipe *pp;
    ULONG cmdlen;
    ULONG actual_len;
    LONG ioerr;

    if(actual)
    {
        *actual = 0;
    }
    if(status)
    {
        *status = SCSI_GOOD;
    }
    if(iu_id)
    {
        *iu_id = 0;
    }
    if(sense_actual)
    {
        *sense_actual = 0;
    }

    memset(&cmdiu, 0, sizeof(cmdiu));
    cmdiu.iu_Id = UAS_IU_ID_COMMAND;
    cmdiu.iu_TaskAttr = 0;
    cmdiu.iu_Tag = AROS_LONG2LE(++ncm->ncm_TagCount);
    nUasFillLun(cmdiu.iu_Lun, ncm->ncm_UnitLUN);
    cmdlen = (cdb_len > 16) ? 16 : cdb_len;
    if(cmdlen)
    {
        CopyMem(cdb, cmdiu.iu_Cdb, cmdlen);
    }

    ioerr = psdDoPipe(ncm->ncm_EPCmdPipe, &cmdiu, sizeof(cmdiu));
    if(ioerr)
    {
        return ioerr;
    }

    if(data_len)
    {
        pp = read ? ncm->ncm_EPInPipe : ncm->ncm_EPOutPipe;
        ioerr = psdDoPipe(pp, data, data_len);
        if(actual)
        {
            *actual = psdGetPipeActual(pp);
        }
        if((ioerr == UHIOERR_OVERFLOW) || (ioerr == UHIOERR_RUNTPACKET))
        {
            ioerr = 0;
        }
        if(ioerr)
        {
            return ioerr;
        }
    }

    ioerr = psdDoPipe(ncm->ncm_EPStatusPipe, statusbuf, sizeof(statusbuf));
    if(ioerr && (ioerr != UHIOERR_RUNTPACKET) && (ioerr != UHIOERR_OVERFLOW))
    {
        return ioerr;
    }
    actual_len = psdGetPipeActual(ncm->ncm_EPStatusPipe);
    if(iu_id)
    {
        *iu_id = statusbuf[0];
    }
    if(status)
    {
        if(actual_len >= sizeof(struct UasStatusIU))
        {
            *status = ((struct UasStatusIU *) statusbuf)->iu_Status;
        } else if(actual_len >= 3) {
            *status = statusbuf[2];
        }
    }
    if(iu_id && (*iu_id == UAS_IU_ID_SENSE) && sense_data && sense_actual)
    {
        ULONG header = offsetof(struct UasSenseIU, iu_Sense);
        ULONG copy_len = 0;

        if(actual_len > header)
        {
            struct UasSenseIU *senseiu = (struct UasSenseIU *) statusbuf;
            ULONG sense_avail = actual_len - header;
            ULONG sense_reported = AROS_LE2WORD(senseiu->iu_SenseLength);

            copy_len = sense_reported;
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
                CopyMem(&statusbuf[header], sense_data, copy_len);
                *sense_actual = (UWORD) copy_len;
            }
        }
    }
    return 0;
}

/* /// "nScsiDirectUAS()" */
LONG nScsiDirectUAS(struct NepClassMS *ncm, struct SCSICmd *scsicmd)
{
    LONG ioerr;
    LONG rioerr = 0;
    ULONG datalen;
    UBYTE status = SCSI_GOOD;
    UBYTE iu_id = 0;

    datalen = scsicmd->scsi_Length;
    scsicmd->scsi_Status = SCSI_GOOD;
    scsicmd->scsi_Actual = 0;
    scsicmd->scsi_CmdActual = (scsicmd->scsi_CmdLength > 16) ? 16 : scsicmd->scsi_CmdLength;
    scsicmd->scsi_SenseActual = 0;

    nLockXFer(ncm);
    do
    {
        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }

        ioerr = nUasDoCommand(ncm, scsicmd->scsi_Command, scsicmd->scsi_CmdLength,
                              (UBYTE *) scsicmd->scsi_Data, datalen,
                              (scsicmd->scsi_Flags & SCSIF_READ) != 0,
                              &scsicmd->scsi_Actual, &status, &iu_id,
                              (scsicmd->scsi_Flags & SCSIF_AUTOSENSE) ? scsicmd->scsi_SenseData : NULL,
                              scsicmd->scsi_SenseLength, &scsicmd->scsi_SenseActual);
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "UAS command transfer failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
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
                                      TRUE, &sense_actual, &sense_status, NULL,
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
        }

    } while(FALSE);
    nUnlockXFer(ncm);
    return(rioerr);
}
/* \\ */
