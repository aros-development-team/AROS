/*
 * Copyright (c) 2002-2008 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2008 Atheros Communications, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id$
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_desc.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416desc.h"

/*
 * Start receive at the PCU engine
 */
void
ar5416StartPcuReceive(struct ath_hal *ah)
{
	struct ath_hal_private *ahp = AH_PRIVATE(ah);

	OS_REG_CLR_BIT(ah, AR_DIAG_SW, AR_DIAG_RX_DIS | AR_DIAG_RX_ABORT);

	HALDEBUG(ah, HAL_DEBUG_RX, "%s: Start PCU Receive \n", __func__);
	ar5212EnableMibCounters(ah);
	/* NB: restore current settings */
	ar5212AniReset(ah, ahp->ah_curchan, ahp->ah_opmode, AH_TRUE);
}

/*
 * Stop receive at the PCU engine
 * and abort current frame in PCU
 */
void
ar5416StopPcuReceive(struct ath_hal *ah)
{
	OS_REG_SET_BIT(ah, AR_DIAG_SW, AR_DIAG_RX_DIS | AR_DIAG_RX_ABORT);
    
	HALDEBUG(ah, HAL_DEBUG_RX, "%s: Stop PCU Receive \n", __func__);
	ar5212DisableMibCounters(ah);
}

/*
 * Initialize RX descriptor, by clearing the status and setting
 * the size (and any other flags).
 */
HAL_BOOL
ar5416SetupRxDesc(struct ath_hal *ah, struct ath_desc *ds,
    uint32_t size, u_int flags)
{
	struct ar5416_desc *ads = AR5416DESC(ds);

	HALASSERT((size &~ AR_BufLen) == 0);

	ads->ds_ctl1 = size & AR_BufLen;
	if (flags & HAL_RXDESC_INTREQ)
		ads->ds_ctl1 |= AR_RxIntrReq;

	/* this should be enough */
	ads->ds_rxstatus8 &= ~AR_RxDone;

	return AH_TRUE;
}

/*
 * Process an RX descriptor, and return the status to the caller.
 * Copy some hardware specific items into the software portion
 * of the descriptor.
 *
 * NB: the caller is responsible for validating the memory contents
 *     of the descriptor (e.g. flushing any cached copy).
 */
HAL_STATUS
ar5416ProcRxDesc(struct ath_hal *ah, struct ath_desc *ds,
    uint32_t pa, struct ath_desc *nds, uint64_t tsf,
    struct ath_rx_status *rs)
{
	struct ar5416_desc *ads = AR5416DESC(ds);
	struct ar5416_desc *ands = AR5416DESC(nds);

	if ((ads->ds_rxstatus8 & AR_RxDone) == 0)
		return HAL_EINPROGRESS;
	/*
	 * Given the use of a self-linked tail be very sure that the hw is
	 * done with this descriptor; the hw may have done this descriptor
	 * once and picked it up again...make sure the hw has moved on.
	 */
	if ((ands->ds_rxstatus8 & AR_RxDone) == 0
	    && OS_REG_READ(ah, AR_RXDP) == pa)
		return HAL_EINPROGRESS;

	rs->rs_status = 0;
	rs->rs_flags = 0;

	rs->rs_datalen = ads->ds_rxstatus1 & AR_DataLen;
	rs->rs_tstamp =  ads->AR_RcvTimestamp;

	/* XXX what about KeyCacheMiss? */

	rs->rs_rssi = MS(ads->ds_rxstatus4, AR_RxRSSICombined);
	rs->rs_rssi_ctl[0] = MS(ads->ds_rxstatus0, AR_RxRSSIAnt00);
	rs->rs_rssi_ctl[1] = MS(ads->ds_rxstatus0, AR_RxRSSIAnt01);
	rs->rs_rssi_ctl[2] = MS(ads->ds_rxstatus0, AR_RxRSSIAnt02);
	rs->rs_rssi_ext[0] = MS(ads->ds_rxstatus4, AR_RxRSSIAnt10);
	rs->rs_rssi_ext[1] = MS(ads->ds_rxstatus4, AR_RxRSSIAnt11);
	rs->rs_rssi_ext[2] = MS(ads->ds_rxstatus4, AR_RxRSSIAnt12);

	if (ads->ds_rxstatus8 & AR_RxKeyIdxValid)
		rs->rs_keyix = MS(ads->ds_rxstatus8, AR_KeyIdx);
	else
		rs->rs_keyix = HAL_RXKEYIX_INVALID;

	/* NB: caller expected to do rate table mapping */
	rs->rs_rate = RXSTATUS_RATE(ah, ads);
	rs->rs_more = (ads->ds_rxstatus1 & AR_RxMore) ? 1 : 0;

	rs->rs_isaggr = (ads->ds_rxstatus8 & AR_RxAggr) ? 1 : 0;
	rs->rs_moreaggr = (ads->ds_rxstatus8 & AR_RxMoreAggr) ? 1 : 0;
	rs->rs_antenna = MS(ads->ds_rxstatus3, AR_RxAntenna);

	if (ads->ds_rxstatus3 & AR_GI)
		rs->rs_flags |= HAL_RX_GI;
	if (ads->ds_rxstatus3 & AR_2040)
		rs->rs_flags |= HAL_RX_2040;

	if (ads->ds_rxstatus8 & AR_PreDelimCRCErr)
		rs->rs_flags |= HAL_RX_DELIM_CRC_PRE;
	if (ads->ds_rxstatus8 & AR_PostDelimCRCErr)
		rs->rs_flags |= HAL_RX_DELIM_CRC_POST;
	if (ads->ds_rxstatus8 & AR_DecryptBusyErr)
		rs->rs_flags |= HAL_RX_DECRYPT_BUSY;
	if (ads->ds_rxstatus8 & AR_HiRxChain)
		rs->rs_flags |= HAL_RX_HI_RX_CHAIN;

	if ((ads->ds_rxstatus8 & AR_RxFrameOK) == 0) {
		/*
		 * These four bits should not be set together.  The
		 * 5416 spec states a Michael error can only occur if
		 * DecryptCRCErr not set (and TKIP is used).  Experience
		 * indicates however that you can also get Michael errors
		 * when a CRC error is detected, but these are specious.
		 * Consequently we filter them out here so we don't
		 * confuse and/or complicate drivers.
		 */
		if (ads->ds_rxstatus8 & AR_CRCErr)
			rs->rs_status |= HAL_RXERR_CRC;
		else if (ads->ds_rxstatus8 & AR_PHYErr) {
			u_int phyerr;

			rs->rs_status |= HAL_RXERR_PHY;
			phyerr = MS(ads->ds_rxstatus8, AR_PHYErrCode);
			rs->rs_phyerr = phyerr;
		} else if (ads->ds_rxstatus8 & AR_DecryptCRCErr)
			rs->rs_status |= HAL_RXERR_DECRYPT;
		else if (ads->ds_rxstatus8 & AR_MichaelErr)
			rs->rs_status |= HAL_RXERR_MIC;
	}

	return HAL_OK;
}
#endif /* AH_SUPPORT_AR5416 */
