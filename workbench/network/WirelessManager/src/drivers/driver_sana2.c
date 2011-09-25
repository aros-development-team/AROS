/*
 * WPA Supplicant - AmigaOS/MorphOS/AROS SANA-II driver interface
 * Copyright (c) 2010-2011, Neil Cafferkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "driver.h"
#include "eloop.h"

#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <devices/sana2.h>
#include <devices/sana2wireless.h>
#include <devices/newstyle.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#ifndef UPINT
#ifdef __AROS__
typedef IPTR UPINT;
typedef SIPTR PINT;
#else
typedef ULONG UPINT;
typedef LONG PINT;
#endif
#endif

#define ETH_MTU 1500
#define MAX_SSID_LEN 32

#define MAX_TX_POWER 15

#define PUDDLE_SIZE 2000
#define MLME_REQ_COUNT 5
#define NULL_INTERVAL 30

int using_wep = 0;

void wpa_scan_results_free(struct wpa_scan_results *res);

static const char device_dir[] = "Networks/";

static const u8 broadcast_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

#ifdef  __AROS__
static BOOL wpa_driver_sana2_buffer_hook(UBYTE *dest, UBYTE *src,
	ULONG size)
{
	CopyMem(src, dest, size);
	return TRUE;
}
#else
/* m68k code to do the same as above */
static const ULONG wpa_driver_sana2_buffer_hook[] =
	{0xC1492C78, 0x00044EAE, 0xFD907001, 0x4E754E71};
#endif

static const struct TagItem buffer_tags[] = {
	{S2_CopyToBuff, (UPINT)wpa_driver_sana2_buffer_hook},
	{S2_CopyFromBuff, (UPINT)wpa_driver_sana2_buffer_hook},
	{TAG_END, 0}
};

struct wpa_driver_sana2_data {
	void *ctx;
	char *device_name;
	struct MsgPort *port;
	struct IOSana2Req *request;
	struct IOSana2Req *event_request;
	struct IOSana2Req *eapol_request;
	struct IOSana2Req *mlme_requests[MLME_REQ_COUNT];
	ULONG event_mask;
	u8 addr[ETH_ALEN];
	u8 bssid[ETH_ALEN];
	char ssid[MAX_SSID_LEN + 1];
	int freq;
	int device_opened;
	int hard_mac;
	int scanning;
	int online;
};

static int b_rates[] = {
	10,
	20,
	55,
	110
};

static int g_rates[] = {
	10,
	20,
	55,
	110,
	60,
	90,
	120,
	180,
	240,
	360,
	480,
	540
};


static int wpa_driver_sana2_get_ssid(void *priv, u8 *ssid)
{
	struct wpa_driver_sana2_data *drv = priv;
	u16 len;

	len = strlen(drv->ssid);
	os_memcpy(ssid, drv->ssid, len);

	return len;
}


static int wpa_driver_sana2_get_bssid(void *priv, u8 *bssid)
{
	struct wpa_driver_sana2_data *drv = priv;
	u8 *cur_bssid = NULL;
	struct TagItem *tag_list;
	struct IOSana2Req *request = drv->request;
	APTR pool;

	request->ios2_Req.io_Command = S2_GETNETWORKINFO;
	pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, PUDDLE_SIZE, PUDDLE_SIZE);
	request->ios2_Data = pool;
	if (pool != NULL && DoIO((APTR)request) == 0) {
		tag_list = request->ios2_StatData;
		cur_bssid = (u8 *)GetTagData(S2INFO_BSSID, (UPINT)NULL,
			tag_list);
	}
	if (cur_bssid != NULL) {
		os_memcpy(bssid, cur_bssid, ETH_ALEN);
		os_memcpy(drv->bssid, cur_bssid, ETH_ALEN);
	}

	return (cur_bssid != NULL) ? 0 : 1;
}


static int wpa_driver_sana2_scan(void *priv,
	struct wpa_driver_scan_params *params)
{
	struct wpa_driver_sana2_data *drv = priv;
	int err = 0;
	struct IOSana2Req *request = drv->request;
	const u8 *ssid;
	size_t ssid_len;
	TEXT *ssid_str = NULL;
	struct TagItem *tag_list;
	APTR pool;

