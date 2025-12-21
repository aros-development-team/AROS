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

/* /// "nCBIRequestSense()" */
LONG nCBIRequestSense(struct NepClassMS *ncm, UBYTE *senseptr, ULONG datalen)
{
    LONG ioerr;
    UBYTE sensecmd[12];
    LONG actual = 0;
    struct UsbMSCBIStatusWrapper umscsw;

    memset(sensecmd, 0, 12);
    senseptr[2] = SK_ILLEGAL_REQUEST;
    sensecmd[0] = SCSI_REQUEST_SENSE;
    sensecmd[1] = 0x00;
    sensecmd[2] = 0x00;
    sensecmd[3] = 0x00;
    sensecmd[4] = datalen;
    sensecmd[5] = 0;
    KPRINTF(2, ("sense command block phase...\n"));

    /*psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);*/
    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                 UMSR_ADSC, 0, (ULONG) ncm->ncm_UnitIfNum);
    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, sensecmd, ((ncm->ncm_CSType == MS_ATAPI_SUBCLASS) ||
                      (ncm->ncm_CSType == MS_FDDATAPI_SUBCLASS) ||
                      (ncm->ncm_CSType == MS_UFI_SUBCLASS)) ? (ULONG) 12 : (ULONG) 6);
    if(!ioerr)
    {
        if(ncm->ncm_CDC->cdc_PatchFlags & PFF_CLEAR_EP)
        {
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
        }

        KPRINTF(2, ("sense data phase %ld bytes...\n", datalen));
        ioerr = psdDoPipe(ncm->ncm_EPInPipe, senseptr, datalen);
        actual = psdGetPipeActual(ncm->ncm_EPInPipe);
        if(ioerr == UHIOERR_STALL)
        {
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
            psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
        }
        if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
        {
            KPRINTF(2, ("sense command status phase...\n"));
            if(ncm->ncm_TPType == MS_PROTO_CBI)
            {
                umscsw.bType = 0;
                umscsw.bValue = USMF_CSW_PHASEERR;
                ioerr = psdDoPipe(ncm->ncm_EPIntPipe, &umscsw, sizeof(struct UsbMSCBIStatusWrapper));
                if(ioerr && (ioerr != UHIOERR_RUNTPACKET))
                {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                   "Status interrupt failed: %s (%ld)",
                                   psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    return(0);
                }
                umscsw.bValue &= USMF_CSW_PERSIST; /* mask out other bits */
            } else {
                umscsw.bType = 0;
                umscsw.bValue = USMF_CSW_PASS;
                ioerr = 0;
            }
            if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
            {
                KPRINTF(2, ("sense Status:\n"
                            "  Status   : %02lx\n",
                            umscsw.bValue));
                if(umscsw.bValue)
                {
                    /*psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                   "Sense failed: %ld",
                                   umscsw.bValue);*/
                    if(umscsw.bValue == USMF_CSW_PHASEERR)
                    {
                        return(0);
                    }
                } else {
                    switch(senseptr[2] & SK_MASK)
                    {
                        case SK_UNIT_ATTENTION:
                            if((senseptr[12] == 0x28) ||
                               (senseptr[12] == 0x3A))
                            {
                                ncm->ncm_ChangeCount++;
                            }
                            break;
                    }

                    if((ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG) && (senseptr[2] & SK_MASK))
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "Request Sense Key %lx/%02lx/%02lx",
                                       senseptr[2] & SK_MASK,
                                       senseptr[12],
                                       senseptr[13]);
                    }
                }
            } else {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                              "Sense status failed: %s (%ld)",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            }
        } else {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                           "Sense data failed: %s (%ld)",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        }
    } else {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                       "Sense block failed: %s (%ld)",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    return(actual);
}
/* \\\ */

