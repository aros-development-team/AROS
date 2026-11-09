/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/******************************************************************************

    NAME

        FindPrinters

    SYNOPSIS

        UNIT/K/N, NAME/K, FORMAT/K, WAIT/K/N, SAVE/S, PREFS/S, TO/K, URI/K

    LOCATION

        C:

    FUNCTION

        Finds the network printers on the local link (DNS-SD over multicast
        DNS, RFC 6762 and 6763) and lists them: IPP printers (_ipp._tcp) and
        those that take a raw job on a socket (_pdl-datastream._tcp).
        Each IPP printer is asked about itself (Get-Printer-Attributes): its
        make and model, state, the paper it has loaded and its supplies.

        With UNIT, sets ipp.device's unit to a printer found: the one whose
        name contains NAME, or the only one there is. The unit's settings go
        to ENV:IPP/Unit<n>, and with SAVE also to ENVARC:. With PREFS as
        well, printer.device prints through that unit: its preferences
        (SYS/printer.prefs) get printspool.device unit n as their device,
        the rest of them as they were, and printspool.device sends unit n's
        jobs to ipp.device unit n (PrintSpool/Unit<n>) in the background. A document the printer reads itself,
        such as a PDF, can go to the unit without printer.device through
        IPP0: (Mount IPP0:, from SYS:Storage/DOSDrivers).

    INPUTS

        UNIT    -- ipp.device unit to set up (0-9).
        NAME    -- part of the printer's name, when there are several.
        FORMAT  -- document format to send, e.g. application/postscript;
                   the default lets the printer tell.
        WAIT    -- seconds to listen for answers (default 3).
        SAVE    -- keep the unit's settings over a reboot.
        PREFS   -- with UNIT, point printer.device at the unit.
        TO      -- also write what was found to this file for programs, one
                   block of Key=value lines per printer.
        URI     -- ask this one IPP printer (ipp://host[:port]/path) instead
                   of looking for printers.

    EXAMPLE

        FindPrinters
        FindPrinters UNIT 0 NAME Dell FORMAT application/postscript SAVE
        FindPrinters UNIT 0 NAME HP PREFS SAVE
        FindPrinters URI ipp://192.168.0.63/ipp/print UNIT 0 PREFS
        FindPrinters UNIT 0 NAME Dell SAVE
        Mount IPP0:
        Copy report.pdf TO IPP0:

******************************************************************************/

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <devices/timer.h>
#include <devices/printer.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/bsdsocket.h>

#include <prefs/printertxt.h>

#include <string.h>

#include "dnssd.h"
#include "fp_ipp.h"
#include "fp_prefs.h"

const TEXT version[] = "$VER: FindPrinters 1.2 (7.11.2026)";

#define TEMPLATE "UNIT/K/N,NAME/K,FORMAT/K,WAIT/K/N,SAVE/S,PREFS/S,TO/K,URI/K"
enum { ARG_UNIT, ARG_NAME, ARG_FORMAT, ARG_WAIT, ARG_SAVE, ARG_PREFS, ARG_TO, ARG_URI, ARG_COUNT };

#define MAX_PRINTERS    32

struct Printer {
    char p_Name[NAME_MAX];              /* the instance's first label */
    char p_URI[NAME_MAX];
    char p_Formats[TXT_MAX];
    char p_UUID[64];
    BOOL p_IPP;
    BOOL p_HaveInfo;
    struct PrinterInfo p_Info;
};

static struct Printer printers[MAX_PRINTERS];
static LONG nprinters;
struct Library *SocketBase;
struct Device *TimerBase;

/* ------------------------------------------------------------------ settings */

/* ipp.device unit: the printer, its name and ID, and the document format (empty: the printer tells) */
static LONG setUnit(LONG unit, struct Printer *p, const char *format, BOOL save)
{
    struct TagItem tags[] = {
        { PDA_URI,    (IPTR)p->p_URI },
        { PDA_Name,   (IPTR)p->p_Name },
        { PDA_UUID,   (IPTR)p->p_UUID },
        { PDA_Format, (IPTR)(format ? format : "") },
        { PDA_Save,   save },
        { TAG_DONE,   0 }
    };

    return Dest_Set("ipp.device", unit, tags);
}