	if (drv->scanning) {
		printf("[scan] MULTIPLE CONCURRENT SCAN REQUESTS!\n");
		Delay(200);
		return 1;
	}

	pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR, PUDDLE_SIZE, PUDDLE_SIZE);
	if (pool == NULL)
		err = 1;

	if (err == 0) {
		ssid = params->ssids[0].ssid;
		ssid_len = params->ssids[0].ssid_len;
		if (ssid != NULL) {
			ssid_str = AllocPooled(pool, ssid_len + 1);
			if (ssid_str != NULL) {
				CopyMem(ssid, ssid_str, ssid_len);
				ssid_str[ssid_len] = '\0';
			} else
				err = 1;
		}
	}

	if (err == 0) {
		tag_list = AllocPooled(pool, sizeof(struct TagItem) * 2);
		if (tag_list == NULL)
			err = 1;
	}

	if (err == 0) {
		tag_list[0].ti_Tag = S2INFO_SSID;
		tag_list[0].ti_Data = (UPINT)ssid_str;
		tag_list[1].ti_Tag = TAG_END;

		request->ios2_Req.io_Command = S2_GETNETWORKS;
		request->ios2_Data = pool;
		request->ios2_StatData = tag_list;
		SendIO((APTR)request);
		drv->scanning = 1;
	}

	return err;
}


static struct wpa_scan_results * wpa_driver_sana2_get_scan_results(void *priv)
{
	struct wpa_driver_sana2_data *drv = priv;
	int err = 0;
	struct IOSana2Req *request = drv->request;
	struct wpa_scan_results *results;
	size_t i, count;
	struct TagItem **tag_lists, *tag;
	TEXT *str;
	UPINT num;
	u8 *ie;
	u16 ie_len, ssid_len, *data;

	if (request->ios2_Req.io_Command != S2_GETNETWORKS) return NULL;

	count = request->ios2_DataLength;
	tag_lists = request->ios2_StatData;

	results = os_zalloc(sizeof(struct wpa_scan_results));
	if (results == NULL) {
		err = 1;
	}

	if (err == 0) {
		results->res =
			os_zalloc(count * sizeof(struct wpa_scan_res *));
		if (results->res != NULL)
			results->num = count;
		else
			err = 1;
	}

	for (i = 0; i < count && err == 0; i++) {
		struct wpa_scan_res *res;
		data = (u16 *)GetTagData(S2INFO_InfoElements,
			(UPINT)NULL, tag_lists[i]);
		if (data != NULL)
			ie_len = data[0];
		else
			ie_len = MAX_SSID_LEN + 2;
		res = results->res[i] =
			os_zalloc(sizeof(struct wpa_scan_res) + ie_len);
		if (res == NULL)
			err = 1;

		if (err == 0) {
			tag = FindTagItem(S2INFO_BSSID, tag_lists[i]);
			if (tag != NULL)
				os_memcpy(res->bssid, (void *)tag->ti_Data,
					ETH_ALEN);

			num = GetTagData(S2INFO_Channel, 0, tag_lists[i]);
			if (num == 14)
				res->freq = 2484;
			else if (num != 0)
				res->freq = 2407 + num * 5;

			res->beacon_int = GetTagData(S2INFO_BeaconInterval,
				0, tag_lists[i]);

			res->caps = GetTagData(S2INFO_Capabilities, 0,
				tag_lists[i]);

			res->level = GetTagData(S2INFO_Signal, 0, tag_lists[i]);

			res->noise = GetTagData(S2INFO_Noise, 0, tag_lists[i]);

			if (res->level > res->noise)
				res->qual = res->level - res->noise;

			/* Copy all IEs if available, or make fake IEs
			 * containing just SSID */
			ie = (u8 *)(res + 1);
			if (data != NULL) {
				res->ie_len = ie_len;
				os_memcpy(ie, data + 1, ie_len);
			} else {
				str = (TEXT *)GetTagData(S2INFO_SSID,
					(UPINT)NULL, tag_lists[i]);
				if (str != NULL) {
					ssid_len = strlen((char *)str);
					res->ie_len = ssid_len + 2;
					ie[0] = 0;
					ie[1] = ssid_len;
					os_memcpy(ie + 2, str, ssid_len);
				}
			}

		}
	}
	DeletePool(request->ios2_Data);

