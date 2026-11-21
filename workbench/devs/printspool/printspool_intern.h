/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef PRINTSPOOL_INTERN_H
#define PRINTSPOOL_INTERN_H

#include <exec/devices.h>
#include <exec/semaphores.h>
#include <dos/dos.h>

#define PRINTSPOOL_UNITS    10      /* as many as printer.device has */
#define REQUEST_MAX         16384   /* a job's own IPP request (PRD_SETJOBINFO) */

struct PrintSpoolBase
{
    struct Device ps_Device;

    struct PrintSpoolUnit
    {
        ULONG pu_Openers;
        BPTR pu_Job;                /* the job being written, or BNULL */
        TEXT pu_Name[64];           /* its file name, without the suffix */
        TEXT pu_Application[64];    /* PRD_SETJOBINFO from printer.device */
        TEXT pu_Document[128];
        UBYTE *pu_Request;          /* the job's own IPP request, or NULL */
        ULONG pu_RequestSize;
        BOOL pu_GivenUp;            /* CMD_RESET: nothing of the job is printed */
    } ps_Unit[PRINTSPOOL_UNITS];

    /* Opened with PRDOF_SETTINGS: no job, does not hold a job back from the spooler. */
    struct PrintSpoolSettingsUnit
    {
        ULONG pss_Number;
    } ps_Settings[PRINTSPOOL_UNITS];

    struct SignalSemaphore ps_Lock;
    BOOL ps_Paused[PRINTSPOOL_UNITS];  /* CMD_STOP from the spooler's own UI: no jobs sent */
    ULONG ps_Sequence;
    struct Process *ps_Spooler;
    ULONG ps_SpoolerSignal;
    struct Library *ps_DOSBase;
};

#define DOSBase PrintSpoolBase->ps_DOSBase

#endif /* PRINTSPOOL_INTERN_H */
