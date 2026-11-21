/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * sd_types.c - plain-English service-type aliases (see sd_types.h).
 */

#include <exec/types.h>

#include "sd_common.h"
#include "sd_types.h"

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#include "../strings.h"

struct SDServiceType
{
    const char *bt_Alias;       /* short word the user may type */
    const char *bt_Type;        /* DNS-SD service type */
    LONG        bt_Friendly;    /* its description: a catalog message */
    const char *bt_English;     /* and its English */
};

#define SD_TYPE(alias, type, msg)   { alias, type, msg, msg##_STR }

/*
 * The first entry of a type is the one SDTypeAt() counts; several aliases
 * may point at the same type.
 */
static const struct SDServiceType SDServiceTypes[] =
{
    SD_TYPE("web",         "_http._tcp",            MSG_TYPE_HTTP),
    SD_TYPE("http",        "_http._tcp",            MSG_TYPE_HTTP),
    SD_TYPE("https",       "_https._tcp",           MSG_TYPE_HTTPS),
    SD_TYPE("files",       "_afpovertcp._tcp",      MSG_TYPE_AFP),
    SD_TYPE("afp",         "_afpovertcp._tcp",      MSG_TYPE_AFP),
    SD_TYPE("smb",         "_smb._tcp",             MSG_TYPE_SMB),
    SD_TYPE("nfs",         "_nfs._tcp",             MSG_TYPE_NFS),
    SD_TYPE("webdav",      "_webdav._tcp",          MSG_TYPE_WEBDAV),
    SD_TYPE("ftp",         "_ftp._tcp",             MSG_TYPE_FTP),
    SD_TYPE("ssh",         "_ssh._tcp",             MSG_TYPE_SSH),
    SD_TYPE("sftp",        "_sftp-ssh._tcp",        MSG_TYPE_SFTP),
    SD_TYPE("telnet",      "_telnet._tcp",          MSG_TYPE_TELNET),
    SD_TYPE("vnc",         "_rfb._tcp",             MSG_TYPE_RFB),
    SD_TYPE("rfb",         "_rfb._tcp",             MSG_TYPE_RFB),
    SD_TYPE("printer",     "_ipp._tcp",             MSG_TYPE_IPP),
    SD_TYPE("ipp",         "_ipp._tcp",             MSG_TYPE_IPP),
    SD_TYPE("ipps",        "_ipps._tcp",            MSG_TYPE_IPPS),
    SD_TYPE("lpr",         "_printer._tcp",         MSG_TYPE_LPR),
    SD_TYPE("scanner",     "_scanner._tcp",         MSG_TYPE_SCANNER),
    SD_TYPE("airplay",     "_airplay._tcp",         MSG_TYPE_AIRPLAY),
    SD_TYPE("airtunes",    "_raop._tcp",            MSG_TYPE_RAOP),
    SD_TYPE("cast",        "_googlecast._tcp",      MSG_TYPE_CAST),
    SD_TYPE("chromecast",  "_googlecast._tcp",      MSG_TYPE_CAST),
    SD_TYPE("spotify",     "_spotify-connect._tcp", MSG_TYPE_SPOTIFY),
    SD_TYPE("companion",   "_companion-link._tcp",  MSG_TYPE_COMPANION),
    SD_TYPE("homekit",     "_hap._tcp",             MSG_TYPE_HAP),
    SD_TYPE("workstation", "_workstation._tcp",     MSG_TYPE_WORKSTATION),
    SD_TYPE("device",      "_device-info._tcp",     MSG_TYPE_DEVICE),
    SD_TYPE("time",        "_timedate._tcp",        MSG_TYPE_TIME),
    SD_TYPE("envoy",       "_envoy-nipc._tcp",      MSG_TYPE_ENVOY),
    { NULL,          NULL,                    0, NULL }
};

BOOL
SDStrEqualCI(const char *a, const char *b)
{
    LONG i = 0;

    if (a == NULL || b == NULL) {
        return FALSE;
    }
    for (;;) {
        char ca = a[i];
        char cb = b[i];

        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca + ('a' - 'A'));
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb + ('a' - 'A'));
        }
        if (ca != cb) {
            return FALSE;
        }
        if (ca == '\0') {
            return TRUE;
        }
        i++;
    }
}

CONST_STRPTR
SDTypeFromAlias(CONST_STRPTR word)
{
    LONG i;

    if (word == NULL) {
        return NULL;
    }
    /* Already a real type (or a subtype path) - pass through untouched. */
    if (word[0] == '_') {
        return word;
    }
    for (i = 0; SDServiceTypes[i].bt_Alias != NULL; i++) {
        if (SDStrEqualCI((const char *)word, SDServiceTypes[i].bt_Alias)) {
            return (CONST_STRPTR)SDServiceTypes[i].bt_Type;
        }
    }
    /* Unknown word: return as-is so the browse simply finds nothing. */
    return word;
}

CONST_STRPTR
SDTypeFriendly(CONST_STRPTR type)
{
    LONG i;

    if (type == NULL) {
        return (CONST_STRPTR)"?";
    }
    for (i = 0; SDServiceTypes[i].bt_Type != NULL; i++) {
        if (SDStrEqualCI((const char *)type, SDServiceTypes[i].bt_Type)) {
            return SDMsgText(SDServiceTypes[i].bt_Friendly,
                (CONST_STRPTR)SDServiceTypes[i].bt_English);
        }
    }
    return type;
}

CONST_STRPTR
SDTypeAt(LONG n)
{
    LONG i, j;

    for (i = 0; SDServiceTypes[i].bt_Type != NULL; i++) {
        BOOL first = TRUE;

        for (j = 0; j < i && first; j++) {
            first = !SDStrEqualCI(SDServiceTypes[j].bt_Type,
                SDServiceTypes[i].bt_Type);
        }
        if (first && n-- == 0) {
            return (CONST_STRPTR)SDServiceTypes[i].bt_Type;
        }
    }
    return NULL;
}