	if (err != 0) {
		wpa_scan_results_free(results);
		results = NULL;
	}

	drv->scanning = 0;
	return results;
}


static int wpa_driver_sana2_cipher(int cipher)
{
	switch (cipher) {
	case CIPHER_NONE:
		return S2ENC_NONE;
	case CIPHER_WEP40:
	case CIPHER_WEP104:
		return S2ENC_WEP;
	case CIPHER_TKIP:
		return S2ENC_TKIP;
	case CIPHER_CCMP:
		return S2ENC_CCMP;
	default:
		return 0;
	}
}


static int wpa_driver_sana2_band(enum hostapd_hw_mode mode)
{
	switch (mode) {
	case HOSTAPD_MODE_IEEE80211B:
		return S2BAND_B;
	case HOSTAPD_MODE_IEEE80211G:
		return S2BAND_G;
	case HOSTAPD_MODE_IEEE80211A:
		return S2BAND_A;
	default:
		return 0;
	}
}


static int wpa_driver_sana2_set_key(const char *ifname, void *priv,
	enum wpa_alg alg, const u8 *addr, int key_idx, int set_tx,
	const u8 *seq, size_t seq_len, const u8 *key, size_t key_len)
{
	struct wpa_driver_sana2_data *drv = priv;
	int err = 0;
	struct IOSana2Req *request = drv->request;

	request->ios2_Req.io_Command = S2_SETKEY;
	request->ios2_WireError = key_idx;
	request->ios2_PacketType = wpa_driver_sana2_cipher(alg);
	request->ios2_DataLength = key_len;
	request->ios2_Data = (u8 *) key;
	request->ios2_StatData = (u8 *) seq;

	/* Work-around for a bug in MLME code */
	if(wpa_driver_sana2_cipher(alg) == S2ENC_WEP)
		using_wep = 1;

	err = DoIO((APTR)request);

	return err;
}


static int wpa_driver_sana2_associate(
	void *priv, struct wpa_driver_associate_params *params)
{
	struct wpa_driver_sana2_data *drv = priv;
	int err = 0, i, alg;
	struct IOSana2Req *request = drv->request;
	struct TagItem *tag_list2 = NULL;
	char *ssid = os_zalloc(MAX_SSID_LEN + 1);
	struct TagItem tag_list[] =
		{{S2INFO_SSID, (UPINT)ssid},
		{(params->bssid != NULL) ? S2INFO_BSSID : TAG_IGNORE,
			(UPINT)params->bssid},
		{S2INFO_Encryption,
			wpa_driver_sana2_cipher(params->group_suite)},
		{S2INFO_PortType, (params->mode == IEEE80211_MODE_IBSS) ?
			S2PORT_ADHOC : S2PORT_MANAGED},
		{S2INFO_Channel, params->freq == 2484 ?
			2484 : (params->freq - 2407) / 5},
		{S2INFO_WPAInfo, (params->wpa_ie_len > 0) ?
			(UPINT)params->wpa_ie : (UPINT)NULL},
		{S2INFO_AuthTypes, params->auth_alg},
		{TAG_END, 0}};

	if (ssid == NULL) {
		err = 1;
	}

	if (err == 0) {
		CopyMem(params->ssid, ssid, params->ssid_len);
		ssid[params->ssid_len] = '\0';

		tag_list2 = CloneTagItems(tag_list);
		if (tag_list2 == NULL) {
			err = 1;
		}
	}

	/* Set or clear WEP keys */
	for (i = 0; i < 4; i++) {
		if (params->wep_key_len[i] > 0)
			alg = WPA_ALG_WEP;
		else
			alg = WPA_ALG_NONE;
		wpa_driver_sana2_set_key(NULL, priv, alg, broadcast_addr, i,
			i == params->wep_tx_keyidx, NULL, 0,
			params->wep_key[i], params->wep_key_len[i]);
	}

	/* Set other options */
	if (err == 0) {
		request->ios2_Req.io_Command = S2_SETOPTIONS;
		request->ios2_Data = tag_list2;
		err = DoIO((APTR)request);
		FreeTagItems(tag_list2);
	}

	/* Store current SSID and BSSID */

	if (err == 0) {
		strcpy(drv->ssid, ssid);
	}

	return 0;
}


