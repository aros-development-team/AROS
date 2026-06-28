/*
 * Copyright (c) 2010-2016 Broadcom Corporation
 * Copyright (c) 2016,2017 Patrick Wildt <patrick@blueri.se>
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
 * Scan/event structures, subset of OpenBSD sys/dev/ic/bwfmreg.h ported for
 * AROS. Multi-byte protocol fields are little-endian except the BRCM event
 * header (event_type/status are big-endian - use the be helpers).
 */

#ifndef BWFM_SCAN_H
#define BWFM_SCAN_H

#include <inttypes.h>

#define BWFM_MAX_SSID_LEN       32
#define ETHER_ADDR_LEN          6

/* --- escan request --------------------------------------------------- */

struct bwfm_ssid
{
    uint32_t    len;
    uint8_t     ssid[BWFM_MAX_SSID_LEN];
} __attribute__((packed));

struct bwfm_scan_params_v0
{
    struct bwfm_ssid ssid;
    uint8_t     bssid[ETHER_ADDR_LEN];
    uint8_t     bss_type;
    uint8_t     scan_type;
    uint32_t    nprobes;
    uint32_t    active_time;
    uint32_t    passive_time;
    uint32_t    home_time;
    uint32_t    channel_num;
    uint16_t    channel_list[1];
} __attribute__((packed));

#define BWFM_SCANTYPE_PASSIVE   1
#define DOT11_BSSTYPE_ANY       2

struct bwfm_escan_params_v0
{
    uint32_t    version;
#define BWFM_ESCAN_REQ_VERSION  1
    uint16_t    action;
#define WL_ESCAN_ACTION_START   1
    uint16_t    sync_id;
    struct bwfm_scan_params_v0 scan_params;
} __attribute__((packed));

/* --- join / association ---------------------------------------------- */

struct bwfm_assoc_params
{
    uint8_t     bssid[ETHER_ADDR_LEN];
    uint16_t    pad;
    uint32_t    chanspec_num;
    uint16_t    chanspec_list[1];       /* empty for an any-channel join */
} __attribute__((packed));

struct bwfm_join_scan_params
{
    uint8_t     scan_type;
    uint8_t     pad[3];
    uint32_t    nprobes;
    uint32_t    active_time;
    uint32_t    passive_time;
    uint32_t    home_time;
} __attribute__((packed));

struct bwfm_ext_join_params
{
    struct bwfm_ssid ssid;
    struct bwfm_join_scan_params scan;
    struct bwfm_assoc_params assoc;
} __attribute__((packed));

struct bwfm_join_params       /* legacy BWFM_C_SET_SSID fallback */
{
    struct bwfm_ssid ssid;
    struct bwfm_assoc_params assoc;
} __attribute__((packed));

/* Passphrase/PMK for the firmware-internal WPA supplicant (BWFM_C_SET_WSEC_PMK) */
struct bwfm_wsec_pmk
{
    uint16_t    key_len;
    uint16_t    flags;
    uint8_t     key[2 * 32 + 1];
} __attribute__((packed));

/* --- escan results --------------------------------------------------- */

#define BWFM_MCSSET_LEN         16

/* Full bss_info layout (ported from OpenBSD bwfmreg.h) so ie_offset/ie_length
 * land at the correct offsets - the IEs (incl. the RSN/WPA element wpa_supplicant
 * needs) follow at (uint8_t *)bss + ie_offset for ie_length bytes. */
struct bwfm_bss_info
{
    uint32_t    version;
    uint32_t    length;
    uint8_t     bssid[ETHER_ADDR_LEN];
    uint16_t    beacon_period;
    uint16_t    capability;
    uint8_t     ssid_len;
    uint8_t     ssid[BWFM_MAX_SSID_LEN];
    uint8_t     pad0;
    uint32_t    nrates;
    uint8_t     rates[16];
    uint16_t    chanspec;
    uint16_t    atim_window;
    uint8_t     dtim_period;
    uint8_t     pad1;
    uint16_t    rssi;
    uint8_t     phy_noise;
    uint8_t     n_cap;
    uint16_t    pad2;
    uint32_t    nbss_cap;
    uint8_t     ctl_ch;
    uint8_t     pad3[3];
    uint32_t    reserved32[1];
    uint8_t     flags;
    uint8_t     reserved[3];
    uint8_t     basic_mcs[BWFM_MCSSET_LEN];
    uint16_t    ie_offset;
    uint16_t    pad4;
    uint32_t    ie_length;
    uint16_t    snr;
} __attribute__((packed));

struct bwfm_escan_results
{
    uint32_t    buflen;
    uint32_t    version;
    uint16_t    sync_id;
    uint16_t    bss_count;
    struct bwfm_bss_info bss_info[1];
} __attribute__((packed));

/* --- async event header (precedes event payload) --------------------- */

struct bwfm_ether_header
{
    uint8_t     dst[ETHER_ADDR_LEN];
    uint8_t     src[ETHER_ADDR_LEN];
    uint16_t    ether_type;
} __attribute__((packed));

struct bwfm_ethhdr
{
    uint16_t    subtype;
    uint16_t    length;
    uint8_t     version;
    uint8_t     oui[3];
    uint16_t    usr_subtype;
} __attribute__((packed));

struct bwfm_event_msg
{
    uint16_t    version;
    uint16_t    flags;
    uint32_t    event_type;     /* big-endian */
    uint32_t    status;         /* big-endian */
    uint32_t    reason;
    uint32_t    auth_type;
    uint32_t    datalen;
    uint8_t     addr[ETHER_ADDR_LEN];
    char        ifname[16];
    uint8_t     ifidx;
    uint8_t     bsscfgidx;
} __attribute__((packed));

struct bwfm_event
{
    struct bwfm_ether_header ehdr;
#define BWFM_ETHERTYPE_LINK_CTL 0x886c
    struct bwfm_ethhdr hdr;
    struct bwfm_event_msg msg;
} __attribute__((packed));

/* BCDC data header that precedes event/data payloads on the SDPCM bus */
struct bwfm_bcdc_data_hdr
{
    uint8_t     flags;
    uint8_t     priority;
    uint8_t     flags2;
    uint8_t     data_offset;    /* in 4-byte words */
} __attribute__((packed));

/* Event types / status (subset) */
#define BWFM_E_SET_SSID         0
#define BWFM_E_AUTH             3
#define BWFM_E_DEAUTH           5
#define BWFM_E_ASSOC            7
#define BWFM_E_DISASSOC         11
#define BWFM_E_LINK             16
#define BWFM_E_EAPOL_MSG        25
#define BWFM_E_IF               54
#define BWFM_E_ESCAN_RESULT     69
#define BWFM_E_LAST             139
#define BWFM_EVENT_MASK_LEN     ((BWFM_E_LAST + 7) / 8)

#define BWFM_E_STATUS_SUCCESS   0
#define BWFM_E_STATUS_PARTIAL   8

/* Result handed back to a scan caller */
struct bwfm_scanresult
{
    uint8_t     bssid[ETHER_ADDR_LEN];
    uint8_t     ssid_len;
    uint8_t     ssid[BWFM_MAX_SSID_LEN];
    int16_t     rssi;
    uint16_t    chanspec;
};

#endif /* BWFM_SCAN_H */
