/*
 * Copyright (c) 2010-2016 Broadcom Corporation
 * Copyright (c) 2018 Patrick Wildt <patrick@blueri.se>
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
 * Subset of OpenBSD sys/dev/sdmmc/if_bwfm_sdio.h ported for AROS.
 */

#ifndef BWFM_SDIO_H
#define BWFM_SDIO_H

/* Broadcom-specific CCCR (function 0) registers */
#define BWFM_SDIO_CCCR_CARDCAP                  0xf0
#define  BWFM_SDIO_CCCR_CARDCAP_CMD14_SUPPORT       (1 << 1)
#define  BWFM_SDIO_CCCR_CARDCAP_CMD14_EXT           (1 << 2)
#define  BWFM_SDIO_CCCR_CARDCAP_CMD_NODEC           (1 << 3)
#define BWFM_SDIO_CCCR_CARDCTRL                 0xf1
#define  BWFM_SDIO_CCCR_CARDCTRL_WLANRESET          (1 << 1)
#define BWFM_SDIO_CCCR_SEPINT                   0xf2

/* Function 1 miscellaneous registers (0x10000-0x1FFFF) */
#define BWFM_SDIO_WATERMARK                     0x10008
#define BWFM_SDIO_DEVICE_CTL                    0x10009
#define BWFM_SDIO_FUNC1_SBADDRLOW               0x1000A
#define BWFM_SDIO_FUNC1_SBADDRMID               0x1000B
#define BWFM_SDIO_FUNC1_SBADDRHIGH              0x1000C
#define BWFM_SDIO_FUNC1_CHIPCLKCSR              0x1000E
#define  BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_ALP           0x01
#define  BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HT            0x02
#define  BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_ILP           0x04
#define  BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL_REQ       0x08
#define  BWFM_SDIO_FUNC1_CHIPCLKCSR_HT_AVAIL_REQ        0x10
#define  BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HW_CLKREQ_OFF 0x20
#define  BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL           0x40
#define  BWFM_SDIO_FUNC1_CHIPCLKCSR_HT_AVAIL            0x80
#define BWFM_SDIO_FUNC1_SDIOPULLUP              0x1000F
#define BWFM_SDIO_FUNC1_WAKEUPCTRL              0x1001E
#define BWFM_SDIO_FUNC1_SLEEPCSR                0x1001F
#define  BWFM_SDIO_FUNC1_SLEEPCSR_KSO               (1 << 0)
#define  BWFM_SDIO_FUNC1_SLEEPCSR_DEVON             (1 << 1)

#define  BWFM_SDIO_DEVICE_CTL_CA_INT_ONLY           0x04

/* Silicon-backplane access windowing */
#define BWFM_SDIO_SB_OFT_ADDR_PAGE              0x08000
#define BWFM_SDIO_SB_OFT_ADDR_MASK              0x07FFF
#define BWFM_SDIO_SB_ACCESS_2_4B_FLAG           0x08000

/* SDIO_DEV (SDPCM) core registers (offsets from the SDIO_DEV core base) */
#define SDPCMD_INTSTATUS                        0x020
#define  SDPCMD_INTSTATUS_HMB_SW_MASK               0x000000f0
#define  SDPCMD_INTSTATUS_HMB_FRAME_IND             (1 << 6)    /* HMB_SW2 */
#define  SDPCMD_INTSTATUS_HMB_HOST_INT              (1 << 7)    /* HMB_SW3: needs mbox ack */
#define  SDPCMD_INTSTATUS_CHIPACTIVE                (1 << 29)
#define SDPCMD_HOSTINTMASK                      0x024
#define SDPCMD_TOSBMAILBOX                      0x040
#define  SDPCMD_TOSBMAILBOX_INT_ACK                 (1 << 1)
#define SDPCMD_TOSBMAILBOXDATA                  0x048
#define SDPCMD_TOHOSTMAILBOXDATA                0x04C
#define  SDPCMD_TOHOSTMAILBOXDATA_DEVREADY          (1 << 1)
#define  SDPCMD_TOHOSTMAILBOXDATA_FWREADY           (1 << 3)

/* SDPCM protocol version handshake (written to TOSBMAILBOXDATA) */
#define SDPCM_PROT_VERSION                      4
#define SDPCM_PROT_VERSION_SHIFT                16