static int wpa_driver_sana2_get_capa(void *priv, struct wpa_driver_capa *capa)
{
	struct wpa_driver_sana2_data *drv = priv;

	os_memset(capa, 0, sizeof(*capa));

	capa->key_mgmt = WPA_DRIVER_CAPA_KEY_MGMT_WPA |
		WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
		WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK |
		WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK;
	capa->enc = WPA_DRIVER_CAPA_ENC_WEP40 | WPA_DRIVER_CAPA_ENC_WEP104 |
		WPA_DRIVER_CAPA_ENC_TKIP | WPA_DRIVER_CAPA_ENC_CCMP;
	if(!drv->hard_mac)
		capa->flags = WPA_DRIVER_FLAGS_USER_SPACE_MLME;

	return 0;
}


static int wpa_driver_sana2_send_eapol(void *priv, const u8 *dest, u16 proto,
			  const u8 *data, size_t data_len)
{
	struct wpa_driver_sana2_data *drv = priv;
	struct IOSana2Req *request = drv->request;

	request->ios2_Req.io_Command = CMD_WRITE;
	request->ios2_Req.io_Flags = 0;
	CopyMem(dest, request->ios2_DstAddr, ETH_ALEN);
	request->ios2_PacketType = proto;
	request->ios2_DataLength = data_len;
	request->ios2_Data = (APTR)data;

	return DoIO((APTR)request);
}


static void wpa_driver_sana2_stay_alive(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_driver_sana2_data *drv = timeout_ctx;

	wpa_driver_sana2_send_eapol(drv, drv->bssid, 0, NULL, 0);
	eloop_register_timeout(NULL_INTERVAL, 0, wpa_driver_sana2_stay_alive,
			       drv->ctx, drv);
}


static const u8 *wpa_driver_sana2_get_mac_addr(void *priv)
{
	struct wpa_driver_sana2_data *drv = priv;

	return drv->addr;
}


static struct hostapd_hw_modes * wpa_driver_sana2_get_hw_feature_data(
	void *priv, u16 *num_modes, u16 *flags)
{
	int err = 0, i, mode_count, channel_count, rate_count;
	struct hostapd_hw_modes *modes;

	/* IEEE 802.11b */
	mode_count = 2;
	channel_count = 14;
	rate_count = 4;

	modes = os_zalloc(sizeof(struct hostapd_hw_modes) * mode_count);
	if (modes == NULL) {
		err = 1;
	}

	if (err == 0) {
		modes[0].mode = HOSTAPD_MODE_IEEE80211B;

		modes[0].num_channels = channel_count;
		modes[0].channels = os_zalloc(
			channel_count * sizeof(struct hostapd_channel_data));
		if (modes[0].channels == NULL)
			err = 1;
	}

	if (err == 0) {
		for (i = 0; i < 13; i++) {
			modes[0].channels[i].chan = i + 1;
			modes[0].channels[i].freq = 2407 + (i + 1) * 5;
			modes[0].channels[i].flag = 0;
			modes[0].channels[i].max_tx_power = MAX_TX_POWER;
		}
		modes[0].channels[i].chan = 14;
		modes[0].channels[i].freq = 2484;
		modes[0].channels[i].flag = 0;
		modes[0].channels[i].max_tx_power = MAX_TX_POWER;
	}

	if (err == 0) {
		modes[0].num_rates = rate_count;
		modes[0].rates = os_zalloc(rate_count * sizeof(int));
		if (modes[0].rates == NULL)
			err = 1;
	}

	if (err == 0) {
		os_memcpy(modes[0].rates, b_rates, sizeof(b_rates));
	}

	/* IEEE 802.11g */
	channel_count = 13;
	rate_count = 12;

	if (err == 0) {
		modes[1].mode = HOSTAPD_MODE_IEEE80211G;

		modes[1].num_channels = channel_count;
		modes[1].channels = os_zalloc(
			channel_count * sizeof(struct hostapd_channel_data));
		if (modes[1].channels == NULL)
			err = 1;
	}

	if (err == 0) {
		for (i = 0; i < 13; i++) {
			modes[1].channels[i].chan = i + 1;
			modes[1].channels[i].freq = 2407 + (i + 1) * 5;
			modes[1].channels[i].flag = 0;
			modes[1].channels[i].max_tx_power = MAX_TX_POWER;
		}
	}

	if (err == 0) {
		modes[1].num_rates = rate_count;
		modes[1].rates = os_zalloc(rate_count * sizeof(int));
		if (modes[1].rates == NULL)
			err = 1;
	}

	if (err == 0) {
		os_memcpy(modes[1].rates, g_rates, sizeof(g_rates));
	}

	if (err != 0) {
		os_free(modes[0].channels);
		os_free(modes[0].rates);
		os_free(modes[1].channels);
		os_free(modes[1].rates);
		modes = NULL;
	}

	*num_modes = mode_count;
	*flags = 0;
	return modes;
}


