/*
 * WPA Supplicant / Control interface (shared code for all backends)
 * Copyright (c) 2004-2010, Jouni Malinen <j@w1.fi>
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

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"
#include "common/ieee802_11_defs.h"
#include "common/wpa_ctrl.h"
#include "eap_peer/eap.h"
#include "eapol_supp/eapol_supp_sm.h"
#include "rsn_supp/wpa.h"
#include "rsn_supp/preauth.h"
#include "rsn_supp/pmksa_cache.h"
#include "l2_packet/l2_packet.h"
#include "wps/wps.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "wps_supplicant.h"
#include "ibss_rsn.h"
#include "ap.h"
#include "notify.h"
#include "bss.h"
#include "scan.h"
#include "ctrl_iface.h"

extern struct wpa_driver_ops *wpa_drivers[];

static int wpa_supplicant_global_iface_list(struct wpa_global *global,
					    char *buf, int len);
static int wpa_supplicant_global_iface_interfaces(struct wpa_global *global,
						  char *buf, int len);


static int wpa_supplicant_ctrl_iface_set(struct wpa_supplicant *wpa_s,
					 char *cmd)
{
	char *value;
	int ret = 0;

	value = os_strchr(cmd, ' ');
	if (value == NULL)
		return -1;
	*value++ = '\0';

	wpa_printf(MSG_DEBUG, "CTRL_IFACE SET '%s'='%s'", cmd, value);
	if (os_strcasecmp(cmd, "EAPOL::heldPeriod") == 0) {
		eapol_sm_configure(wpa_s->eapol,
				   atoi(value), -1, -1, -1);
	} else if (os_strcasecmp(cmd, "EAPOL::authPeriod") == 0) {
		eapol_sm_configure(wpa_s->eapol,
				   -1, atoi(value), -1, -1);
	} else if (os_strcasecmp(cmd, "EAPOL::startPeriod") == 0) {
		eapol_sm_configure(wpa_s->eapol,
				   -1, -1, atoi(value), -1);
	} else if (os_strcasecmp(cmd, "EAPOL::maxStart") == 0) {
		eapol_sm_configure(wpa_s->eapol,
				   -1, -1, -1, atoi(value));
	} else if (os_strcasecmp(cmd, "dot11RSNAConfigPMKLifetime") == 0) {
		if (wpa_sm_set_param(wpa_s->wpa, RSNA_PMK_LIFETIME,
				     atoi(value)))
			ret = -1;
	} else if (os_strcasecmp(cmd, "dot11RSNAConfigPMKReauthThreshold") ==
		   0) {
		if (wpa_sm_set_param(wpa_s->wpa, RSNA_PMK_REAUTH_THRESHOLD,
				     atoi(value)))
			ret = -1;
	} else if (os_strcasecmp(cmd, "dot11RSNAConfigSATimeout") == 0) {
		if (wpa_sm_set_param(wpa_s->wpa, RSNA_SA_TIMEOUT, atoi(value)))
			ret = -1;
	} else
		ret = -1;

	return ret;
}


#ifdef IEEE8021X_EAPOL
static int wpa_supplicant_ctrl_iface_preauth(struct wpa_supplicant *wpa_s,
					     char *addr)
{
	u8 bssid[ETH_ALEN];
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	if (hwaddr_aton(addr, bssid)) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE PREAUTH: invalid address "
			   "'%s'", addr);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "CTRL_IFACE PREAUTH " MACSTR, MAC2STR(bssid));
	rsn_preauth_deinit(wpa_s->wpa);
	if (rsn_preauth_init(wpa_s->wpa, bssid, ssid ? &ssid->eap : NULL))
		return -1;

	return 0;
}
#endif /* IEEE8021X_EAPOL */


#ifdef CONFIG_PEERKEY
/* MLME-STKSTART.request(peer) */
static int wpa_supplicant_ctrl_iface_stkstart(
	struct wpa_supplicant *wpa_s, char *addr)
{
	u8 peer[ETH_ALEN];

	if (hwaddr_aton(addr, peer)) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE STKSTART: invalid "
			   "address '%s'", addr);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "CTRL_IFACE STKSTART " MACSTR,
		   MAC2STR(peer));

	return wpa_sm_stkstart(wpa_s->wpa, peer);
}
#endif /* CONFIG_PEERKEY */


#ifdef CONFIG_IEEE80211R
static int wpa_supplicant_ctrl_iface_ft_ds(
	struct wpa_supplicant *wpa_s, char *addr)
{
	u8 target_ap[ETH_ALEN];
	struct wpa_bss *bss;
	const u8 *mdie;

	if (hwaddr_aton(addr, target_ap)) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE FT_DS: invalid "
			   "address '%s'", addr);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "CTRL_IFACE FT_DS " MACSTR, MAC2STR(target_ap));

	bss = wpa_bss_get_bssid(wpa_s, target_ap);
	if (bss)
		mdie = wpa_bss_get_ie(bss, WLAN_EID_MOBILITY_DOMAIN);
	else
		mdie = NULL;

	return wpa_ft_start_over_ds(wpa_s->wpa, target_ap, mdie);
}
#endif /* CONFIG_IEEE80211R */


#ifdef CONFIG_WPS
static int wpa_supplicant_ctrl_iface_wps_pbc(struct wpa_supplicant *wpa_s,
					     char *cmd)
{
	u8 bssid[ETH_ALEN], *_bssid = bssid;

	if (cmd == NULL || os_strcmp(cmd, "any") == 0)
		_bssid = NULL;
	else if (hwaddr_aton(cmd, bssid)) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE WPS_PBC: invalid BSSID '%s'",
			   cmd);
		return -1;
	}

#ifdef CONFIG_AP
	if (wpa_s->ap_iface)
		return wpa_supplicant_ap_wps_pbc(wpa_s, _bssid);
#endif /* CONFIG_AP */

	return wpas_wps_start_pbc(wpa_s, _bssid);
}


static int wpa_supplicant_ctrl_iface_wps_pin(struct wpa_supplicant *wpa_s,
					     char *cmd, char *buf,
					     size_t buflen)
{
	u8 bssid[ETH_ALEN], *_bssid = bssid;
	char *pin;
	int ret;

	pin = os_strchr(cmd, ' ');
	if (pin)
		*pin++ = '\0';

	if (os_strcmp(cmd, "any") == 0)
		_bssid = NULL;
	else if (hwaddr_aton(cmd, bssid)) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE WPS_PIN: invalid BSSID '%s'",
			   cmd);
		return -1;
	}

#ifdef CONFIG_AP
	if (wpa_s->ap_iface)
		return wpa_supplicant_ap_wps_pin(wpa_s, _bssid, pin,
						 buf, buflen);
#endif /* CONFIG_AP */

	if (pin) {
		ret = wpas_wps_start_pin(wpa_s, _bssid, pin);
		if (ret < 0)
			return -1;
		ret = os_snprintf(buf, buflen, "%s", pin);
		if (ret < 0 || (size_t) ret >= buflen)
			return -1;
		return ret;
	}

	ret = wpas_wps_start_pin(wpa_s, _bssid, NULL);
	if (ret < 0)
		return -1;

	/* Return the generated PIN */
	ret = os_snprintf(buf, buflen, "%08d", ret);
	if (ret < 0 || (size_t) ret >= buflen)
		return -1;
	return ret;
}


#ifdef CONFIG_WPS_OOB
static int wpa_supplicant_ctrl_iface_wps_oob(struct wpa_supplicant *wpa_s,
					     char *cmd)
{
	char *path, *method, *name;

	path = os_strchr(cmd, ' ');
	if (path == NULL)
		return -1;
	*path++ = '\0';

	method = os_strchr(path, ' ');
	if (method == NULL)
		return -1;
	*method++ = '\0';

	name = os_strchr(method, ' ');
	if (name != NULL)
		*name++ = '\0';

	return wpas_wps_start_oob(wpa_s, cmd, path, method, name);
}
#endif /* CONFIG_WPS_OOB */