/* SDPCM frame headers and channels */
struct bwfm_sdio_hwhdr
{
    uint16_t    frmlen;
    uint16_t    cksum;          /* ~frmlen */
};

struct bwfm_sdio_swhdr
{
    uint8_t     seqnr;
    uint8_t     chanflag;
    uint8_t     nextlen;
    uint8_t     dataoff;
    uint8_t     flowctl;
    uint8_t     maxseqnr;
    uint16_t    res0;
};

#define BWFM_SDIO_SWHDR_CHANNEL_CONTROL         0x00
#define BWFM_SDIO_SWHDR_CHANNEL_EVENT           0x01
#define BWFM_SDIO_SWHDR_CHANNEL_DATA            0x02
#define BWFM_SDIO_SWHDR_CHANNEL_GLOM            0x03
#define BWFM_SDIO_SWHDR_CHANNEL_MASK            0x0f

/* BCDC control message header (precedes the command payload) */
struct bwfm_bcdc_dcmd_hdr
{
    uint32_t    cmd;
    uint32_t    len;
    uint32_t    flags;
    uint32_t    status;
};

#define BWFM_BCDC_DCMD_ERROR                    (1 << 0)
#define BWFM_BCDC_DCMD_SET                      (1 << 1)
#define BWFM_BCDC_DCMD_ID_SET(x)                (((x) & 0xffff) << 16)
#define BWFM_BCDC_DCMD_ID_GET(x)                (((x) >> 16) & 0xffff)

/* BCDC/BDC data header (precedes data/event payloads, struct bwfm_bcdc_data_hdr) */
#define BWFM_BCDC_FLAG_PROTO_VER                2
#define BWFM_BCDC_FLAG_VER(x)                   (((x) & 0xf) << 4)

/* BWFMRxFrame() *info flags (low byte = SDPCM channel) */
#define BWFM_RX_EVENT                           0x100

/*
 * Blob download header for the "clmload" iovar (regulatory CLM data). The blob
 * is sent as a sequence of chunks, each prefixed with this header; the first
 * chunk sets BEGIN and the last sets END. Ported from OpenBSD bwfm_dload_data.
 */
struct bwfm_dload_data
{
    uint16_t    flag;
    uint16_t    type;
    uint32_t    len;
    uint32_t    crc;
    /* chunk data follows */
};

#define BWFM_DLOAD_FLAG_BEGIN                   (1 << 1)
#define BWFM_DLOAD_FLAG_END                     (1 << 2)
#define BWFM_DLOAD_FLAG_HANDLER_VER_1           (1 << 12)
#define BWFM_DLOAD_TYPE_CLM                     2

/* Firmware command numbers */
#define BWFM_C_UP                               2
#define BWFM_C_DOWN                             3
#define BWFM_C_SET_INFRA                        20
#define BWFM_C_GET_BSSID                        23
#define BWFM_C_SET_SSID                         26
#define BWFM_C_DISASSOC                         52
#define BWFM_C_SET_PM                           86
#define BWFM_C_SET_AP                           118
#define BWFM_C_SET_SCAN_CHANNEL_TIME            185
#define BWFM_C_SET_SCAN_UNASSOC_TIME            187
#define BWFM_C_SET_SCAN_PASSIVE_TIME            258
#define BWFM_C_GET_VAR                          262
#define BWFM_C_SET_VAR                          263
#define BWFM_C_SET_WSEC_PMK                     268

/* Security setup for join (wsec / wpa_auth / auth / mfp iovars) */
#define BWFM_AUTH_OPEN                          0
#define BWFM_MFP_NONE                           0
#define BWFM_WPA_AUTH_DISABLED                  0
#define BWFM_WPA_AUTH_WPA2_PSK                  (1 << 7)
#define BWFM_WSEC_NONE                          0
#define BWFM_WSEC_AES                           (1 << 2)
#define BWFM_WSEC_PASSPHRASE                    (1 << 0)

/* E_LINK event_msg flags: bit0 = link up */
#define BWFM_EVENT_FLAG_LINK_UP                 (1 << 0)

/* Power-management modes (BWFM_C_SET_PM) */
#define BWFM_PM_CAM                             0
#define BWFM_PM_FAST_PS                         2

#endif /* BWFM_SDIO_H */