static void showInfo(struct Printer *p)
{
    struct PrinterInfo *pi = &p->p_Info;
    LONG i;

    if (!p->p_HaveInfo)
        return;
    if (pi->pi_Model[0])
        Printf("   model: %s\n", pi->pi_Model);
    if (pi->pi_State[0])
        Printf("   state: %s\n", pi->pi_State);
    if (pi->pi_Reasons[0] && strcmp(pi->pi_Reasons, "none") != 0)
        Printf("   attention: %s\n", pi->pi_Reasons);
    if (pi->pi_Message[0])
        Printf("   message: %s\n", pi->pi_Message);
    if (pi->pi_Paper[0])
        Printf("   paper: %s\n", pi->pi_Paper);
    for (i = 0; i < pi->pi_NumSupplies; i++)
        if (pi->pi_Level[i] >= 0)
            Printf("   %s: %ld%%\n", pi->pi_Supply[i], (long)pi->pi_Level[i]);
}

/* The TO file: a block of Key=value lines per printer, a blank line between. */
static BOOL writeList(const char *to)
{
    BPTR fh;
    LONG i, j;

    if (!(fh = Open(to, MODE_NEWFILE)))
        return FALSE;
    for (i = 0; i < nprinters; i++) {
        struct Printer *p = &printers[i];
        struct PrinterInfo *pi = &p->p_Info;

        if (i)
            FPuts(fh, "\n");
        FPrintf(fh, "Printer=%s\nURI=%s\n", p->p_Name, p->p_URI);
        if (p->p_Formats[0])
            FPrintf(fh, "Formats=%s\n", p->p_Formats);
        if (p->p_UUID[0])
            FPrintf(fh, "UUID=%s\n", p->p_UUID);
        if (!p->p_HaveInfo)
            continue;
        if (pi->pi_Model[0])
            FPrintf(fh, "Model=%s\n", pi->pi_Model);
        if (pi->pi_State[0])
            FPrintf(fh, "State=%s\n", pi->pi_State);
        if (pi->pi_Reasons[0])
            FPrintf(fh, "Reasons=%s\n", pi->pi_Reasons);
        if (pi->pi_Message[0])
            FPrintf(fh, "Message=%s\n", pi->pi_Message);
        if (pi->pi_Paper[0])
            FPrintf(fh, "Paper=%s\n", pi->pi_Paper);
        for (j = 0; j < pi->pi_NumSupplies; j++)
            FPrintf(fh, "Supply=%ld;%s;%s\n", (long)pi->pi_Level[j], pi->pi_Colour[j], pi->pi_Supply[j]);
    }
    Close(fh);
    return TRUE;
}

