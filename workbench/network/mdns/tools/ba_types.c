/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 *
 * ba_types.c - plain-English service-type aliases (see ba_types.h).
 */

#include <exec/types.h>

#include "ba_types.h"

struct BAServiceType
{
    const char *bt_Alias;       /* short word the user may type (or NULL) */
    const char *bt_Type;        /* DNS-SD service type */
    const char *bt_Friendly;    /* human description */
};

/*
 * Ordered so the first entry for a given type carries the description used by
 * BATypeFriendly().  Several aliases may point at the same type.
 */
static const struct BAServiceType BAServiceTypes[] =
{
    { "web",         "_http._tcp",           "Web server (HTTP)" },
    { "http",        "_http._tcp",           "Web server (HTTP)" },
    { "https",       "_https._tcp",          "Secure web (HTTPS)" },
    { "files",       "_afpovertcp._tcp",     "Apple file share" },
    { "afp",         "_afpovertcp._tcp",     "Apple file share" },
    { "smb",         "_smb._tcp",            "Windows file share (SMB)" },
    { "nfs",         "_nfs._tcp",            "NFS share" },
    { "webdav",      "_webdav._tcp",         "WebDAV share" },
    { "ftp",         "_ftp._tcp",            "FTP server" },
    { "ssh",         "_ssh._tcp",            "SSH login" },
    { "sftp",        "_sftp-ssh._tcp",       "SFTP (SSH file transfer)" },
    { "telnet",      "_telnet._tcp",         "Telnet login" },
    { "vnc",         "_rfb._tcp",            "Screen sharing (VNC)" },
    { "rfb",         "_rfb._tcp",            "Screen sharing (VNC)" },
    { "printer",     "_ipp._tcp",            "Printer (IPP)" },
    { "ipp",         "_ipp._tcp",            "Printer (IPP)" },
    { "ipps",        "_ipps._tcp",           "Secure printer (IPPS)" },
    { "lpr",         "_printer._tcp",        "Printer (LPR)" },
    { "scanner",     "_scanner._tcp",        "Scanner" },
    { "airplay",     "_airplay._tcp",        "AirPlay" },
    { "airtunes",    "_raop._tcp",           "AirPlay audio (RAOP)" },
    { "cast",        "_googlecast._tcp",     "Chromecast" },
    { "chromecast",  "_googlecast._tcp",     "Chromecast" },
    { "spotify",     "_spotify-connect._tcp","Spotify Connect" },
    { "companion",   "_companion-link._tcp", "Apple companion device" },
    { "homekit",     "_hap._tcp",            "HomeKit accessory" },
    { "workstation", "_workstation._tcp",    "Workstation" },
    { "device",      "_device-info._tcp",    "Device info" },
    { "time",        "_timedate._tcp",       "Time/date service" },
    { "envoy",       "_envoy-nipc._tcp",     "Envoy host" },
    { NULL,          NULL,                   NULL }
};

BOOL
BAStrEqualCI(const char *a, const char *b)
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
BATypeFromAlias(CONST_STRPTR word)
{
    LONG i;

    if (word == NULL) {
        return NULL;
    }
    /* Already a real type (or a subtype path) - pass through untouched. */
    if (word[0] == '_') {
        return word;
    }
    for (i = 0; BAServiceTypes[i].bt_Alias != NULL; i++) {
        if (BAStrEqualCI((const char *)word, BAServiceTypes[i].bt_Alias)) {
            return (CONST_STRPTR)BAServiceTypes[i].bt_Type;
        }
    }
    /* Unknown word: return as-is so the browse simply finds nothing. */
    return word;
}

CONST_STRPTR
BATypeFriendly(CONST_STRPTR type)
{
    LONG i;

    if (type == NULL) {
        return (CONST_STRPTR)"?";
    }
    for (i = 0; BAServiceTypes[i].bt_Type != NULL; i++) {
        if (BAStrEqualCI((const char *)type, BAServiceTypes[i].bt_Type)) {
            return (CONST_STRPTR)BAServiceTypes[i].bt_Friendly;
        }
    }
    return type;
}