/* /// "nScsiDirectCBI()" */
LONG nScsiDirectCBI(struct NepClassMS *ncm, struct SCSICmd *scsicmd)
{
    LONG ioerr;
    struct PsdPipe *pp;
    struct PsdPipe *backpp;
    struct UsbMSCBIStatusWrapper umscsw;
    ULONG datalen;
    LONG rioerr;
    UWORD retrycnt = 0;
    UBYTE sensedata[18];
    UBYTE *senseptr;
    UBYTE asc;
    BOOL datadone;
    BOOL statusdone;
    UBYTE cmdstrbuf[16*3+2];

    GM_UNIQUENAME(nHexString)(scsicmd->scsi_Command, (ULONG) (scsicmd->scsi_CmdLength < 16 ? scsicmd->scsi_CmdLength : 16), cmdstrbuf);

    if(scsicmd->scsi_Flags & 0x80) /* Autoretry */
    {
        retrycnt = 1;
    }
    scsicmd->scsi_Status = SCSI_GOOD;
    nLockXFer(ncm);
    do
    {
        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }
        rioerr = 0;
        datalen = scsicmd->scsi_Length;

        KPRINTF(2, ("command block phase, tag %08lx, len %ld, flags %02lx...\n",
                scsicmd, scsicmd->scsi_CmdLength, scsicmd->scsi_Flags));

        KPRINTF(2, ("command: %s\n", cmdstrbuf));

        //nBulkClear(ncm);
        scsicmd->scsi_CmdActual = scsicmd->scsi_CmdLength;
        /*if(datalen)
        {
            psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                         USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                         (ULONG) ((scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInNum|URTF_IN : ncm->ncm_EPOutNum));
            ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
        }*/
        psdPipeSetup(ncm->ncm_EP0Pipe, URTF_CLASS|URTF_INTERFACE,
                     UMSR_ADSC, 0, (ULONG) ncm->ncm_UnitIfNum);
        ioerr = psdDoPipe(ncm->ncm_EP0Pipe, scsicmd->scsi_Command, (ULONG) scsicmd->scsi_CmdLength);

        if(ncm->ncm_DenyRequests)
        {
            rioerr = HFERR_Phase;
            break;
        }
        if(!ioerr)
        {
            datadone = statusdone = FALSE;
            if(datalen)
            {
                if(ncm->ncm_CDC->cdc_PatchFlags & PFF_CLEAR_EP)
                {
                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                                 (ULONG) ((scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInNum|URTF_IN : ncm->ncm_EPOutNum));
                    ioerr = psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                }

                KPRINTF(2, ("data phase %ld bytes...\n", datalen));
                pp = (scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInPipe : ncm->ncm_EPOutPipe;

                if(ncm->ncm_TPType == MS_PROTO_CBI)
                {
                    /* okay, this is a major pain in the arse.
                       we have to do this asynchroneously */
                    umscsw.bType = 0;
                    umscsw.bValue = USMF_CSW_PHASEERR;
                    psdSendPipe(ncm->ncm_EPIntPipe, &umscsw, sizeof(struct UsbMSCBIStatusWrapper));
                    psdSendPipe(pp, scsicmd->scsi_Data, datalen);
                    do
                    {
                        WaitPort(ncm->ncm_TaskMsgPort);
                        while((backpp = (struct PsdPipe *) GetMsg(ncm->ncm_TaskMsgPort)))
                        {
                            if(backpp == pp)
                            {
                                /* data transfer finished */
                                datadone = TRUE;
                            }
                            else if(backpp == ncm->ncm_EPIntPipe)
                            {
                                /* status returned */
                                statusdone = TRUE;
                            }
                        }
                    } while(!statusdone);
                    if(!datadone)
                    {
                        psdAbortPipe(pp);
                        psdWaitPipe(pp);
                        ioerr = 0;
                    } else {
                        ioerr = psdGetPipeError(pp);
                    }

                } else {
                    ioerr = psdDoPipe(pp, scsicmd->scsi_Data, datalen);
                }

                scsicmd->scsi_Actual = psdGetPipeActual(pp);
                if(ioerr == UHIOERR_OVERFLOW)
                {
                    KPRINTF(10, ("Extra Data received, but ignored!\n"));
                    ioerr = 0;
                }
                if(ioerr == UHIOERR_STALL) /* Accept on stall */
                {
                    KPRINTF(2, ("stall...\n"));
                    //nBulkClear(ncm);
                    psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                 USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT,
                                 (ULONG) ((scsicmd->scsi_Flags & SCSIF_READ) ? ncm->ncm_EPInNum|URTF_IN : ncm->ncm_EPOutNum));
                    psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                    ioerr = 0;
                }
            } else {
                ioerr = 0;
                scsicmd->scsi_Actual = 0;
            }
            if(!ioerr)
            {
                if(ncm->ncm_TPType == MS_PROTO_CBI)
                {
                    /* wait for status on interrupt pipe */
                    ioerr = psdDoPipe(ncm->ncm_EPIntPipe, &umscsw, sizeof(struct UsbMSCBIStatusWrapper));
                } else {
                    umscsw.bType = 0;
                    umscsw.bValue = USMF_CSW_PASS;
                }
                if((!ioerr) || (ioerr == UHIOERR_RUNTPACKET))
                {
                    if(ioerr)
                    {
                        ioerr = 0;
                    }
                    KPRINTF(2, ("Status:\n"
                                "  Status   : %02lx\n",
                                umscsw.bValue));
                    if(umscsw.bValue)
                    {
                        scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                        rioerr = HFERR_BadStatus;
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname),
                                       "Command (%s) failed: %ld", cmdstrbuf, umscsw.bValue);
                    } else {
                        scsicmd->scsi_Status = SCSI_GOOD;
                    }

                    /* Autosensing required? */
                    if((scsicmd->scsi_Status != SCSI_GOOD) && (scsicmd->scsi_Flags & SCSIF_AUTOSENSE))
                    {
                        senseptr = scsicmd->scsi_SenseData;
                        datalen = scsicmd->scsi_SenseLength;
                        if(datalen < 18)
                        {
                            senseptr = sensedata;
                            datalen = 18;
                        }
                        if(!(scsicmd->scsi_SenseActual = nCBIRequestSense(ncm, senseptr, datalen)))
                        {
                            nBulkReset(ncm);
                            break;
                        }
                        scsicmd->scsi_SenseActual = (scsicmd->scsi_SenseLength > 18) ? 18 : scsicmd->scsi_SenseLength;
                        if(senseptr != scsicmd->scsi_SenseData)
                        {
                            CopyMem(senseptr, scsicmd->scsi_SenseData, scsicmd->scsi_SenseLength);
                        }
                        asc = scsicmd->scsi_SenseData[12];
                        if((scsicmd->scsi_SenseData[2] & SK_MASK) == SK_UNIT_ATTENTION)
                        {
                            switch(asc)
                            {
                                case 0x28:
                                case 0x3A:
                                    if(ncm->ncm_CDC->cdc_PatchFlags & PFF_REM_SUPPORT)
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
                        }
                        if((ncm->ncm_CDC->cdc_PatchFlags & PFF_DEBUG) && (scsicmd->scsi_SenseData[2] & SK_MASK))
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
                    if(ioerr == UHIOERR_STALL)
                    {
                        psdPipeSetup(ncm->ncm_EP0Pipe, URTF_STANDARD|URTF_ENDPOINT,
                                     USR_CLEAR_FEATURE, UFS_ENDPOINT_HALT, (ULONG) ncm->ncm_EPInNum|URTF_IN);
                        psdDoPipe(ncm->ncm_EP0Pipe, NULL, 0);
                        ioerr = 0;
                    }
                    if(ioerr)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) GM_UNIQUENAME(libname), "Command (%s) failed:", cmdstrbuf);
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) GM_UNIQUENAME(libname),
                                       "Command status failed: %s (%ld)",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                        scsicmd->scsi_Status = SCSI_CHECK_CONDITION;
                        rioerr = HFERR_Phase;
                        nBulkReset(ncm);
                    }
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
            if(ioerr == UHIOERR_STALL)
            {
                KPRINTF(2, ("stall...\n"));
                nBulkClear(ncm);
                ioerr = psdDoPipe(ncm->ncm_EP0Pipe, scsicmd->scsi_Command, (ULONG) scsicmd->scsi_CmdLength);
            }
            if(ioerr)
            {
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