static int wpa_driver_sana2_set_channel(void *priv,
	enum hostapd_hw_mode phymode, int chan, int freq)
{
	struct wpa_driver_sana2_data *drv = priv;
	struct IOSana2Req *request = drv->request;
	struct TagItem tag_list[] =
		{{S2INFO_Band, wpa_driver_sana2_band(phymode)},
		{S2INFO_Channel, chan},
		{TAG_END, 0}};

	drv->freq = freq;
	request->ios2_Req.io_Command = S2_SETOPTIONS;
	request->ios2_Data = tag_list;
	return DoIO((APTR)request);
}


static int wpa_driver_sana2_set_ssid(void *priv, const u8 *ssid,
	size_t ssid_len)
{
	struct wpa_driver_sana2_data *drv = priv;
	struct IOSana2Req *request = drv->request;
	TEXT ssid_str[ssid_len + 1];
	struct TagItem tag_list[] =
		{{S2INFO_SSID, (UPINT)ssid_str},
		{TAG_END, 0}};

	CopyMem(ssid, ssid_str, ssid_len);
	ssid_str[ssid_len] = '\0';

	request->ios2_Req.io_Command = S2_SETOPTIONS;
	request->ios2_Data = tag_list;
	return DoIO((APTR)request);
}


static int wpa_driver_sana2_set_bssid(void *priv, const u8 *bssid)
{
	struct wpa_driver_sana2_data *drv = priv;
	struct IOSana2Req *request = drv->request;
	struct TagItem tag_list[] =
		{{S2INFO_BSSID, (UPINT)bssid},
		{TAG_END, 0}};

	CopyMem(bssid, drv->bssid, ETH_ALEN);

	request->ios2_Req.io_Command = S2_SETOPTIONS;
	request->ios2_Data = tag_list;
	return DoIO((APTR)request);
}


static int wpa_driver_sana2_set_associd(void *priv, u16 aid)
{
	struct wpa_driver_sana2_data *drv = priv;
	struct IOSana2Req *request = drv->request;
	struct TagItem tag_list[] =
		{{S2INFO_AssocID, aid},
		{TAG_END, 0}};

	eloop_register_timeout(NULL_INTERVAL, 0, wpa_driver_sana2_stay_alive,
			       drv->ctx, drv);

	request->ios2_Req.io_Command = S2_SETOPTIONS;
	request->ios2_Data = tag_list;
	return DoIO((APTR)request);
}


static int wpa_driver_sana2_set_capabilities(void *priv, u16 capab)
{
	struct wpa_driver_sana2_data *drv = priv;
	struct IOSana2Req *request = drv->request;
	struct TagItem tag_list[] =
		{{S2INFO_Capabilities, capab},
		{TAG_END, 0}};

wpa_printf(MSG_DEBUG, "sana2: setting negotiated capabilities");
	request->ios2_Req.io_Command = S2_SETOPTIONS;
	request->ios2_Data = tag_list;
	return DoIO((APTR)request);
}


static int wpa_driver_sana2_send_mlme(void *priv, const u8 *data,
	size_t data_len)
{
	struct wpa_driver_sana2_data *drv = priv;
	struct IOSana2Req *request = drv->request;

wpa_printf(MSG_DEBUG, "sana2: sending MLME frame");
	request->ios2_Req.io_Command = S2_WRITEMGMT;
	request->ios2_Req.io_Flags = 0;
	request->ios2_DataLength = data_len;
	request->ios2_Data = (APTR)data;

	return DoIO((APTR)request);
}