static int wpa_supplicant_ctrl_iface_wps_reg(struct wpa_supplicant *wpa_s,
					     char *cmd)
{
	u8 bssid[ETH_ALEN], *_bssid = bssid;
	char *pin;
	char *new_ssid;
	char *new_auth;
	char *new_encr;
	char *new_key;
	struct wps_new_ap_settings ap;

	pin = os_strchr(cmd, ' ');
	if (pin == NULL)
		return -1;
	*pin++ = '\0';

	if (os_strcmp(cmd, "any") == 0)
		_bssid = NULL;
	else if (hwaddr_aton(cmd, bssid)) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE WPS_REG: invalid BSSID '%s'",
			   cmd);
		return -1;
	}

	new_ssid = os_strchr(pin, ' ');
	if (new_ssid == NULL)
		return wpas_wps_start_reg(wpa_s, _bssid, pin, NULL);
	*new_ssid++ = '\0';

	new_auth = os_strchr(new_ssid, ' ');
	if (new_auth == NULL)
		return -1;
	*new_auth++ = '\0';

	new_encr = os_strchr(new_auth, ' ');
	if (new_encr == NULL)
		return -1;
	*new_encr++ = '\0';

	new_key = os_strchr(new_encr, ' ');
	if (new_key == NULL)
		return -1;
	*new_key++ = '\0';

	os_memset(&ap, 0, sizeof(ap));
	ap.ssid_hex = new_ssid;
	ap.auth = new_auth;
	ap.encr = new_encr;
	ap.key_hex = new_key;
	return wpas_wps_start_reg(wpa_s, _bssid, pin, &ap);
}


#ifdef CONFIG_WPS_ER
static int wpa_supplicant_ctrl_iface_wps_er_pin(struct wpa_supplicant *wpa_s,
						char *cmd)
{
	char *uuid = cmd, *pin;
	pin = os_strchr(uuid, ' ');
	if (pin == NULL)
		return -1;
	*pin++ = '\0';
	return wpas_wps_er_add_pin(wpa_s, uuid, pin);
}


static int wpa_supplicant_ctrl_iface_wps_er_learn(struct wpa_supplicant *wpa_s,
						  char *cmd)
{
	char *uuid = cmd, *pin;
	pin = os_strchr(uuid, ' ');
	if (pin == NULL)
		return -1;
	*pin++ = '\0';
	return wpas_wps_er_learn(wpa_s, uuid, pin);
}
#endif /* CONFIG_WPS_ER */

#endif /* CONFIG_WPS */


#ifdef CONFIG_IBSS_RSN
static int wpa_supplicant_ctrl_iface_ibss_rsn(
	struct wpa_supplicant *wpa_s, char *addr)
{
	u8 peer[ETH_ALEN];

	if (hwaddr_aton(addr, peer)) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE IBSS_RSN: invalid "
			   "address '%s'", addr);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "CTRL_IFACE IBSS_RSN " MACSTR,
		   MAC2STR(peer));

	return ibss_rsn_start(wpa_s->ibss_rsn, peer);
}
#endif /* CONFIG_IBSS_RSN */


static int wpa_supplicant_ctrl_iface_ctrl_rsp(struct wpa_supplicant *wpa_s,
					      char *rsp)
{
#ifdef IEEE8021X_EAPOL
	char *pos, *id_pos;
	int id;
	struct wpa_ssid *ssid;
	struct eap_peer_config *eap;

	pos = os_strchr(rsp, '-');
	if (pos == NULL)
		return -1;
	*pos++ = '\0';
	id_pos = pos;
	pos = os_strchr(pos, ':');
	if (pos == NULL)
		return -1;
	*pos++ = '\0';
	id = atoi(id_pos);
	wpa_printf(MSG_DEBUG, "CTRL_IFACE: field=%s id=%d", rsp, id);
	wpa_hexdump_ascii_key(MSG_DEBUG, "CTRL_IFACE: value",
			      (u8 *) pos, os_strlen(pos));

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: Could not find SSID id=%d "
			   "to update", id);
		return -1;
	}
	eap = &ssid->eap;

	if (os_strcmp(rsp, "IDENTITY") == 0) {
		os_free(eap->identity);
		eap->identity = (u8 *) os_strdup(pos);
		eap->identity_len = os_strlen(pos);
		eap->pending_req_identity = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
	} else if (os_strcmp(rsp, "PASSWORD") == 0) {
		os_free(eap->password);
		eap->password = (u8 *) os_strdup(pos);
		eap->password_len = os_strlen(pos);
		eap->pending_req_password = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
	} else if (os_strcmp(rsp, "NEW_PASSWORD") == 0) {
		os_free(eap->new_password);
		eap->new_password = (u8 *) os_strdup(pos);
		eap->new_password_len = os_strlen(pos);
		eap->pending_req_new_password = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
	} else if (os_strcmp(rsp, "PIN") == 0) {
		os_free(eap->pin);
		eap->pin = os_strdup(pos);
		eap->pending_req_pin = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
	} else if (os_strcmp(rsp, "OTP") == 0) {
		os_free(eap->otp);
		eap->otp = (u8 *) os_strdup(pos);
		eap->otp_len = os_strlen(pos);
		os_free(eap->pending_req_otp);
		eap->pending_req_otp = NULL;
		eap->pending_req_otp_len = 0;
	} else if (os_strcmp(rsp, "PASSPHRASE") == 0) {
		os_free(eap->private_key_passwd);
		eap->private_key_passwd = (u8 *) os_strdup(pos);
		eap->pending_req_passphrase = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
	} else {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: Unknown field '%s'", rsp);
		return -1;
	}

	return 0;
#else /* IEEE8021X_EAPOL */
	wpa_printf(MSG_DEBUG, "CTRL_IFACE: 802.1X not included");
	return -1;
#endif /* IEEE8021X_EAPOL */
}


static int wpa_supplicant_ctrl_iface_status(struct wpa_supplicant *wpa_s,
					    const char *params,
					    char *buf, size_t buflen)
{
	char *pos, *end, tmp[30];
	int res, verbose, ret;

	verbose = os_strcmp(params, "-VERBOSE") == 0;
	pos = buf;
	end = buf + buflen;
	if (wpa_s->wpa_state >= WPA_ASSOCIATED) {
		struct wpa_ssid *ssid = wpa_s->current_ssid;
		ret = os_snprintf(pos, end - pos, "bssid=" MACSTR "\n",
				  MAC2STR(wpa_s->bssid));
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		if (ssid) {
			u8 *_ssid = ssid->ssid;
			size_t ssid_len = ssid->ssid_len;
			u8 ssid_buf[MAX_SSID_LEN];
			if (ssid_len == 0) {
				int _res = wpa_drv_get_ssid(wpa_s, ssid_buf);
				if (_res < 0)
					ssid_len = 0;
				else
					ssid_len = _res;
				_ssid = ssid_buf;
			}
			ret = os_snprintf(pos, end - pos, "ssid=%s\nid=%d\n",
					  wpa_ssid_txt(_ssid, ssid_len),
					  ssid->id);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;

			if (ssid->id_str) {
				ret = os_snprintf(pos, end - pos,
						  "id_str=%s\n",
						  ssid->id_str);
				if (ret < 0 || ret >= end - pos)
					return pos - buf;
				pos += ret;
			}

			switch (ssid->mode) {
			case WPAS_MODE_INFRA:
				ret = os_snprintf(pos, end - pos,
						  "mode=station\n");
				break;
			case WPAS_MODE_IBSS:
				ret = os_snprintf(pos, end - pos,
						  "mode=IBSS\n");
				break;
			case WPAS_MODE_AP:
				ret = os_snprintf(pos, end - pos,
						  "mode=AP\n");
				break;
			default:
				ret = 0;
				break;
			}
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}

#ifdef CONFIG_AP
		if (wpa_s->ap_iface) {
			pos += ap_ctrl_iface_wpa_get_status(wpa_s, pos,
							    end - pos,
							    verbose);
		} else
#endif /* CONFIG_AP */
		pos += wpa_sm_get_status(wpa_s->wpa, pos, end - pos, verbose);
	}
	ret = os_snprintf(pos, end - pos, "wpa_state=%s\n",
			  wpa_supplicant_state_txt(wpa_s->wpa_state));
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	if (wpa_s->l2 &&
	    l2_packet_get_ip_addr(wpa_s->l2, tmp, sizeof(tmp)) >= 0) {
		ret = os_snprintf(pos, end - pos, "ip_address=%s\n", tmp);
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}

	if (wpa_key_mgmt_wpa_ieee8021x(wpa_s->key_mgmt) ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		res = eapol_sm_get_status(wpa_s->eapol, pos, end - pos,
					  verbose);
		if (res >= 0)
			pos += res;
	}

	res = rsn_preauth_get_status(wpa_s->wpa, pos, end - pos, verbose);
	if (res >= 0)
		pos += res;

	return pos - buf;
}


