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

/* /// "nBulkReset()" */
LONG nBulkReset(struct NepClassMS *ncm)
{
    LONG ioerr;
    LONG ioerr2 = 0;
    static UBYTE cbiresetcmd12[12] = { 0x1D, 0x04, 0xFF, 0xFF, 0xFF, 0xFF,
                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    //struct UsbMSCBIStatusWrapper umscsw;
    //UBYTE sensedata[18];
    if(ncm->ncm_DenyRequests)
    {
        return UHIOERR_TIMEOUT;
    }
    KPRINTF(1, ("Bulk Reset\n"));
    //psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Bulk Reset...");
    switch(ncm->ncm_TPType)
    {
        case MS_PROTO_BULK:
            if(!ncm->ncm_BulkResetBorks)
            {
                 psdPipeSetup(ncm->ncm_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                              UMSR_BULK_ONLY_RESET, 0, (ULONG) ncm->ncm_UnitIfNum);
                 ioerr2 = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                 if(ioerr2 == UHIOERR_TIMEOUT)
                 {
                     return(ioerr2);
                 }
                 if(ioerr2)
                 {
                     psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                    "BULK_ONLY_RESET failed: %s (%ld)",
                                    psdNumToStr(NTS_IOERR, ioerr2, "unknown"), ioerr2);
                     ncm->ncm_BulkResetBorks = TRUE;
                 }
                 if(ncm->ncm_DenyRequests)
                 {
                     return ioerr2;
                 }
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPInNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            if(ncm->ncm_DenyRequests)
            {
                return ioerr;
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPOutNum);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPOutNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            return(ioerr2 ? ioerr2 : ioerr);

        case MS_PROTO_UAS:
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPInNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            if(ncm->ncm_DenyRequests)
            {
                return ioerr;
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPOutNum);
            ioerr2 = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr2)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPOutNum, psdNumToStr(NTS_IOERR, ioerr2, "unknown"), ioerr2);
            }
            if(ncm->ncm_EPCmdNum)
            {
                psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                             USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPCmdNum);
                ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                if(ioerr)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                                   ncm->ncm_EPCmdNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }
            }
            if(ncm->ncm_EPStatusNum)
            {
                psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                             USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPStatusNum|URTF_IN);
                ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                if(ioerr)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                                   ncm->ncm_EPStatusNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                }
            }
            return(ioerr2 ? ioerr2 : ioerr);

        case MS_PROTO_CBI:
        case MS_PROTO_CB:
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                         UMSR_ADSC, 0, (ULONG) ncm->ncm_UnitIfNum);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, cbiresetcmd12, 12);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CBI_RESET failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            if(ncm->ncm_DenyRequests)
            {
                return ioerr;
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPInNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            if(ncm->ncm_DenyRequests)
            {
                return ioerr;
            }
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPOutNum);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                               "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                               ncm->ncm_EPOutNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
            //nCBIRequestSense(ncm, sensedata, 18);
            return(ioerr);
    }
    return(0);
}
/* \\\ */

/* /// "nBulkClear()" */
LONG nBulkClear(struct NepClassMS *ncm)
{
    LONG ioerr;
    if(ncm->ncm_DenyRequests)
    {
        return UHIOERR_TIMEOUT;
    }
    KPRINTF(1, ("Bulk Clear\n"));
    //psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Bulk Clear...");
    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
    if(ioerr == UHIOERR_TIMEOUT)
    {
        return(ioerr);
    }
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                       ncm->ncm_EPInNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    if(ncm->ncm_DenyRequests)
    {
        return ioerr;
    }
    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPOutNum);
    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                       "CLEAR_ENDPOINT_HALT %ld failed: %s (%ld)",
                       ncm->ncm_EPOutNum, psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    if(ncm->ncm_TPType == MS_PROTO_UAS)
    {
        if(ncm->ncm_EPCmdNum)
        {
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPCmdNum);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
        }
        if(ncm->ncm_EPStatusNum)
        {
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPStatusNum|URTF_IN);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
        }
    }
    return(ioerr);
}
/* \\\ */