static int wpa_driver_sana2_mlme_add_sta(void *priv, const u8 *addr,
	const u8 *supp_rates, size_t supp_rates_len)
{
	return 0;
}


static int wpa_driver_sana2_mlme_remove_sta(void *priv, const u8 *addr)
{
	return 0;
}


static void wpa_driver_sana2_event_handler(int sig, void *sig_ctx)
{
	struct wpa_driver_sana2_data *drv = sig_ctx;
	int i;
	ULONG events;
	struct IOSana2Req *request;
	union wpa_event_data event_data;
	struct TagItem *tag_list;
	u8 *ie;
	APTR pool;

	os_memset(&event_data, 0, sizeof(event_data));

	/* Propagate events */
	while((request = (APTR)GetMsg(drv->port)) != NULL)
	{
		switch(request->ios2_Req.io_Command)
		{
		case CMD_READ:
			if(request->ios2_Req.io_Error == 0) {
				/* Deliver frame to supplicant */
				drv_event_eapol_rx(drv->ctx,
					request->ios2_SrcAddr,
					request->ios2_Data,
					request->ios2_DataLength);
			}
			if(request->ios2_Req.io_Error != S2ERR_OUTOFSERVICE) {
				/* Send request back for next frame */
				SendIO((APTR)request);
			}
			break;

		case S2_READMGMT:
			if(request->ios2_Req.io_Error == 0) {
				/* Deliver frame to supplicant */
				os_memset(&event_data, 0, sizeof(event_data));
				event_data.mlme_rx.buf = request->ios2_Data;
				event_data.mlme_rx.len =
					request->ios2_DataLength;
				event_data.mlme_rx.freq = drv->freq;
				wpa_supplicant_event(drv->ctx, EVENT_MLME_RX,
					&event_data);
			}
			if(request->ios2_Req.io_Error != S2ERR_OUTOFSERVICE) {
				/* Send request back for next frame */
				request->ios2_DataLength = ETH_MTU;
				SendIO((APTR)request);
			}
			break;

		case S2_ONEVENT:
			events = request->ios2_WireError;
			if (request->ios2_Req.io_Error != 0)
				events = 0;
			if (events & S2EVENT_CONNECT)
			{
				request->ios2_Req.io_Command =
					S2_GETNETWORKINFO;
				pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
					PUDDLE_SIZE, PUDDLE_SIZE);
				request->ios2_Data = pool;
				if (pool != NULL && DoIO((APTR)request) == 0)
				{
					tag_list = request->ios2_StatData;
					event_data.assoc_info.req_ies = ie =
						(u8 *)GetTagData(S2INFO_WPAInfo,
							(UPINT)NULL,
							tag_list);
				} else
					ie = NULL;
				if (ie != NULL)
					event_data.assoc_info.req_ies_len =
						ie[1] + 2;
				else
					event_data.assoc_info.req_ies_len = 0;

				wpa_supplicant_event(drv->ctx, EVENT_ASSOC,
					&event_data);

				DeletePool(pool);

				/* Start periodic sending of null-data frames */
				eloop_register_timeout(NULL_INTERVAL, 0,
					wpa_driver_sana2_stay_alive,
					drv->ctx, drv);
			}

			if (events & S2EVENT_DISCONNECT) {
				wpa_supplicant_event(drv->ctx, EVENT_DISASSOC,
					NULL);

				/* Stop periodic sending of null-data frames */
				eloop_cancel_timeout(
					wpa_driver_sana2_stay_alive,
					drv->ctx, drv);
			}

			if (events & S2EVENT_ONLINE) {
				drv->event_mask &= ~S2EVENT_ONLINE;
				drv->event_mask |= S2EVENT_OFFLINE;

				/* Send read requests back for next frame */
				SendIO((APTR)drv->eapol_request);
				if(!drv->hard_mac)
					for(i = 0; i < MLME_REQ_COUNT; i++)
						SendIO((APTR)
							drv->mlme_requests[i]);
			}

			if (events & S2EVENT_OFFLINE) {
				drv->event_mask &= ~S2EVENT_OFFLINE;
				drv->event_mask |= S2EVENT_ONLINE;
			}

			/* Send request back to receive next event */
			request->ios2_Req.io_Command = S2_ONEVENT;
			request->ios2_WireError = drv->event_mask;
			SendIO((APTR)request);
			break;

		case S2_GETNETWORKS:
			wpa_supplicant_event(drv->ctx, EVENT_SCAN_RESULTS,
				NULL);
			break;
		}
	}
}


