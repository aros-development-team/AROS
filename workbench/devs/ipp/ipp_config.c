/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/var.h>

#include <string.h>

#include "ipp_intern.h"

void IPP_AppendNumber(char *s, ULONG n)
{
    char digits[12];
    int i = 0;

    do {
        digits[i++] = '0' + (n % 10);
        n /= 10;
    } while (n);
    s += strlen(s);
    while (i)
        *s++ = digits[--i];
    *s = 0;
}

static void copyValue(char *to, const char *from, ULONG max)
{
    ULONG n = 0;

    while (from[n] && from[n] != '\n' && from[n] != '\r' && n < max - 1) {
        to[n] = from[n];
        n++;
    }
    while (n > 0 && (to[n - 1] == ' ' || to[n - 1] == '\t'))
        n--;
    to[n] = 0;
}

/* ENV:IPP/Unit<n>: lines of KEY=value, as FindPrinters writes them.
   URI=ipp://192.168.0.63:631/ipp/print   (or socket://host[:9100])
   FORMAT=application/postscript           (optional)
   NAME=Dell Color MFP E525w               (optional)
   Other keys are kept for the tools and ignored here. */
BOOL IPP_ReadConfig(struct IPPBase *IPPBase, ULONG unit, struct IPPConfig *ic)
{
    char path[32] = "ENV:IPP/Unit";
    char line[IPP_URI_MAX + 16];
    BPTR fh;

    memset(ic, 0, sizeof(*ic));
    IPP_AppendNumber(path, unit);
    fh = Open(path, MODE_OLDFILE);
    if (fh == BNULL)
        return FALSE;
    while (FGets(fh, line, sizeof(line))) {
        if (strncmp(line, "URI=", 4) == 0)
            copyValue(ic->ic_URI, line + 4, sizeof(ic->ic_URI));
        else if (strncmp(line, "FORMAT=", 7) == 0)
            copyValue(ic->ic_Format, line + 7, sizeof(ic->ic_Format));
        else if (strncmp(line, "NAME=", 5) == 0)
            copyValue(ic->ic_Name, line + 5, sizeof(ic->ic_Name));
    }
    Close(fh);
    if (ic->ic_Format[0] == 0)
        strcpy(ic->ic_Format, "application/octet-stream");
    D(bug("[ipp] unit %lu: %s (%s)\n", (unsigned long)unit, ic->ic_URI, ic->ic_Format));
    return ic->ic_URI[0] != 0;
}

BOOL IPP_ParseURI(const char *uri, struct IPPTarget *t)
{
    const char *p = uri, *host;
    ULONG n;

    memset(t, 0, sizeof(*t));
    if (strncmp(p, "ipp://", 6) == 0 || strncmp(p, "http://", 7) == 0) {
        t->it_IPP = TRUE;
        t->it_Port = 631;
        p = strchr(p, '/') + 2;
    } else if (strncmp(p, "socket://", 9) == 0) {
        t->it_Port = 9100;
        p += 9;
    } else
        return FALSE;

    host = p;
    while (*p && *p != ':' && *p != '/')
        p++;
    n = p - host;
    if (n == 0 || n >= sizeof(t->it_Host))
        return FALSE;
    memcpy(t->it_Host, host, n);
    t->it_Host[n] = 0;
    if (*p == ':') {
        ULONG port = 0;
        for (p++; *p >= '0' && *p <= '9'; p++) {
            port = port * 10 + (*p - '0');
            if (port > 65535)
                return FALSE;
        }
        if (port == 0)
            return FALSE;
        t->it_Port = port;
    }
    strncpy(t->it_Path, *p == '/' ? p : "/", sizeof(t->it_Path) - 1);
    return TRUE;
}

/* ENV:IPP/Status: "Unit<n>: <what happened to the last job>". */
void IPP_SetStatus(struct IPPBase *IPPBase, struct IPPUnit *iu, const char *text)
{
    char line[200] = "Unit";
    BPTR lock;

    IPP_AppendNumber(line, iu->iu_Number);
    strcat(line, ": ");
    strncat(line, text, sizeof(line) - strlen(line) - 1);
    /* SetVar() makes the variable's file but not its directory. */
    if ((lock = Lock("ENV:IPP", SHARED_LOCK)) == BNULL)
        lock = CreateDir("ENV:IPP");
    if (lock != BNULL)
        UnLock(lock);
    SetVar("IPP/Status", line, -1, GVF_GLOBAL_ONLY);
}