/* /// "nScsiDirectBulk()" */
LONG nScsiDirectBulk(struct NepClassMS *ncm, struct SCSICmd *scsicmd)
{
    LONG ioerr;
    struct PsdPipe *pp;
    struct UsbMSCmdBlkWrapper umscbw;
    struct UsbMSCmdStatusWrapper umscsw;
    ULONG datalen;
    LONG rioerr;
    UWORD retrycnt = 0;
    UBYTE cmdstrbuf[16*3+2];

    KPRINTF(10, ("\n"));

    GM_UNIQUENAME(nHexString)(scsicmd->scsi_Command, (ULONG) (scsicmd->scsi_CmdLength < 16 ? scsicmd->scsi_CmdLength : 16), cmdstrbuf);

    if(scsicmd->scsi_Flags & 0x80) /* Autoretry */
    {
        retrycnt = 1;
    }
    umscbw.dCBWSignature = AROS_LONG2LE(0x43425355);
    scsicmd->scsi_Status = SCSI_GOOD;
    nLockXFer(ncm);
    do
    {
        KPRINTF(10, ("retrycnt %ld\n",retrycnt));
        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }
        /*nBulkReset(ncm);*/

        rioerr = 0;

        /*psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                     USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
        ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);*/

        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA)
        {
            psdDelayMS(1);
        }

        datalen = scsicmd->scsi_Length;
        umscbw.dCBWTag = (IPTR) scsicmd + ++ncm->ncm_TagCount;
        umscbw.dCBWDataTransferLength = AROS_LONG2LE(datalen);
        umscbw.bmCBWFlags = scsicmd->scsi_Flags & SCSIF_READ ? 0x80 : 0x00;
        umscbw.bCBWLUN = ncm->ncm_UnitLUN;
        if((scsicmd->scsi_CmdLength) >= 16)
        {
            CopyMemQuick(scsicmd->scsi_Command, umscbw.CBWCB, 16);
            umscbw.bCBWCBLength = scsicmd->scsi_CmdActual = 16;
        } else {
            memset(umscbw.CBWCB, 0, 16);
            CopyMem(scsicmd->scsi_Command, umscbw.CBWCB, (ULONG) scsicmd->scsi_CmdLength);
            umscbw.bCBWCBLength = scsicmd->scsi_CmdActual = scsicmd->scsi_CmdLength;
        }
        //psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname), "Issueing command %s Dlen=%ld", cmdstrbuf, datalen);

        KPRINTF(2, ("command block phase, tag %08lx, len %ld, flags %02lx...\n",
                umscbw.dCBWTag, scsicmd->scsi_CmdLength, scsicmd->scsi_Flags));
        KPRINTF(2, ("command: %s\n", cmdstrbuf));
        ioerr = psdDoPipe(ncm->ncm_EPOutPipe, &umscbw, UMSCBW_SIZEOF);
        if(ioerr == UHIOERR_STALL) /* Retry on stall */
        {
            KPRINTF(2, ("stall...\n"));
            nBulkClear(ncm);
            ioerr = psdDoPipe(ncm->ncm_EPOutPipe, &umscbw, UMSCBW_SIZEOF);
        }
        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }
        if(!ioerr)
        {
            if(datalen)
            {
                KPRINTF(2, ("data phase %ld bytes...\n", datalen));
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA)
                {
                    psdDelayMS(1);
                }
                pp = (scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInPipe : ncm->ncm_EPOutPipe;
                ioerr = psdDoPipe(pp, scsicmd->scsi_Data, datalen);
                scsicmd->scsi_Actual = psdGetPipeActual(pp);
                if(ioerr == UHIOERR_OVERFLOW)
                {
                    KPRINTF(10, ("Extra Data received, but ignored!\n"));
                    ioerr = 0;
                }
                else if(ioerr == UHIOERR_STALL) /* Accept on stall */
                {
                    KPRINTF(2, ("stall...\n"));
                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                                 (ULONG) ((scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInNum|URTF_IN : ncm->ncm_EPOutNum));
                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                }
                else if(ioerr == UHIOERR_RUNTPACKET)
                {
                    KPRINTF(10, ("Runt packet ignored...\n"));
                    ioerr = 0;
                    /*psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                                 (ULONG) ncm->ncm_EPInNum|URTF_IN);
                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);*/
                }
            } else {
                ioerr = 0;
                scsicmd->scsi_Actual = 0;
            }
            if(!ioerr)
            {
                KPRINTF(2, ("command status phase...\n"));
                ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                if(ioerr == UHIOERR_STALL) /* Retry on stall */
                {
                    KPRINTF(2, ("stall...\n"));
                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                    /*nBulkClear(ncm);*/
                    ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                }
                if(ioerr == UHIOERR_RUNTPACKET)
                {
                    // well, retry then
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Command status block truncated (%ld bytes), retrying...",
                                   psdGetPipeActual(ncm->ncm_EPInPipe));
                    ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                }
                if(ioerr == UHIOERR_OVERFLOW)
                {
                    KPRINTF(10, ("Extra Status received, but ignored!\n"));
                    ioerr = 0;
                }
                if(ncm->ncm_DenyRequests)
                {
                    rioerr = HFERR_Phase;
                    break;
                }
                if(!ioerr)
                {
                    KPRINTF(2, ("Status:\n"
                                "  Signature: %08lx\n"
                                "  Tag      : %08lx\n"
                                "  Residue  : %08lx\n"
                                "  Status   : %02lx\n",
                                umscsw.dCSWSignature,
                                umscbw.dCBWTag,
                                umscsw.dCSWDataResidue,
                                umscsw.bCSWStatus));
                    if(((umscsw.dCSWSignature != AROS_LONG2LE(0x53425355)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_CSS_BROKEN))) || (umscsw.dCSWTag != umscbw.dCBWTag))
                    {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Illegal command status block (Sig:%08lx, Tag=%08lx/%08lx (TX/RX), Len=%ld)",
                                       umscsw.dCSWSignature,
                                       umscbw.dCBWTag,
                                       umscsw.dCSWTag,
                                       psdGetPipeActual(ncm->ncm_EPInPipe));
                        scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                        rioerr = HFERR_Phase;
                        nBulkReset(ncm);
                        continue;
                    }
                    /* ignore this: too many firmwares report shit */
                    //scsicmd->scsi_Actual = datalen - AROS_LONG2LE(umscsw.dCSWDataResidue);
                    if((scsicmd->scsi_Actual > 7) && ((AROS_LONG2BE(*((ULONG *) scsicmd->scsi_Data))>>8) == 0x555342) && (((ULONG *) scsicmd->scsi_Data)[1] == umscbw.dCBWTag))
                    {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Your MSD has a very bad firmware! Havoc!");
                        scsicmd->scsi_Actual = 0;
                        umscsw.bCSWStatus = USMF_CSW_FAIL;
                    }
                    scsicmd->scsi_Status = umscsw.bCSWStatus;
                    if(umscsw.bCSWStatus)
                    {
                        if(umscsw.bCSWStatus == USMF_CSW_PHASEERR)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed: %ld", cmdstrbuf, umscsw.bCSWStatus);
                            nBulkReset(ncm);
                        }
                        /* Autosensing required? */
                        if(scsicmd->scsi_Flags & SCSIF_AUTOSENSE)
                        {
                            /*nBulkClear(ncm);*/

                            datalen = scsicmd->scsi_SenseLength;
                            umscbw.dCBWTag = (IPTR) scsicmd + ++ncm->ncm_TagCount;
                            umscbw.dCBWDataTransferLength = AROS_LONG2LE(datalen);
                            umscbw.bmCBWFlags = 0x80;
                            /*umscbw.bCBWLUN = ncm->ncm_UnitLun;*/
                            umscbw.bCBWCBLength = 6;
                            umscbw.CBWCB[0] = SCSI_REQUEST_SENSE;
                            umscbw.CBWCB[1] = 0x00;
                            umscbw.CBWCB[2] = 0x00;
                            umscbw.CBWCB[3] = 0x00;
                            umscbw.CBWCB[4] = datalen;
                            umscbw.CBWCB[5] = 0;
                            KPRINTF(2, ("sense command block phase...\n"));
                            ioerr = psdDoPipe(ncm->ncm_EPOutPipe, &umscbw, UMSCBW_SIZEOF);
                            if(ioerr == UHIOERR_STALL) /* Retry on stall */
                            {
                                KPRINTF(2, ("stall...\n"));
                                nBulkClear(ncm);
                                ioerr = psdDoPipe(ncm->ncm_EPOutPipe, &umscbw, UMSCBW_SIZEOF);
                            }
                            if(!ioerr)
                            {
                                KPRINTF(2, ("sense data phase %ld bytes...\n", datalen));
                                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA)
                                {
                                    psdDelayMS(1);
                                }
                                ioerr = psdDoPipe(ncm->ncm_EPInPipe, scsicmd->scsi_SenseData, datalen);
                                scsicmd->scsi_SenseActual = psdGetPipeActual(ncm->ncm_EPInPipe);
                                if(ioerr == UHIOERR_STALL) /* Accept on stall */
                                {
                                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
                                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                                }
                                if((ioerr == UHIOERR_RUNTPACKET) || (ioerr == UHIOERR_OVERFLOW))
                                {
                                    KPRINTF(10, ("Extra or less data received, but ignored!\n"));
                                    ioerr = 0;
                                }

                                if(!ioerr)
                                {
                                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DELAY_DATA)
                                    {
                                        psdDelayMS(1);
                                    }
                                    KPRINTF(2, ("sense command status phase...\n"));
                                    ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                                    if(ioerr == UHIOERR_STALL) /* Retry on stall */
                                    {
                                        KPRINTF(2, ("stall...\n"));
                                        psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                                     USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
                                        ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                                        ioerr |= psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                                    }
                                    if(ioerr == UHIOERR_RUNTPACKET)
                                    {
                                        // well, retry then
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                          "Command (sense) status block truncated (%ld bytes), retrying...",
                                                          psdGetPipeActual(ncm->ncm_EPInPipe));
                                        ioerr = psdDoPipe(ncm->ncm_EPInPipe, &umscsw, UMSCSW_SIZEOF);
                                    }

                                    if(ioerr == UHIOERR_OVERFLOW)
                                    {
                                        KPRINTF(10, ("Extra Status received, but ignored!\n"));
                                        ioerr = 0;
                                    }
                                    if(!ioerr)
                                    {
                                        KPRINTF(2, ("sense Status:\n"
                                                    "  Signature: %08lx\n"
                                                    "  Tag      : %08lx\n"
                                                    "  Residue  : %08lx\n"
                                                    "  Status   : %02lx\n",
                                                    umscsw.dCSWSignature,
                                                    umscsw.dCSWTag,
                                                    umscsw.dCSWDataResidue,
                                                    umscsw.bCSWStatus));
                                        if(((umscsw.dCSWSignature != AROS_LONG2LE(0x53425355)) && (!(ncm->ncm_CDC->cdc_PatchFlags & PFF_CSS_BROKEN))) || (umscsw.dCSWTag != umscbw.dCBWTag))
                                        {
                                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                          "Illegal command (sense) status block (Sig:%08lx, Tag=%08lx/%08lx (TX/RX), Len=%ld)",
                                                          umscsw.dCSWSignature,
                                                          umscbw.dCBWTag,
                                                          umscsw.dCSWTag,
                                                          psdGetPipeActual(ncm->ncm_EPInPipe));
                                            scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                                            rioerr = HFERR_Phase;
                                            nBulkReset(ncm);
                                            continue;
                                        }
                                        /* ignore this: too many firmwares report shit */
                                        //scsicmd->scsi_SenseActual = datalen - AROS_LONG2LE(umscsw.dCSWDataResidue);
                                        if((scsicmd->scsi_SenseActual > 7) && ((AROS_LONG2BE(*((ULONG *) scsicmd->scsi_SenseData))>>8) == 0x555342) && (((ULONG *) scsicmd->scsi_SenseData)[1] == umscbw.dCBWTag))
                                        {
                                            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                          "Your MSD has a very bad firmware! Havoc!");
                                            scsicmd->scsi_Actual = 0;
                                            umscsw.bCSWStatus = USMF_CSW_FAIL;
                                        }

                                        if(umscsw.bCSWStatus)
                                        {
                                            /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                           "Sense failed: %ld",
                                                           umscsw.bCSWStatus);*/
                                            if(umscsw.bCSWStatus == USMF_CSW_PHASEERR)
                                            {
                                                nBulkReset(ncm);
                                            }
                                        } else {
                                            switch(scsicmd->scsi_SenseData[2] & SK_MASK)
                                            {
                                                case SK_ILLEGAL_REQUEST:
                                                case SK_NOT_READY:
                                                    retrycnt = 0;
                                                    break;
                                                case SK_DATA_PROTECT:
                                                    if(!ncm->ncm_WriteProtect)
                                                    {
                                                        ncm->ncm_WriteProtect = TRUE;
                                                        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                                                        {
                                                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                                                           "WriteProtect On: Sense Data Protect");
                                                        }
                                                    }
                                                    break;

                                                case SK_UNIT_ATTENTION:
                                                    if((ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT) &&
                                                       ((scsicmd->scsi_SenseData[12] == 0x28) ||
                                                       (scsicmd->scsi_SenseData[12] == 0x3A)))
                                                    {
                                                        ncm->ncm_ChangeCount++;
                                                        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                                                        {
                                                            psdAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                                                                           "Diskchange: Unit Attention (count = %ld)",
                                                                           ncm->ncm_ChangeCount);
                                                        }
                                                    }
                                                    break;
                                            }
                                            KPRINTF(10, ("Sense Key: %lx/%02lx/%02lx\n",
                                                        scsicmd->scsi_SenseData[2] & SK_MASK,
                                                        scsicmd->scsi_SenseData[12],
                                                        scsicmd->scsi_SenseData[13]));
                                            if(ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG)
                                            {
                                                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                                               "Cmd %s: Sense Key %lx/%02lx/%02lx",
                                                               cmdstrbuf,
                                                               scsicmd->scsi_SenseData[2] & SK_MASK,
                                                               scsicmd->scsi_SenseData[12],
                                                               scsicmd->scsi_SenseData[13]);
                                            }
                                        }
                                    } else {
                                        KPRINTF(10, ("Sense status failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) okay, but:", cmdstrbuf);
                                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                       "Sense status failed: %s (%ld)",
                                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                        nBulkReset(ncm);
                                    }
                                } else {
                                    KPRINTF(10, ("Sense data failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) okay, but:", cmdstrbuf);
                                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                                   "Sense data failed: %s (%ld)",
                                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                    nBulkReset(ncm);
                                }
                            } else {
                                KPRINTF(10, ("Sense block failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) okay, but:", cmdstrbuf);
                                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                               "Sense block failed: %s (%ld)",
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                                /*nBulkReset(ncm);*/
                            }
                        }
                        rioerr = HFERR_BadStatus;
                    }
                } else {
                    KPRINTF(10, ("Command status failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                  "Command status failed: %s (%ld)",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                    rioerr = HFERR_Phase;
                    nBulkReset(ncm);
                }
            } else {
                KPRINTF(10, ("Data phase failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
                psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                               "Data phase failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                rioerr = HFERR_Phase;
                nBulkReset(ncm);
            }
        } else {
            KPRINTF(10, ("Command block failed: %s (%ld)\n", psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr));
            scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
            rioerr = HFERR_Phase;
            if(ioerr == UHIOERR_TIMEOUT)
            {
                break;
            }
            psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Command block failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            nBulkReset(ncm);
        }
        if(!rioerr)
        {
            break;
        }
        KPRINTF(1, ("Retrying...\n"));
    } while(retrycnt--);
    nUnlockXFer(ncm);
    return(rioerr);
}
/* \\\ */