static void wpa_driver_sana2_deinit(void *priv)
{
	struct wpa_driver_sana2_data *drv = priv;
	int i;

	/* Abort outstanding I/O requests */
	if(drv->eapol_request != NULL) {
		AbortIO((APTR)drv->eapol_request);
		WaitIO((APTR)drv->eapol_request);
		FreeVec(drv->eapol_request);
	}

	for(i = 0; i < MLME_REQ_COUNT; i++) {
		if(drv->mlme_requests[i] != NULL) {
			AbortIO((APTR)drv->mlme_requests[i]);
			WaitIO((APTR)drv->mlme_requests[i]);
			FreeVec(drv->mlme_requests[i]);
		}
	}

	if(drv->event_request != NULL) {
		AbortIO((APTR)drv->event_request);
		WaitIO((APTR)drv->event_request);
	}

	/* Close device */
	if(drv->device_opened) {
		AbortIO((APTR)drv->request);
		WaitIO((APTR)drv->request);
		CloseDevice((APTR)drv->request);
	}
	DeleteIORequest((APTR)drv->request);
	DeleteMsgPort(drv->port);

	FreeVec(drv->device_name);

	os_free(drv);
}


static void * wpa_driver_sana2_init(void *ctx, const char *ifname)
{
	struct wpa_driver_sana2_data *drv;
	int err = 0, i;
	char *end, *p;
	ULONG unit_no;
	struct IOSana2Req *request, *eapol_request, *mlme_request;
	struct NSDeviceQueryResult device_info =
		{0, sizeof(struct NSDeviceQueryResult)};

	drv = os_zalloc(sizeof(*drv));
	if (drv == NULL)
		return NULL;
	drv->ctx = ctx;
	drv->device_name = AllocVec(strlen(device_dir) + strlen(ifname),
		MEMF_PUBLIC);
	if(drv->device_name == NULL)
		err = 1;

	/* Split fake interface name into device and unit */
	if(err == 0) {
		end = strrchr(ifname, ':');
		if(strchr(ifname, ':') == end && strchr(ifname, '/') == NULL) {
			strcpy(drv->device_name, device_dir);
			p = drv->device_name + strlen(device_dir);
		} else
			p = drv->device_name;
		strncpy(p, ifname, end - ifname);
		p[end - ifname] = '\0';

		unit_no = atoi(end + 1);

		drv->port = CreateMsgPort();
		drv->request = (APTR)CreateIORequest(drv->port,
			sizeof(struct IOSana2Req));
		if(drv->request == NULL)
			err = 1;
	}

	if(err == 0) {
		/* Open device */
		drv->request->ios2_BufferManagement = (APTR)buffer_tags;
		if(OpenDevice((TEXT *)drv->device_name, unit_no,
			(APTR)drv->request, 0) != 0)
			err = 1;
	}

	/* Register a handler for the port's signal */
	if(err == 0) {
		drv->device_opened = 1;

		eloop_register_signal(drv->port->mp_SigBit,
			wpa_driver_sana2_event_handler, drv);
	}

	/* Check if this is a hard-MAC device */
	if(err == 0) {
		request = drv->request;
		((struct IOStdReq *)request)->io_Command = NSCMD_DEVICEQUERY;
		((struct IOStdReq *)request)->io_Data = &device_info;
		if(DoIO((APTR)request) != 0)
			err = 1;
		else {
			for(i = 0; device_info.SupportedCommands[i] != 0; i++)
				if(device_info.SupportedCommands[i]
					== S2_GETNETWORKS)
					drv->hard_mac = 1;
		}
	}

	/* Put device online */
	if(err == 0) {
		request = drv->request;
		request->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
		if(DoIO((APTR)request) != 0)
			err = 1;
	}

	if(err == 0) {
		CopyMem(request->ios2_DstAddr, drv->addr, ETH_ALEN);

		request->ios2_Req.io_Command = S2_CONFIGINTERFACE;
		CopyMem(drv->addr, request->ios2_SrcAddr, ETH_ALEN);

		if(DoIO((APTR)request) != 0) {
			if(request->ios2_WireError == S2WERR_IS_CONFIGURED) {
				request->ios2_Req.io_Command = S2_ONLINE;
				DoIO((APTR)request);
			} else
				err = 1;
		}
		drv->online = 1;
	}

	/* Create and queue up EAPOL frame read request */
	if(err == 0) {
		request = drv->request;
		request->ios2_Req.io_Command = CMD_READ;
		request->ios2_PacketType = ETH_P_EAPOL;

		eapol_request = AllocVec(sizeof(struct IOSana2Req) + ETH_MTU,
			MEMF_PUBLIC | MEMF_CLEAR);
		if(eapol_request == NULL)
			err = 1;
	}

	if(err == 0) {
		CopyMem(drv->request, eapol_request,
			sizeof(struct IOSana2Req));
		eapol_request->ios2_Data = eapol_request + 1;
		drv->eapol_request = eapol_request;

		SendIO((APTR)eapol_request);
	}

	/* Create and queue up management frame read requests */
	if(err == 0 && !drv->hard_mac) {
		request = drv->request;
		request->ios2_Req.io_Command = S2_READMGMT;
		request->ios2_DataLength = ETH_MTU;
		for(i = 0; i < MLME_REQ_COUNT; i++) {
			mlme_request =
				AllocVec(sizeof(struct IOSana2Req) + ETH_MTU,
					MEMF_PUBLIC | MEMF_CLEAR);
			if(mlme_request == NULL)
				err = 1;

			if(err == 0) {
				CopyMem(drv->request, mlme_request,
					sizeof(struct IOSana2Req));
				mlme_request->ios2_Data = mlme_request + 1;
				drv->mlme_requests[i] = mlme_request;

				SendIO((APTR)mlme_request);
			}
		}
	}

	if(err == 0) {
		/* Create and send event-notification request */
		request = drv->request;
		request->ios2_Req.io_Command = S2_ONEVENT;
		if(drv->hard_mac)
			drv->event_mask = S2EVENT_CONNECT | S2EVENT_DISCONNECT
				| S2EVENT_OFFLINE;
		else
			drv->event_mask = S2EVENT_OFFLINE;
		request->ios2_WireError = drv->event_mask;

		drv->event_request = AllocVec(sizeof(struct IOSana2Req),
			MEMF_PUBLIC | MEMF_CLEAR);
		if(drv->event_request == NULL)
			err = 1;
	}

	if(err == 0) {
		CopyMem(drv->request, drv->event_request,
			sizeof(struct IOSana2Req));

		SendIO((APTR)drv->event_request);
	}

	if(err != 0) {
		wpa_driver_sana2_deinit(drv);
		drv = NULL;
	}

	return drv;
}