static BOOL contains(const char *hay, const char *needle)
{
    LONG n = strlen(needle);

    for (; *hay; hay++)
        if (strncasecmp(hay, needle, n) == 0)
            return TRUE;
    return FALSE;
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct MsgPort *port;
    struct timerequest *tr = NULL;
    LONG wait = 3, i, rc = RETURN_OK;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL))) {
        PrintFault(IoErr(), "FindPrinters");
        return RETURN_FAIL;
    }
    if (args[ARG_WAIT])
        wait = *(LONG *)args[ARG_WAIT];
    if ((port = CreateMsgPort()) && (tr = (struct timerequest *)CreateIORequest(port, sizeof(*tr)))
        && OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0) == 0)
        TimerBase = tr->tr_node.io_Device;
    else {
        PutStr("FindPrinters: no timer.device\n");
        if (tr)
            DeleteIORequest((struct IORequest *)tr);
        if (port)
            DeleteMsgPort(port);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (!(SocketBase = OpenLibrary("bsdsocket.library", 4))) {
        PutStr("FindPrinters: the network is not running\n");
        CloseDevice((struct IORequest *)tr);
        DeleteIORequest((struct IORequest *)tr);
        DeleteMsgPort(port);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (args[ARG_URI]) {
        /* One printer, asked directly. */
        struct Printer *p = &printers[0];
        const char *uri = (const char *)args[ARG_URI];

        if (strncmp(uri, "ipp://", 6) != 0 || strlen(uri) >= sizeof(p->p_URI)) {
            PutStr("FindPrinters: URI is ipp://host[:port]/path\n");
            rc = RETURN_ERROR;
        } else {
            memset(p, 0, sizeof(*p));
            strcpy(p->p_URI, uri);
            strcpy(p->p_Name, uri);
            p->p_IPP = TRUE;
            nprinters = 1;
        }
    } else {
        static struct DNSSDPrinter found[MAX_PRINTERS];
        LONG n, error;

        if ((n = DNSSD_FindPrinters(SocketBase, TimerBase, wait * 1000, found, MAX_PRINTERS, &error)) < 0) {
            Printf("FindPrinters: cannot send to the multicast group (error %ld): is an interface up?\n", (long)error);
            rc = RETURN_FAIL;
        }
        for (i = 0; i < n; i++) {
            struct Printer *p = &printers[nprinters++];

            memset(p, 0, sizeof(*p));
            strcpy(p->p_Name, found[i].dp_Name);
            strcpy(p->p_URI, found[i].dp_URI);
            strcpy(p->p_Formats, found[i].dp_Formats);
            strcpy(p->p_UUID, found[i].dp_UUID);
            p->p_IPP = found[i].dp_IPP;
        }
    }

    if (rc == RETURN_OK) {
        if (nprinters == 0)
            PutStr("No network printers found.\n");
        for (i = 0; i < nprinters; i++) {
            struct Printer *p = &printers[i];

            if (p->p_IPP)
                p->p_HaveInfo = IPP_GetInfo(p->p_URI, &p->p_Info);
            /* a printer asked by URI has no DNS-SD record to name its formats */
            if (p->p_HaveInfo && !p->p_Formats[0])
                strncpy(p->p_Formats, p->p_Info.pi_Formats, sizeof(p->p_Formats) - 1);
            if (args[ARG_URI] && p->p_HaveInfo && p->p_Info.pi_Model[0])
                strncpy(p->p_Name, p->p_Info.pi_Model, sizeof(p->p_Name) - 1);
            if (args[ARG_URI] && !p->p_HaveInfo) {
                Printf("FindPrinters: %s does not answer\n", p->p_URI);
                rc = RETURN_WARN;
            }
            Printf("%ld  %s\n   %s\n", (long)i + 1, p->p_Name, p->p_URI);
            if (p->p_Formats[0])
                Printf("   formats: %s\n", p->p_Formats);
            showInfo(p);
        }
        if (args[ARG_TO] && !writeList((const char *)args[ARG_TO])) {
            PrintFault(IoErr(), "FindPrinters");
            rc = RETURN_ERROR;
        }

        if (args[ARG_PREFS] && !args[ARG_UNIT]) {
            PutStr("FindPrinters: PREFS needs UNIT\n");
            rc = RETURN_ERROR;
        } else if (args[ARG_UNIT]) {
            LONG unit = *(LONG *)args[ARG_UNIT];
            const char *name = (const char *)args[ARG_NAME];
            struct Printer *pick = NULL;
            LONG matches = 0, error;
            for (i = 0; i < nprinters; i++)
                if (!name || contains(printers[i].p_Name, name)) {
                    if (!pick || (printers[i].p_IPP && !pick->p_IPP))
                        pick = &printers[i];
                    matches++;
                }
            if (unit < 0 || unit > 9) {
                PutStr("FindPrinters: UNIT is 0 to 9\n");
                rc = RETURN_ERROR;
            } else if (!pick) {
                PutStr("FindPrinters: no printer to set up\n");
                rc = RETURN_WARN;
            } else if (!name && nprinters > 1 && matches > 1) {
                PutStr("FindPrinters: several printers; name one with NAME\n");
                rc = RETURN_WARN;
            } else if ((error = setUnit(unit, pick, (const char *)args[ARG_FORMAT], args[ARG_SAVE] != 0)) != 0) {
                if (IoErr() != 0)
                    PrintFault(IoErr(), "FindPrinters: ipp.device");
                else
                    Printf("FindPrinters: ipp.device unit %ld refused the printer (error %ld)\n", (long)unit, (long)error);
                rc = RETURN_ERROR;
            } else {
                char driver[DRIVERNAMESIZE + 1];

                Printf("ipp.device unit %ld: %s\n", (long)unit, pick->p_Name);
                if (args[ARG_PREFS]) {
                    if (!Prefs_UseIPP(unit, args[ARG_SAVE] != 0, driver, sizeof(driver))) {
                        PrintFault(IoErr(), "FindPrinters");
                        rc = RETURN_ERROR;
                    } else {
                        Printf("printer.device: prints with the %s driver through printspool.device unit %ld to ipp.device unit %ld\n",
                            driver, (long)unit, (long)unit);
                        if (strcasecmp(driver, "PostScript") == 0 && pick->p_Formats[0]
                            && !contains(pick->p_Formats, "postscript"))
                            PutStr("FindPrinters: the printer does not list PostScript; choose a driver for it in Printer preferences\n");
                    }
                }
            }
        }
    }
    CloseLibrary(SocketBase);
    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(port);
    FreeArgs(rda);
    return rc;
}