static int wpa_supplicant_ctrl_iface_bssid(struct wpa_supplicant *wpa_s,
					   char *cmd)
{
	char *pos;
	int id;
	struct wpa_ssid *ssid;
	u8 bssid[ETH_ALEN];

	/* cmd: "<network id> <BSSID>" */
	pos = os_strchr(cmd, ' ');
	if (pos == NULL)
		return -1;
	*pos++ = '\0';
	id = atoi(cmd);
	wpa_printf(MSG_DEBUG, "CTRL_IFACE: id=%d bssid='%s'", id, pos);
	if (hwaddr_aton(pos, bssid)) {
		wpa_printf(MSG_DEBUG ,"CTRL_IFACE: invalid BSSID '%s'", pos);
		return -1;
	}

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: Could not find SSID id=%d "
			   "to update", id);
		return -1;
	}

	os_memcpy(ssid->bssid, bssid, ETH_ALEN);
	ssid->bssid_set = !is_zero_ether_addr(bssid);

	return 0;
}


static int wpa_supplicant_ctrl_iface_list_networks(
	struct wpa_supplicant *wpa_s, char *buf, size_t buflen)
{
	char *pos, *end;
	struct wpa_ssid *ssid;
	int ret;

	pos = buf;
	end = buf + buflen;
	ret = os_snprintf(pos, end - pos,
			  "network id / ssid / bssid / flags\n");
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	ssid = wpa_s->conf->ssid;
	while (ssid) {
		ret = os_snprintf(pos, end - pos, "%d\t%s",
				  ssid->id,
				  wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		if (ssid->bssid_set) {
			ret = os_snprintf(pos, end - pos, "\t" MACSTR,
					  MAC2STR(ssid->bssid));
		} else {
			ret = os_snprintf(pos, end - pos, "\tany");
		}
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		ret = os_snprintf(pos, end - pos, "\t%s%s",
				  ssid == wpa_s->current_ssid ?
				  "[CURRENT]" : "",
				  ssid->disabled ? "[DISABLED]" : "");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		ret = os_snprintf(pos, end - pos, "\n");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;

		ssid = ssid->next;
	}

	return pos - buf;
}


static char * wpa_supplicant_cipher_txt(char *pos, char *end, int cipher)
{
	int first = 1, ret;
	ret = os_snprintf(pos, end - pos, "-");
	if (ret < 0 || ret >= end - pos)
		return pos;
	pos += ret;
	if (cipher & WPA_CIPHER_NONE) {
		ret = os_snprintf(pos, end - pos, "%sNONE", first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	if (cipher & WPA_CIPHER_WEP40) {
		ret = os_snprintf(pos, end - pos, "%sWEP40", first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	if (cipher & WPA_CIPHER_WEP104) {
		ret = os_snprintf(pos, end - pos, "%sWEP104",
				  first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	if (cipher & WPA_CIPHER_TKIP) {
		ret = os_snprintf(pos, end - pos, "%sTKIP", first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	if (cipher & WPA_CIPHER_CCMP) {
		ret = os_snprintf(pos, end - pos, "%sCCMP", first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	return pos;
}


static char * wpa_supplicant_ie_txt(char *pos, char *end, const char *proto,
				    const u8 *ie, size_t ie_len)
{
	struct wpa_ie_data data;
	int first, ret;

	ret = os_snprintf(pos, end - pos, "[%s-", proto);
	if (ret < 0 || ret >= end - pos)
		return pos;
	pos += ret;

	if (wpa_parse_wpa_ie(ie, ie_len, &data) < 0) {
		ret = os_snprintf(pos, end - pos, "?]");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		return pos;
	}

	first = 1;
	if (data.key_mgmt & WPA_KEY_MGMT_IEEE8021X) {
		ret = os_snprintf(pos, end - pos, "%sEAP", first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	if (data.key_mgmt & WPA_KEY_MGMT_PSK) {
		ret = os_snprintf(pos, end - pos, "%sPSK", first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	if (data.key_mgmt & WPA_KEY_MGMT_WPA_NONE) {
		ret = os_snprintf(pos, end - pos, "%sNone", first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
#ifdef CONFIG_IEEE80211R
	if (data.key_mgmt & WPA_KEY_MGMT_FT_IEEE8021X) {
		ret = os_snprintf(pos, end - pos, "%sFT/EAP",
				  first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	if (data.key_mgmt & WPA_KEY_MGMT_FT_PSK) {
		ret = os_snprintf(pos, end - pos, "%sFT/PSK",
				  first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_IEEE80211W
	if (data.key_mgmt & WPA_KEY_MGMT_IEEE8021X_SHA256) {
		ret = os_snprintf(pos, end - pos, "%sEAP-SHA256",
				  first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
	if (data.key_mgmt & WPA_KEY_MGMT_PSK_SHA256) {
		ret = os_snprintf(pos, end - pos, "%sPSK-SHA256",
				  first ? "" : "+");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
		first = 0;
	}
#endif /* CONFIG_IEEE80211W */

	pos = wpa_supplicant_cipher_txt(pos, end, data.pairwise_cipher);

	if (data.capabilities & WPA_CAPABILITY_PREAUTH) {
		ret = os_snprintf(pos, end - pos, "-preauth");
		if (ret < 0 || ret >= end - pos)
			return pos;
		pos += ret;
	}

	ret = os_snprintf(pos, end - pos, "]");
	if (ret < 0 || ret >= end - pos)
		return pos;
	pos += ret;

	return pos;
}


#ifdef CONFIG_WPS
static char * wpa_supplicant_wps_ie_txt_buf(char *pos, char *end,
					    struct wpabuf *wps_ie)
{
	int ret;
	const char *txt;

	if (wps_ie == NULL)
		return pos;
	if (wps_is_selected_pbc_registrar(wps_ie))
		txt = "[WPS-PBC]";
	else if (wps_is_selected_pin_registrar(wps_ie))
		txt = "[WPS-PIN]";
	else
		txt = "[WPS]";

	ret = os_snprintf(pos, end - pos, "%s", txt);
	if (ret >= 0 && ret < end - pos)
		pos += ret;
	wpabuf_free(wps_ie);
	return pos;
}
#endif /* CONFIG_WPS */


static char * wpa_supplicant_wps_ie_txt(char *pos, char *end,
					const struct wpa_bss *bss)
{
#ifdef CONFIG_WPS
	struct wpabuf *wps_ie;
	wps_ie = wpa_bss_get_vendor_ie_multi(bss, WPS_IE_VENDOR_TYPE);
	return wpa_supplicant_wps_ie_txt_buf(pos, end, wps_ie);
#else /* CONFIG_WPS */
	return pos;
#endif /* CONFIG_WPS */
}


/* Format one result on one text line into a buffer. */
static int wpa_supplicant_ctrl_iface_scan_result(
	const struct wpa_bss *bss, char *buf, size_t buflen)
{
	char *pos, *end;
	int ret;
	const u8 *ie, *ie2;

	pos = buf;
	end = buf + buflen;

	ret = os_snprintf(pos, end - pos, MACSTR "\t%d\t%d\t",
			  MAC2STR(bss->bssid), bss->freq, bss->level);
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;
	ie = wpa_bss_get_vendor_ie(bss, WPA_IE_VENDOR_TYPE);
	if (ie)
		pos = wpa_supplicant_ie_txt(pos, end, "WPA", ie, 2 + ie[1]);
	ie2 = wpa_bss_get_ie(bss, WLAN_EID_RSN);
	if (ie2)
		pos = wpa_supplicant_ie_txt(pos, end, "WPA2", ie2, 2 + ie2[1]);
	pos = wpa_supplicant_wps_ie_txt(pos, end, bss);
	if (!ie && !ie2 && bss->caps & IEEE80211_CAP_PRIVACY) {
		ret = os_snprintf(pos, end - pos, "[WEP]");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}
	if (bss->caps & IEEE80211_CAP_IBSS) {
		ret = os_snprintf(pos, end - pos, "[IBSS]");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}
	if (bss->caps & IEEE80211_CAP_ESS) {
		ret = os_snprintf(pos, end - pos, "[ESS]");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}

	ret = os_snprintf(pos, end - pos, "\t%s",
			  wpa_ssid_txt(bss->ssid, bss->ssid_len));
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	ret = os_snprintf(pos, end - pos, "\n");
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	return pos - buf;
}


static int wpa_supplicant_ctrl_iface_scan_results(
	struct wpa_supplicant *wpa_s, char *buf, size_t buflen)
{
	char *pos, *end;
	struct wpa_bss *bss;
	int ret;

	pos = buf;
	end = buf + buflen;
	ret = os_snprintf(pos, end - pos, "bssid / frequency / signal level / "
			  "flags / ssid\n");
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	dl_list_for_each(bss, &wpa_s->bss_id, struct wpa_bss, list_id) {
		ret = wpa_supplicant_ctrl_iface_scan_result(bss, pos,
							    end - pos);
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}

	return pos - buf;
}


static int wpa_supplicant_ctrl_iface_select_network(
	struct wpa_supplicant *wpa_s, char *cmd)
{
	int id;
	struct wpa_ssid *ssid;

	/* cmd: "<network id>" or "any" */
	if (os_strcmp(cmd, "any") == 0) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: SELECT_NETWORK any");
		ssid = NULL;
	} else {
		id = atoi(cmd);
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: SELECT_NETWORK id=%d", id);

		ssid = wpa_config_get_network(wpa_s->conf, id);
		if (ssid == NULL) {
			wpa_printf(MSG_DEBUG, "CTRL_IFACE: Could not find "
				   "network id=%d", id);
			return -1;
		}
	}

	wpa_supplicant_select_network(wpa_s, ssid);

	return 0;
}


static int wpa_supplicant_ctrl_iface_enable_network(
	struct wpa_supplicant *wpa_s, char *cmd)
{
	int id;
	struct wpa_ssid *ssid;

	/* cmd: "<network id>" or "all" */
	if (os_strcmp(cmd, "all") == 0) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: ENABLE_NETWORK all");
		ssid = NULL;
	} else {
		id = atoi(cmd);
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: ENABLE_NETWORK id=%d", id);

		ssid = wpa_config_get_network(wpa_s->conf, id);
		if (ssid == NULL) {
			wpa_printf(MSG_DEBUG, "CTRL_IFACE: Could not find "
				   "network id=%d", id);
			return -1;
		}
	}
	wpa_supplicant_enable_network(wpa_s, ssid);

	return 0;
}


static int wpa_supplicant_ctrl_iface_disable_network(
	struct wpa_supplicant *wpa_s, char *cmd)
{
	int id;
	struct wpa_ssid *ssid;

	/* cmd: "<network id>" or "all" */
	if (os_strcmp(cmd, "all") == 0) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: DISABLE_NETWORK all");
		ssid = NULL;
	} else {
		id = atoi(cmd);
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: DISABLE_NETWORK id=%d", id);

		ssid = wpa_config_get_network(wpa_s->conf, id);
		if (ssid == NULL) {
			wpa_printf(MSG_DEBUG, "CTRL_IFACE: Could not find "
				   "network id=%d", id);
			return -1;
		}
	}
	wpa_supplicant_disable_network(wpa_s, ssid);

	return 0;
}


static int wpa_supplicant_ctrl_iface_add_network(
	struct wpa_supplicant *wpa_s, char *buf, size_t buflen)
{
	struct wpa_ssid *ssid;
	int ret;

	wpa_printf(MSG_DEBUG, "CTRL_IFACE: ADD_NETWORK");

	ssid = wpa_config_add_network(wpa_s->conf);
	if (ssid == NULL)
		return -1;

	wpas_notify_network_added(wpa_s, ssid);

	ssid->disabled = 1;
	wpa_config_set_network_defaults(ssid);

	ret = os_snprintf(buf, buflen, "%d\n", ssid->id);
	if (ret < 0 || (size_t) ret >= buflen)
		return -1;
	return ret;
}


static int wpa_supplicant_ctrl_iface_remove_network(
	struct wpa_supplicant *wpa_s, char *cmd)
{
	int id;
	struct wpa_ssid *ssid;

	/* cmd: "<network id>" or "all" */
	if (os_strcmp(cmd, "all") == 0) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: REMOVE_NETWORK all");
		ssid = wpa_s->conf->ssid;
		while (ssid) {
			struct wpa_ssid *remove_ssid = ssid;
			id = ssid->id;
			ssid = ssid->next;
			wpas_notify_network_removed(wpa_s, remove_ssid);
			wpa_config_remove_network(wpa_s->conf, id);
		}
		if (wpa_s->current_ssid) {
			eapol_sm_invalidate_cached_session(wpa_s->eapol);
			wpa_supplicant_disassociate(wpa_s,
				                    WLAN_REASON_DEAUTH_LEAVING);
		}
		return 0;
	}

	id = atoi(cmd);
	wpa_printf(MSG_DEBUG, "CTRL_IFACE: REMOVE_NETWORK id=%d", id);

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL ||
	    wpa_config_remove_network(wpa_s->conf, id) < 0) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: Could not find network "
			   "id=%d", id);
		return -1;
	}

	if (ssid == wpa_s->current_ssid) {
		/*
		 * Invalidate the EAP session cache if the current network is
		 * removed.
		 */
		eapol_sm_invalidate_cached_session(wpa_s->eapol);

		wpa_supplicant_disassociate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);
	}

	return 0;
}


static int wpa_supplicant_ctrl_iface_set_network(
	struct wpa_supplicant *wpa_s, char *cmd)
{
	int id;
	struct wpa_ssid *ssid;
	char *name, *value;

	/* cmd: "<network id> <variable name> <value>" */
	name = os_strchr(cmd, ' ');
	if (name == NULL)
		return -1;
	*name++ = '\0';

	value = os_strchr(name, ' ');
	if (value == NULL)
		return -1;
	*value++ = '\0';

	id = atoi(cmd);
	wpa_printf(MSG_DEBUG, "CTRL_IFACE: SET_NETWORK id=%d name='%s'",
		   id, name);
	wpa_hexdump_ascii_key(MSG_DEBUG, "CTRL_IFACE: value",
			      (u8 *) value, os_strlen(value));

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: Could not find network "
			   "id=%d", id);
		return -1;
	}

	if (wpa_config_set(ssid, name, value, 0) < 0) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: Failed to set network "
			   "variable '%s'", name);
		return -1;
	}

	if (wpa_s->current_ssid == ssid) {
		/*
		 * Invalidate the EAP session cache if anything in the current
		 * configuration changes.
		 */
		eapol_sm_invalidate_cached_session(wpa_s->eapol);
	}

	if ((os_strcmp(name, "psk") == 0 &&
	     value[0] == '"' && ssid->ssid_len) ||
	    (os_strcmp(name, "ssid") == 0 && ssid->passphrase))
		wpa_config_update_psk(ssid);
	else if (os_strcmp(name, "priority") == 0)
		wpa_config_update_prio_list(wpa_s->conf);

	return 0;
}


static int wpa_supplicant_ctrl_iface_get_network(
	struct wpa_supplicant *wpa_s, char *cmd, char *buf, size_t buflen)
{
	int id;
	size_t res;
	struct wpa_ssid *ssid;
	char *name, *value;

	/* cmd: "<network id> <variable name>" */
	name = os_strchr(cmd, ' ');
	if (name == NULL || buflen == 0)
		return -1;
	*name++ = '\0';

	id = atoi(cmd);
	wpa_printf(MSG_DEBUG, "CTRL_IFACE: GET_NETWORK id=%d name='%s'",
		   id, name);

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: Could not find network "
			   "id=%d", id);
		return -1;
	}

	value = wpa_config_get_no_key(ssid, name);
	if (value == NULL) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: Failed to get network "
			   "variable '%s'", name);
		return -1;
	}

	res = os_strlcpy(buf, value, buflen);
	if (res >= buflen) {
		os_free(value);
		return -1;
	}

	os_free(value);

	return res;
}


#ifndef CONFIG_NO_CONFIG_WRITE
static int wpa_supplicant_ctrl_iface_save_config(struct wpa_supplicant *wpa_s)
{
	int ret;

	if (!wpa_s->conf->update_config) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: SAVE_CONFIG - Not allowed "
			   "to update configuration (update_config=0)");
		return -1;
	}

	ret = wpa_config_write(wpa_s->confname, wpa_s->conf);
	if (ret) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: SAVE_CONFIG - Failed to "
			   "update configuration");
	} else {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE: SAVE_CONFIG - Configuration"
			   " updated");
	}

	return ret;
}
#endif /* CONFIG_NO_CONFIG_WRITE */


static int ctrl_iface_get_capability_pairwise(int res, char *strict,
					      struct wpa_driver_capa *capa,
					      char *buf, size_t buflen)
{
	int ret, first = 1;
	char *pos, *end;
	size_t len;

	pos = buf;
	end = pos + buflen;

	if (res < 0) {
		if (strict)
			return 0;
		len = os_strlcpy(buf, "CCMP TKIP NONE", buflen);
		if (len >= buflen)
			return -1;
		return len;
	}

	if (capa->enc & WPA_DRIVER_CAPA_ENC_CCMP) {
		ret = os_snprintf(pos, end - pos, "%sCCMP", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	if (capa->enc & WPA_DRIVER_CAPA_ENC_TKIP) {
		ret = os_snprintf(pos, end - pos, "%sTKIP", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	if (capa->key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_WPA_NONE) {
		ret = os_snprintf(pos, end - pos, "%sNONE", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	return pos - buf;
}


static int ctrl_iface_get_capability_group(int res, char *strict,
					   struct wpa_driver_capa *capa,
					   char *buf, size_t buflen)
{
	int ret, first = 1;
	char *pos, *end;
	size_t len;

	pos = buf;
	end = pos + buflen;

	if (res < 0) {
		if (strict)
			return 0;
		len = os_strlcpy(buf, "CCMP TKIP WEP104 WEP40", buflen);
		if (len >= buflen)
			return -1;
		return len;
	}

	if (capa->enc & WPA_DRIVER_CAPA_ENC_CCMP) {
		ret = os_snprintf(pos, end - pos, "%sCCMP", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	if (capa->enc & WPA_DRIVER_CAPA_ENC_TKIP) {
		ret = os_snprintf(pos, end - pos, "%sTKIP", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	if (capa->enc & WPA_DRIVER_CAPA_ENC_WEP104) {
		ret = os_snprintf(pos, end - pos, "%sWEP104",
				  first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	if (capa->enc & WPA_DRIVER_CAPA_ENC_WEP40) {
		ret = os_snprintf(pos, end - pos, "%sWEP40", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	return pos - buf;
}


static int ctrl_iface_get_capability_key_mgmt(int res, char *strict,
					      struct wpa_driver_capa *capa,
					      char *buf, size_t buflen)
{
	int ret;
	char *pos, *end;
	size_t len;

	pos = buf;
	end = pos + buflen;

	if (res < 0) {
		if (strict)
			return 0;
		len = os_strlcpy(buf, "WPA-PSK WPA-EAP IEEE8021X WPA-NONE "
				 "NONE", buflen);
		if (len >= buflen)
			return -1;
		return len;
	}

	ret = os_snprintf(pos, end - pos, "NONE IEEE8021X");
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	if (capa->key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA |
			      WPA_DRIVER_CAPA_KEY_MGMT_WPA2)) {
		ret = os_snprintf(pos, end - pos, " WPA-EAP");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}

	if (capa->key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK |
			      WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK)) {
		ret = os_snprintf(pos, end - pos, " WPA-PSK");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}

	if (capa->key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_WPA_NONE) {
		ret = os_snprintf(pos, end - pos, " WPA-NONE");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}

	return pos - buf;
}


static int ctrl_iface_get_capability_proto(int res, char *strict,
					   struct wpa_driver_capa *capa,
					   char *buf, size_t buflen)
{
	int ret, first = 1;
	char *pos, *end;
	size_t len;

	pos = buf;
	end = pos + buflen;

	if (res < 0) {
		if (strict)
			return 0;
		len = os_strlcpy(buf, "RSN WPA", buflen);
		if (len >= buflen)
			return -1;
		return len;
	}

	if (capa->key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
			      WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK)) {
		ret = os_snprintf(pos, end - pos, "%sRSN", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	if (capa->key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA |
			      WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK)) {
		ret = os_snprintf(pos, end - pos, "%sWPA", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	return pos - buf;
}


static int ctrl_iface_get_capability_auth_alg(int res, char *strict,
					      struct wpa_driver_capa *capa,
					      char *buf, size_t buflen)
{
	int ret, first = 1;
	char *pos, *end;
	size_t len;

	pos = buf;
	end = pos + buflen;

	if (res < 0) {
		if (strict)
			return 0;
		len = os_strlcpy(buf, "OPEN SHARED LEAP", buflen);
		if (len >= buflen)
			return -1;
		return len;
	}

	if (capa->auth & (WPA_DRIVER_AUTH_OPEN)) {
		ret = os_snprintf(pos, end - pos, "%sOPEN", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	if (capa->auth & (WPA_DRIVER_AUTH_SHARED)) {
		ret = os_snprintf(pos, end - pos, "%sSHARED",
				  first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	if (capa->auth & (WPA_DRIVER_AUTH_LEAP)) {
		ret = os_snprintf(pos, end - pos, "%sLEAP", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
		first = 0;
	}

	return pos - buf;
}


static int wpa_supplicant_ctrl_iface_get_capability(
	struct wpa_supplicant *wpa_s, const char *_field, char *buf,
	size_t buflen)
{
	struct wpa_driver_capa capa;
	int res;
	char *strict;
	char field[30];
	size_t len;

	/* Determine whether or not strict checking was requested */
	len = os_strlcpy(field, _field, sizeof(field));
	if (len >= sizeof(field))
		return -1;
	strict = os_strchr(field, ' ');
	if (strict != NULL) {
		*strict++ = '\0';
		if (os_strcmp(strict, "strict") != 0)
			return -1;
	}

	wpa_printf(MSG_DEBUG, "CTRL_IFACE: GET_CAPABILITY '%s' %s",
		field, strict ? strict : "");

	if (os_strcmp(field, "eap") == 0) {
		return eap_get_names(buf, buflen);
	}

	res = wpa_drv_get_capa(wpa_s, &capa);

	if (os_strcmp(field, "pairwise") == 0)
		return ctrl_iface_get_capability_pairwise(res, strict, &capa,
							  buf, buflen);

	if (os_strcmp(field, "group") == 0)
		return ctrl_iface_get_capability_group(res, strict, &capa,
						       buf, buflen);

	if (os_strcmp(field, "key_mgmt") == 0)
		return ctrl_iface_get_capability_key_mgmt(res, strict, &capa,
							  buf, buflen);

	if (os_strcmp(field, "proto") == 0)
		return ctrl_iface_get_capability_proto(res, strict, &capa,
						       buf, buflen);

	if (os_strcmp(field, "auth_alg") == 0)
		return ctrl_iface_get_capability_auth_alg(res, strict, &capa,
							  buf, buflen);

	wpa_printf(MSG_DEBUG, "CTRL_IFACE: Unknown GET_CAPABILITY field '%s'",
		   field);

	return -1;
}


static int wpa_supplicant_ctrl_iface_bss(struct wpa_supplicant *wpa_s,
					 const char *cmd, char *buf,
					 size_t buflen)
{
	u8 bssid[ETH_ALEN];
	size_t i;
	struct wpa_bss *bss;
	int ret;
	char *pos, *end;
	const u8 *ie, *ie2;

	if (os_strcmp(cmd, "FIRST") == 0)
		bss = dl_list_first(&wpa_s->bss, struct wpa_bss, list);
	else if (os_strncmp(cmd, "ID-", 3) == 0) {
		i = atoi(cmd + 3);
		bss = wpa_bss_get_id(wpa_s, i);
	} else if (os_strncmp(cmd, "NEXT-", 5) == 0) {
		i = atoi(cmd + 5);
		bss = wpa_bss_get_id(wpa_s, i);
		if (bss) {
			struct dl_list *next = bss->list_id.next;
			if (next == &wpa_s->bss_id)
				bss = NULL;
			else
				bss = dl_list_entry(next, struct wpa_bss,
						    list_id);
		}
	} else if (hwaddr_aton(cmd, bssid) == 0)
		bss = wpa_bss_get_bssid(wpa_s, bssid);
	else {
		struct wpa_bss *tmp;
		i = atoi(cmd);
		bss = NULL;
		dl_list_for_each(tmp, &wpa_s->bss_id, struct wpa_bss, list_id)
		{
			if (i-- == 0) {
				bss = tmp;
				break;
			}
		}
	}

	if (bss == NULL)
		return 0;

	pos = buf;
	end = buf + buflen;
	ret = os_snprintf(pos, end - pos,
			  "id=%u\n"
			  "bssid=" MACSTR "\n"
			  "freq=%d\n"
			  "beacon_int=%d\n"
			  "capabilities=0x%04x\n"
			  "qual=%d\n"
			  "noise=%d\n"
			  "level=%d\n"
			  "tsf=%016llu\n"
			  "ie=",
			  bss->id,
			  MAC2STR(bss->bssid), bss->freq, bss->beacon_int,
			  bss->caps, bss->qual, bss->noise, bss->level,
			  (unsigned long long) bss->tsf);
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	ie = (const u8 *) (bss + 1);
	for (i = 0; i < bss->ie_len; i++) {
		ret = os_snprintf(pos, end - pos, "%02x", *ie++);
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}

	ret = os_snprintf(pos, end - pos, "\n");
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	ret = os_snprintf(pos, end - pos, "flags=");
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	ie = wpa_bss_get_vendor_ie(bss, WPA_IE_VENDOR_TYPE);
	if (ie)
		pos = wpa_supplicant_ie_txt(pos, end, "WPA", ie, 2 + ie[1]);
	ie2 = wpa_bss_get_ie(bss, WLAN_EID_RSN);
	if (ie2)
		pos = wpa_supplicant_ie_txt(pos, end, "WPA2", ie2, 2 + ie2[1]);
	pos = wpa_supplicant_wps_ie_txt(pos, end, bss);
	if (!ie && !ie2 && bss->caps & IEEE80211_CAP_PRIVACY) {
		ret = os_snprintf(pos, end - pos, "[WEP]");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}
	if (bss->caps & IEEE80211_CAP_IBSS) {
		ret = os_snprintf(pos, end - pos, "[IBSS]");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}
	if (bss->caps & IEEE80211_CAP_ESS) {
		ret = os_snprintf(pos, end - pos, "[ESS]");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;
	}

	ret = os_snprintf(pos, end - pos, "\n");
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

	ret = os_snprintf(pos, end - pos, "ssid=%s\n",
			  wpa_ssid_txt(bss->ssid, bss->ssid_len));
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;

#ifdef CONFIG_WPS
	ie = (const u8 *) (bss + 1);
	ret = wpas_wps_scan_result_text(ie, bss->ie_len, pos, end);
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;
#endif /* CONFIG_WPS */

	return pos - buf;
}


static int wpa_supplicant_ctrl_iface_ap_scan(
	struct wpa_supplicant *wpa_s, char *cmd)
{
	int ap_scan = atoi(cmd);
	return wpa_supplicant_set_ap_scan(wpa_s, ap_scan);
}


static void wpa_supplicant_ctrl_iface_drop_sa(struct wpa_supplicant *wpa_s)
{
	u8 *bcast = (u8 *) "\xff\xff\xff\xff\xff\xff";

	wpa_printf(MSG_DEBUG, "Dropping SA without deauthentication");
	/* MLME-DELETEKEYS.request */
	wpa_drv_set_key(wpa_s, WPA_ALG_NONE, bcast, 0, 0, NULL, 0, NULL, 0);
	wpa_drv_set_key(wpa_s, WPA_ALG_NONE, bcast, 1, 0, NULL, 0, NULL, 0);
	wpa_drv_set_key(wpa_s, WPA_ALG_NONE, bcast, 2, 0, NULL, 0, NULL, 0);
	wpa_drv_set_key(wpa_s, WPA_ALG_NONE, bcast, 3, 0, NULL, 0, NULL, 0);
#ifdef CONFIG_IEEE80211W
	wpa_drv_set_key(wpa_s, WPA_ALG_NONE, bcast, 4, 0, NULL, 0, NULL, 0);
	wpa_drv_set_key(wpa_s, WPA_ALG_NONE, bcast, 5, 0, NULL, 0, NULL, 0);
#endif /* CONFIG_IEEE80211W */

	wpa_drv_set_key(wpa_s, WPA_ALG_NONE, wpa_s->bssid, 0, 0, NULL, 0, NULL,
			0);
	/* MLME-SETPROTECTION.request(None) */
	wpa_drv_mlme_setprotection(wpa_s, wpa_s->bssid,
				   MLME_SETPROTECTION_PROTECT_TYPE_NONE,
				   MLME_SETPROTECTION_KEY_TYPE_PAIRWISE);
	wpa_sm_drop_sa(wpa_s->wpa);
}


static int wpa_supplicant_ctrl_iface_roam(struct wpa_supplicant *wpa_s,
					  char *addr)
{
	u8 bssid[ETH_ALEN];
	struct wpa_bss *bss;
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	if (hwaddr_aton(addr, bssid)) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE ROAM: invalid "
			   "address '%s'", addr);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "CTRL_IFACE ROAM " MACSTR, MAC2STR(bssid));

	bss = wpa_bss_get_bssid(wpa_s, bssid);
	if (!bss) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE ROAM: Target AP not found "
			   "from BSS table");
		return -1;
	}

	/*
	 * TODO: Find best network configuration block from configuration to
	 * allow roaming to other networks
	 */

	if (!ssid) {
		wpa_printf(MSG_DEBUG, "CTRL_IFACE ROAM: No network "
			   "configuration known for the target AP");
		return -1;
	}

	wpa_s->reassociate = 1;
	wpa_supplicant_connect(wpa_s, bss, ssid);

	return 0;
}


char * wpa_supplicant_ctrl_iface_process(struct wpa_supplicant *wpa_s,
					 char *buf, size_t *resp_len)
{
	char *reply;
	const int reply_size = 2048;
	int ctrl_rsp = 0;
	int reply_len;

	if (os_strncmp(buf, WPA_CTRL_RSP, os_strlen(WPA_CTRL_RSP)) == 0 ||
	    os_strncmp(buf, "SET_NETWORK ", 12) == 0) {
		wpa_hexdump_ascii_key(MSG_DEBUG, "RX ctrl_iface",
				      (const u8 *) buf, os_strlen(buf));
	} else {
		wpa_hexdump_ascii(MSG_DEBUG, "RX ctrl_iface",
				  (const u8 *) buf, os_strlen(buf));
	}

	reply = os_malloc(reply_size);
	if (reply == NULL) {
		*resp_len = 1;
		return NULL;
	}

	os_memcpy(reply, "OK\n", 3);
	reply_len = 3;

	if (os_strcmp(buf, "PING") == 0) {
		os_memcpy(reply, "PONG\n", 5);
		reply_len = 5;
	} else if (os_strcmp(buf, "MIB") == 0) {
		reply_len = wpa_sm_get_mib(wpa_s->wpa, reply, reply_size);
		if (reply_len >= 0) {
			int res;
			res = eapol_sm_get_mib(wpa_s->eapol, reply + reply_len,
					       reply_size - reply_len);
			if (res < 0)
				reply_len = -1;
			else
				reply_len += res;
		}
	} else if (os_strncmp(buf, "STATUS", 6) == 0) {
		reply_len = wpa_supplicant_ctrl_iface_status(
			wpa_s, buf + 6, reply, reply_size);
	} else if (os_strcmp(buf, "PMKSA") == 0) {
		reply_len = wpa_sm_pmksa_cache_list(wpa_s->wpa, reply,
						    reply_size);
	} else if (os_strncmp(buf, "SET ", 4) == 0) {
		if (wpa_supplicant_ctrl_iface_set(wpa_s, buf + 4))
			reply_len = -1;
	} else if (os_strcmp(buf, "LOGON") == 0) {
		eapol_sm_notify_logoff(wpa_s->eapol, FALSE);
	} else if (os_strcmp(buf, "LOGOFF") == 0) {
		eapol_sm_notify_logoff(wpa_s->eapol, TRUE);
	} else if (os_strcmp(buf, "REASSOCIATE") == 0) {
		wpa_s->disconnected = 0;
		wpa_s->reassociate = 1;
		wpa_supplicant_req_scan(wpa_s, 0, 0);
	} else if (os_strcmp(buf, "RECONNECT") == 0) {
		if (wpa_s->disconnected) {
			wpa_s->disconnected = 0;
			wpa_s->reassociate = 1;
			wpa_supplicant_req_scan(wpa_s, 0, 0);
		}
#ifdef IEEE8021X_EAPOL
	} else if (os_strncmp(buf, "PREAUTH ", 8) == 0) {
		if (wpa_supplicant_ctrl_iface_preauth(wpa_s, buf + 8))
			reply_len = -1;
#endif /* IEEE8021X_EAPOL */
#ifdef CONFIG_PEERKEY
	} else if (os_strncmp(buf, "STKSTART ", 9) == 0) {
		if (wpa_supplicant_ctrl_iface_stkstart(wpa_s, buf + 9))
			reply_len = -1;
#endif /* CONFIG_PEERKEY */
#ifdef CONFIG_IEEE80211R
	} else if (os_strncmp(buf, "FT_DS ", 6) == 0) {
		if (wpa_supplicant_ctrl_iface_ft_ds(wpa_s, buf + 6))
			reply_len = -1;
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_WPS
	} else if (os_strcmp(buf, "WPS_PBC") == 0) {
		if (wpa_supplicant_ctrl_iface_wps_pbc(wpa_s, NULL))
			reply_len = -1;
	} else if (os_strncmp(buf, "WPS_PBC ", 8) == 0) {
		if (wpa_supplicant_ctrl_iface_wps_pbc(wpa_s, buf + 8))
			reply_len = -1;
	} else if (os_strncmp(buf, "WPS_PIN ", 8) == 0) {
		reply_len = wpa_supplicant_ctrl_iface_wps_pin(wpa_s, buf + 8,
							      reply,
							      reply_size);
#ifdef CONFIG_WPS_OOB
	} else if (os_strncmp(buf, "WPS_OOB ", 8) == 0) {
		if (wpa_supplicant_ctrl_iface_wps_oob(wpa_s, buf + 8))
			reply_len = -1;
#endif /* CONFIG_WPS_OOB */
	} else if (os_strncmp(buf, "WPS_REG ", 8) == 0) {
		if (wpa_supplicant_ctrl_iface_wps_reg(wpa_s, buf + 8))
			reply_len = -1;
#ifdef CONFIG_WPS_ER
	} else if (os_strcmp(buf, "WPS_ER_START") == 0) {
		if (wpas_wps_er_start(wpa_s))
			reply_len = -1;
	} else if (os_strcmp(buf, "WPS_ER_STOP") == 0) {
		if (wpas_wps_er_stop(wpa_s))
			reply_len = -1;
	} else if (os_strncmp(buf, "WPS_ER_PIN ", 11) == 0) {
		if (wpa_supplicant_ctrl_iface_wps_er_pin(wpa_s, buf + 11))
			reply_len = -1;
	} else if (os_strncmp(buf, "WPS_ER_PBC ", 11) == 0) {
		if (wpas_wps_er_pbc(wpa_s, buf + 11))
			reply_len = -1;
	} else if (os_strncmp(buf, "WPS_ER_LEARN ", 13) == 0) {
		if (wpa_supplicant_ctrl_iface_wps_er_learn(wpa_s, buf + 13))
			reply_len = -1;
#endif /* CONFIG_WPS_ER */
#endif /* CONFIG_WPS */
#ifdef CONFIG_IBSS_RSN
	} else if (os_strncmp(buf, "IBSS_RSN ", 9) == 0) {
		if (wpa_supplicant_ctrl_iface_ibss_rsn(wpa_s, buf + 9))
			reply_len = -1;
#endif /* CONFIG_IBSS_RSN */
	} else if (os_strncmp(buf, WPA_CTRL_RSP, os_strlen(WPA_CTRL_RSP)) == 0)
	{
		if (wpa_supplicant_ctrl_iface_ctrl_rsp(
			    wpa_s, buf + os_strlen(WPA_CTRL_RSP)))
			reply_len = -1;
		else
			ctrl_rsp = 1;
	} else if (os_strcmp(buf, "RECONFIGURE") == 0) {
		if (wpa_supplicant_reload_configuration(wpa_s))
			reply_len = -1;
	} else if (os_strcmp(buf, "TERMINATE") == 0) {
		wpa_supplicant_terminate_proc(wpa_s->global);
	} else if (os_strncmp(buf, "BSSID ", 6) == 0) {
		if (wpa_supplicant_ctrl_iface_bssid(wpa_s, buf + 6))
			reply_len = -1;
	} else if (os_strcmp(buf, "LIST_NETWORKS") == 0) {
		reply_len = wpa_supplicant_ctrl_iface_list_networks(
			wpa_s, reply, reply_size);
	} else if (os_strcmp(buf, "DISCONNECT") == 0) {
		wpa_s->reassociate = 0;
		wpa_s->disconnected = 1;
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_DEAUTH_LEAVING);
	} else if (os_strcmp(buf, "SCAN") == 0) {
		wpa_s->scan_req = 2;
		wpa_supplicant_req_scan(wpa_s, 0, 0);
	} else if (os_strcmp(buf, "SCAN_RESULTS") == 0) {
		reply_len = wpa_supplicant_ctrl_iface_scan_results(
			wpa_s, reply, reply_size);
	} else if (os_strncmp(buf, "SELECT_NETWORK ", 15) == 0) {
		if (wpa_supplicant_ctrl_iface_select_network(wpa_s, buf + 15))
			reply_len = -1;
	} else if (os_strncmp(buf, "ENABLE_NETWORK ", 15) == 0) {
		if (wpa_supplicant_ctrl_iface_enable_network(wpa_s, buf + 15))
			reply_len = -1;
	} else if (os_strncmp(buf, "DISABLE_NETWORK ", 16) == 0) {
		if (wpa_supplicant_ctrl_iface_disable_network(wpa_s, buf + 16))
			reply_len = -1;
	} else if (os_strcmp(buf, "ADD_NETWORK") == 0) {
		reply_len = wpa_supplicant_ctrl_iface_add_network(
			wpa_s, reply, reply_size);
	} else if (os_strncmp(buf, "REMOVE_NETWORK ", 15) == 0) {
		if (wpa_supplicant_ctrl_iface_remove_network(wpa_s, buf + 15))
			reply_len = -1;
	} else if (os_strncmp(buf, "SET_NETWORK ", 12) == 0) {
		if (wpa_supplicant_ctrl_iface_set_network(wpa_s, buf + 12))
			reply_len = -1;
	} else if (os_strncmp(buf, "GET_NETWORK ", 12) == 0) {
		reply_len = wpa_supplicant_ctrl_iface_get_network(
			wpa_s, buf + 12, reply, reply_size);
#ifndef CONFIG_NO_CONFIG_WRITE
	} else if (os_strcmp(buf, "SAVE_CONFIG") == 0) {
		if (wpa_supplicant_ctrl_iface_save_config(wpa_s))
			reply_len = -1;
#endif /* CONFIG_NO_CONFIG_WRITE */
	} else if (os_strncmp(buf, "GET_CAPABILITY ", 15) == 0) {
		reply_len = wpa_supplicant_ctrl_iface_get_capability(
			wpa_s, buf + 15, reply, reply_size);
	} else if (os_strncmp(buf, "AP_SCAN ", 8) == 0) {
		if (wpa_supplicant_ctrl_iface_ap_scan(wpa_s, buf + 8))
			reply_len = -1;
	} else if (os_strcmp(buf, "INTERFACE_LIST") == 0) {
		reply_len = wpa_supplicant_global_iface_list(
			wpa_s->global, reply, reply_size);
	} else if (os_strcmp(buf, "INTERFACES") == 0) {
		reply_len = wpa_supplicant_global_iface_interfaces(
			wpa_s->global, reply, reply_size);
	} else if (os_strncmp(buf, "BSS ", 4) == 0) {
		reply_len = wpa_supplicant_ctrl_iface_bss(
			wpa_s, buf + 4, reply, reply_size);
#ifdef CONFIG_AP
	} else if (os_strcmp(buf, "STA-FIRST") == 0) {
		reply_len = ap_ctrl_iface_sta_first(wpa_s, reply, reply_size);
	} else if (os_strncmp(buf, "STA ", 4) == 0) {
		reply_len = ap_ctrl_iface_sta(wpa_s, buf + 4, reply,
					      reply_size);
	} else if (os_strncmp(buf, "STA-NEXT ", 9) == 0) {
		reply_len = ap_ctrl_iface_sta_next(wpa_s, buf + 9, reply,
						   reply_size);
#endif /* CONFIG_AP */
	} else if (os_strcmp(buf, "SUSPEND") == 0) {
		wpas_notify_suspend(wpa_s->global);
	} else if (os_strcmp(buf, "RESUME") == 0) {
		wpas_notify_resume(wpa_s->global);
	} else if (os_strcmp(buf, "DROP_SA") == 0) {
		wpa_supplicant_ctrl_iface_drop_sa(wpa_s);
	} else if (os_strncmp(buf, "ROAM ", 5) == 0) {
		if (wpa_supplicant_ctrl_iface_roam(wpa_s, buf + 5))
			reply_len = -1;
	} else {
		os_memcpy(reply, "UNKNOWN COMMAND\n", 16);
		reply_len = 16;
	}

	if (reply_len < 0) {
		os_memcpy(reply, "FAIL\n", 5);
		reply_len = 5;
	}

	if (ctrl_rsp)
		eapol_sm_notify_ctrl_response(wpa_s->eapol);

	*resp_len = reply_len;
	return reply;
}


static int wpa_supplicant_global_iface_add(struct wpa_global *global,
					   char *cmd)
{
	struct wpa_interface iface;
	char *pos;

	/*
	 * <ifname>TAB<confname>TAB<driver>TAB<ctrl_interface>TAB<driver_param>
	 * TAB<bridge_ifname>
	 */
	wpa_printf(MSG_DEBUG, "CTRL_IFACE GLOBAL INTERFACE_ADD '%s'", cmd);

	os_memset(&iface, 0, sizeof(iface));

	do {
		iface.ifname = pos = cmd;
		pos = os_strchr(pos, '\t');
		if (pos)
			*pos++ = '\0';
		if (iface.ifname[0] == '\0')
			return -1;
		if (pos == NULL)
			break;

		iface.confname = pos;
		pos = os_strchr(pos, '\t');
		if (pos)
			*pos++ = '\0';
		if (iface.confname[0] == '\0')
			iface.confname = NULL;
		if (pos == NULL)
			break;

		iface.driver = pos;
		pos = os_strchr(pos, '\t');
		if (pos)
			*pos++ = '\0';
		if (iface.driver[0] == '\0')
			iface.driver = NULL;
		if (pos == NULL)
			break;

		iface.ctrl_interface = pos;
		pos = os_strchr(pos, '\t');
		if (pos)
			*pos++ = '\0';
		if (iface.ctrl_interface[0] == '\0')
			iface.ctrl_interface = NULL;
		if (pos == NULL)
			break;

		iface.driver_param = pos;
		pos = os_strchr(pos, '\t');
		if (pos)
			*pos++ = '\0';
		if (iface.driver_param[0] == '\0')
			iface.driver_param = NULL;
		if (pos == NULL)
			break;

		iface.bridge_ifname = pos;
		pos = os_strchr(pos, '\t');
		if (pos)
			*pos++ = '\0';
		if (iface.bridge_ifname[0] == '\0')
			iface.bridge_ifname = NULL;
		if (pos == NULL)
			break;
	} while (0);

	if (wpa_supplicant_get_iface(global, iface.ifname))
		return -1;

	return wpa_supplicant_add_iface(global, &iface) ? 0 : -1;
}


static int wpa_supplicant_global_iface_remove(struct wpa_global *global,
					      char *cmd)
{
	struct wpa_supplicant *wpa_s;

	wpa_printf(MSG_DEBUG, "CTRL_IFACE GLOBAL INTERFACE_REMOVE '%s'", cmd);

	wpa_s = wpa_supplicant_get_iface(global, cmd);
	if (wpa_s == NULL)
		return -1;
	return wpa_supplicant_remove_iface(global, wpa_s);
}


static void wpa_free_iface_info(struct wpa_interface_info *iface)
{
	struct wpa_interface_info *prev;

	while (iface) {
		prev = iface;
		iface = iface->next;

		os_free(prev->ifname);
		os_free(prev->desc);
		os_free(prev);
	}
}


static int wpa_supplicant_global_iface_list(struct wpa_global *global,
					    char *buf, int len)
{
	int i, res;
	struct wpa_interface_info *iface = NULL, *last = NULL, *tmp;
	char *pos, *end;

	for (i = 0; wpa_drivers[i]; i++) {
		struct wpa_driver_ops *drv = wpa_drivers[i];
		if (drv->get_interfaces == NULL)
			continue;
		tmp = drv->get_interfaces(global->drv_priv[i]);
		if (tmp == NULL)
			continue;

		if (last == NULL)
			iface = last = tmp;
		else
			last->next = tmp;
		while (last->next)
			last = last->next;
	}

	pos = buf;
	end = buf + len;
	for (tmp = iface; tmp; tmp = tmp->next) {
		res = os_snprintf(pos, end - pos, "%s\t%s\t%s\n",
				  tmp->drv_name, tmp->ifname,
				  tmp->desc ? tmp->desc : "");
		if (res < 0 || res >= end - pos) {
			*pos = '\0';
			break;
		}
		pos += res;
	}

	wpa_free_iface_info(iface);

	return pos - buf;
}


static int wpa_supplicant_global_iface_interfaces(struct wpa_global *global,
						  char *buf, int len)
{
	int res;
	char *pos, *end;
	struct wpa_supplicant *wpa_s;

	wpa_s = global->ifaces;
	pos = buf;
	end = buf + len;

	while (wpa_s) {
		res = os_snprintf(pos, end - pos, "%s\n", wpa_s->ifname);
		if (res < 0 || res >= end - pos) {
			*pos = '\0';
			break;
		}
		pos += res;
		wpa_s = wpa_s->next;
	}
	return pos - buf;
}


char * wpa_supplicant_global_ctrl_iface_process(struct wpa_global *global,
						char *buf, size_t *resp_len)
{
	char *reply;
	const int reply_size = 2048;
	int reply_len;

	wpa_hexdump_ascii(MSG_DEBUG, "RX global ctrl_iface",
			  (const u8 *) buf, os_strlen(buf));

	reply = os_malloc(reply_size);
	if (reply == NULL) {
		*resp_len = 1;
		return NULL;
	}

	os_memcpy(reply, "OK\n", 3);
	reply_len = 3;

	if (os_strcmp(buf, "PING") == 0) {
		os_memcpy(reply, "PONG\n", 5);
		reply_len = 5;
	} else if (os_strncmp(buf, "INTERFACE_ADD ", 14) == 0) {
		if (wpa_supplicant_global_iface_add(global, buf + 14))
			reply_len = -1;
	} else if (os_strncmp(buf, "INTERFACE_REMOVE ", 17) == 0) {
		if (wpa_supplicant_global_iface_remove(global, buf + 17))
			reply_len = -1;
	} else if (os_strcmp(buf, "INTERFACE_LIST") == 0) {
		reply_len = wpa_supplicant_global_iface_list(
			global, reply, reply_size);
	} else if (os_strcmp(buf, "INTERFACES") == 0) {
		reply_len = wpa_supplicant_global_iface_interfaces(
			global, reply, reply_size);
	} else if (os_strcmp(buf, "TERMINATE") == 0) {
		wpa_supplicant_terminate_proc(global);
	} else if (os_strcmp(buf, "SUSPEND") == 0) {
		wpas_notify_suspend(global);
	} else if (os_strcmp(buf, "RESUME") == 0) {
		wpas_notify_resume(global);
	} else {
		os_memcpy(reply, "UNKNOWN COMMAND\n", 16);
		reply_len = 16;
	}

	if (reply_len < 0) {
		os_memcpy(reply, "FAIL\n", 5);
		reply_len = 5;
	}

	*resp_len = reply_len;
	return reply;
}