const struct wpa_driver_ops wpa_driver_sana2_ops = {
	.name = "sana2",
	.desc = "SANA-II driver",
	.get_bssid = wpa_driver_sana2_get_bssid,
	.get_ssid = wpa_driver_sana2_get_ssid,
	.set_key = wpa_driver_sana2_set_key,
	.init = wpa_driver_sana2_init,
	.deinit = wpa_driver_sana2_deinit,
	.scan2 = wpa_driver_sana2_scan,
	.associate = wpa_driver_sana2_associate,
	.get_capa = wpa_driver_sana2_get_capa,
	.get_mac_addr = wpa_driver_sana2_get_mac_addr,
	.send_eapol = wpa_driver_sana2_send_eapol,
	.get_scan_results2 = wpa_driver_sana2_get_scan_results,
	.get_hw_feature_data = wpa_driver_sana2_get_hw_feature_data,
	.set_channel = wpa_driver_sana2_set_channel,
	.set_ssid = wpa_driver_sana2_set_ssid,
	.set_bssid = wpa_driver_sana2_set_bssid,
	.set_associd = wpa_driver_sana2_set_associd,
	.set_capabilities = wpa_driver_sana2_set_capabilities,
	.send_mlme = wpa_driver_sana2_send_mlme,
	.mlme_add_sta = wpa_driver_sana2_mlme_add_sta,
	.mlme_remove_sta = wpa_driver_sana2_mlme_remove_sta,
};
