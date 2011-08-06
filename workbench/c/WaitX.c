/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Execute a command after a given time
    Lang: English
*/

/******************************************************************************

    NAME

        WaitX

    SYNOPSIS

        D=DATE/K,T=TIME/K,YR=YEARS/K/N,MN=MONTHS/K/N,DY=DAYS/K/N,H=HOURS/K/N,
        M=MINS/K/N,S=SECS/K/N,L=LOOP/K/N,A=ALWAYS/S,V=VERBOSE/S,HELP/S,CMDLINE/F

    LOCATION

        C:

    FUNCTION

        WaitX will wait for a given amount of time and then it
        will execute the given command.

    INPUTS

        D=DATE     -- Waits until DATE has been reached
        T=TIME     -- Waits until TIME has been reached
        YR=YEARS   -- How many years to wait
        MN=MONTHS  -- How many months to wait
        DY=DAYS    -- How many days to wait
        H=HOURS    -- How many hours to wait
        M=MINS     -- How many minutes to wait
        S=SECS     -- How many seconds to wait
        L=LOOP     -- How many times to execute CMDLINE
        A=ALWAYS   -- Execute CMDLINE every set interval/time/date
        V=VERBOSE  -- Print extra info on what waitx is doing

    EXAMPLE

        $ waitx TIME=12:34:12 echo "this is an example"
        waitx waits until 12:34:12 is reached and will execute echo

        $ waitx H=5 M=36 echo "this is an example"
        waitx will wait 5 hours and 36 minutes and execute echo

        $ waitx HOURS=2 MINS=12 SECS=59
        waitx will wait 2 hours, 12 minutes and 59 seconds and then
        returns to the prompt

        $ waitx DY=1 L=5 echo "this is an example"
        waitx will wait 1 day and execute echo,
        then repeat this a total of 5 times

        $ waitx M=15 ALWAYS echo "this is an example"
        waitx will execute echo every 15 minutes

        $ waitx D=12/9 T=16 L=0 echo "this is an example"
        waitx will wait until September 12th 16:00 and execute echo,
        and repeat forever

        $ waitx echo "this is an example"
        waitx will execute echo immediatly

    RESULT

    NOTES

        Based on Public Domain WaitX:
            http://aminet.net/package/util/cli/waitx
            Programming: Sigbjørn Skjæret <cisc@c2i.net>
            Idea & Docs: Nicholas Stallard <snowy@netphile.de>

    EXAMPLE

    BUGS

        Will not return  to prompt while waiting. This is intended.

    SEE ALSO

    INTERNALS

******************************************************************************/

#define __USE_SYSBASE
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <proto/timer.h>
#include <proto/utility.h>
#include <utility/date.h>


struct Interval
{
    ULONG set;
    ULONG years;
    ULONG months;
    ULONG seconds;
};

const char Version[] = "$VER: WaitX 2.1 (16.04.2008)";

int strtoi(STRPTR string);

LONG MainEntry(struct ExecBase *SysBase);

__startup static AROS_ENTRY(int, Start,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  struct ExecBase *, sBase)
{
    AROS_USERFUNC_INIT
    return MainEntry(sBase);
    AROS_USERFUNC_EXIT
}


LONG MainEntry(struct ExecBase *SysBase)
{
    struct DosLibrary *DOSBase = NULL;
    struct UtilityBase *UtilityBase = NULL;
    struct Library *TimerBase;

    BYTE TimerDevice = -1;
    struct ClockData *clock = NULL;
    struct Interval *interval = NULL;
    struct MsgPort *TimerPort = NULL;
    struct timerequest *TimerReq = NULL;
    ULONG i, loop, step, unit, seconds = 0, signal = 0, timesig = 0, usersig = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E;

    BPTR StdErr = BNULL, StdIn;
    char ProgName[256];
    struct RDArgs *rdargs = NULL;
    STRPTR Template = "D=DATE/K,T=TIME/K,YR=YEARS/K/N,MN=MONTHS/K/N,DY=DAYS/K/N,H=HOURS/K/N,M=MINS/K/N,S=SECS/K/N,L=LOOP/K/N,A=ALWAYS/S,V=VERBOSE/S,HELP/S,CMDLINE/F";

    enum
    {
        TEM_DATE,
        TEM_TIME,
        TEM_YEARS,
        TEM_MONTHS,
        TEM_DAYS,
        TEM_HOURS,
        TEM_MINS,
        TEM_SECS,
        TEM_LOOP,
        TEM_ALWAYS,
        TEM_VERBOSE,
        TEM_HELP,
        TEM_CMDLINE,
        TEM_NUMARGS
    };

    IPTR ArgArray[TEM_NUMARGS], ret;
    STRPTR cmdline;
    STRPTR tmpptr;

    ret = 20; /* Fatal error if something fails here */
    if (!(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36)))
        goto exit;
    if (!(UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 36)))
        goto exit;
    if (!(StdErr = Open("CONSOLE:", MODE_NEWFILE)))
        goto exit;
    ret = 0; /* Clear return */

    GetProgramName(ProgName, sizeof(ProgName));
    for (i=0; i<TEM_NUMARGS; ArgArray[i++]=0)
        ;
    rdargs = ReadArgs(Template, ArgArray, NULL);

    if (ArgArray[TEM_HELP] || !rdargs)
    {
        FPrintf(StdErr, "%s\n\n", &Version[6]);
        FPrintf(StdErr, "Usage: %s [DATE=<DD/MM/YYYY>] [TIME=<HH:MM:SS>] <commandline>\n", ProgName);
        FPrintf(StdErr, "Usage: %s [HOURS=<hours>] [MINS=<mins>] [SECS=<secs>] <commandline>\n", ProgName);
        goto exit;
    }

    loop = step = 1;
    if (ArgArray[TEM_LOOP])
        loop = *((ULONG *)ArgArray[TEM_LOOP]);

    if (ArgArray[TEM_ALWAYS] || loop == 0)
    {
        loop = 1;
        step = 0;
    }

    if (ArgArray[TEM_CMDLINE])
        cmdline = (STRPTR)ArgArray[TEM_CMDLINE];
    else
        cmdline = ""; /* Prevented from Execute()ing later */

    if (ArgArray[TEM_DATE] || ArgArray[TEM_TIME] || ArgArray[TEM_YEARS] || ArgArray[TEM_MONTHS])
    {
        unit = UNIT_WAITUNTIL;
        if (!(clock = AllocMem(sizeof(struct ClockData), MEMF_PUBLIC | MEMF_CLEAR)))
        {
            FPrintf(StdErr, "Unable to allocate needed memory!\n");
            ret = 10;
            goto exit;
        }
        if (!(interval = AllocMem(sizeof(struct Interval), MEMF_PUBLIC | MEMF_CLEAR)))
        {
            FPrintf(StdErr, "Unable to allocate needed memory!\n");
            ret = 10;
            goto exit;
        }

        if (ArgArray[TEM_YEARS])
            interval->years = *((ULONG *)ArgArray[TEM_YEARS]);
        if (ArgArray[TEM_MONTHS])
            interval->months = *((ULONG *)ArgArray[TEM_MONTHS]);
        if (ArgArray[TEM_DAYS])
            interval->seconds += *((ULONG *)ArgArray[TEM_DAYS]) * 86400;
        if (ArgArray[TEM_HOURS])
            interval->seconds += *((ULONG *)ArgArray[TEM_HOURS]) * 3600;
        if (ArgArray[TEM_MINS])
            interval->seconds += *((ULONG *)ArgArray[TEM_MINS]) * 60;
        if (ArgArray[TEM_SECS])
            interval->seconds += *((ULONG *)ArgArray[TEM_SECS]);

        if (interval->years || interval->months || interval->seconds)
            interval->set = 1;
    }
    else
    {
        unit = UNIT_VBLANK;
        seconds = 0;

        if (ArgArray[TEM_DAYS])
            seconds += *((ULONG *)ArgArray[TEM_DAYS]) * 86400;
        if (ArgArray[TEM_HOURS])
            seconds += *((ULONG *)ArgArray[TEM_HOURS]) * 3600;
        if (ArgArray[TEM_MINS])
            seconds += *((ULONG *)ArgArray[TEM_MINS]) * 60;
        if (ArgArray[TEM_SECS])
            seconds += *((ULONG *)ArgArray[TEM_SECS]);
    }

    if (seconds > 0 || clock)
    {
        if (!(TimerPort = CreateMsgPort()))
        {
            FPrintf(StdErr, "Couldn't create Timer-MsgPort!\n");
            ret = 10;
            goto exit;
        }
        if (!(TimerReq = (struct timerequest *)CreateIORequest(TimerPort, sizeof(struct timerequest))))
        {
            FPrintf(StdErr, "Couldn't create Timer-IORequest!\n");
            ret = 10;
            goto exit;
        }
        if ((TimerDevice = OpenDevice(TIMERNAME, unit, (struct IORequest *)TimerReq, 0)))
        {
            FPrintf(StdErr, "Couldn't open %s!\n", TIMERNAME);
            ret = 10;
            goto exit;
        }

        TimerBase = (struct Library *)TimerReq->tr_node.io_Device;
        timesig = 1L << TimerPort->mp_SigBit;

        if (clock)
        {
            GetSysTime(&TimerReq->tr_time); /* Get current System Time */
            Amiga2Date(TimerReq->tr_time.tv_secs, clock); /* Fill in current date/time as default */

            if (ArgArray[TEM_DATE])
            {
                tmpptr = (STRPTR)ArgArray[TEM_DATE];
                clock->mday = strtoi(tmpptr);
                if ((tmpptr = strstr(tmpptr, "/")))
                {
                    clock->month = strtoi(++tmpptr);
                    if ((tmpptr = strstr(tmpptr, "/")))
                    {
                        clock->year = strtoi(++tmpptr);
                    }
                    if (!interval->set)
                    {
                        interval->years = 1;
                        interval->set = 1;
                    }
                }
                else if (!interval->set)
                {
                    interval->months = 1;
                    interval->set = 1;
                }

                if (clock->year < 100) /* Be nice to digit-challenged ppl. ;) */
                {
                    if (clock->year < 78)
                        clock->year += 2000; /* If less than 78, assume 20xx */
                    else
                        clock->year += 1900;
                }
            }
            else if (!interval->set)
            {
                interval->seconds = 86400;
                interval->set = 1;
            }

            if (ArgArray[TEM_TIME])
            {
                tmpptr = (STRPTR)ArgArray[TEM_TIME];
                clock->hour = strtoi(tmpptr);
                clock->min = clock->sec = 0; /* Clear default time */
                if ((tmpptr = strstr(tmpptr, ":")))
                {
                    clock->min = strtoi(++tmpptr);
                    if ((tmpptr = strstr(tmpptr, ":")))
                        clock->sec = strtoi(++tmpptr);
                }
            }

            if (!(seconds = CheckDate(clock)))
            {
                FPrintf(StdErr, "Invalid date/time!\n");
                ret = 5;
                goto exit;
            }
            if (seconds <= TimerReq->tr_time.tv_secs)
            {
                ULONG count = loop;

                for (i=0; i<count; i+=step)
                {
                    clock->year += interval->years;
                    clock->month += interval->months;

                    if (clock->month > 12) /* We need some annual magic */
                    {
                        clock->month %= 12;
                        clock->year += clock->month / 12;
                    }

                    if (!(seconds = CheckDate(clock)))
                    {
                        clock->mday -= 1; /* If date doesn't exist, try previous day */
                        if (!(seconds = CheckDate(clock)))
                        {
                            clock->mday -= 1;
                            if (!(seconds = CheckDate(clock)))
                            {
                                clock->mday -= 1;
                                if (!(seconds = CheckDate(clock)))
                                {
                                    FPrintf(StdErr, "Invalid date/time!\n");
                                    ret = 5;
                                    goto exit;
                                } /* Give up */
                                clock->mday += 1;
                            }
                            clock->mday += 1;
                        }
                        clock->mday += 1; /* Restore day for future reference */
                    }

                    if (interval->seconds)
                    {
                        seconds += interval->seconds;
                        Amiga2Date(seconds, clock);
                    }

                    if (loop > 0 && step > 0)
                        loop--;
                    if (seconds > TimerReq->tr_time.tv_secs)
                        break;
                }

                if (seconds <= TimerReq->tr_time.tv_secs || loop == 0)
                {
                    FPrintf(StdErr, "Date/time has already passed!\n");
                    ret = 5;
                    goto exit;
                }
                else if (ArgArray[TEM_VERBOSE])
                {
                    FPrintf(StdErr, "Note: Schedule has been moved to %02lu/%02lu/%lu %02lu:%02lu:%02lu because the assigned date/time has already passed",
                    (ULONG)clock->mday, (ULONG)clock->month, (ULONG)clock->year, (ULONG)clock->hour, (ULONG)clock->min, (ULONG)clock->sec);

                    if (step == 0)
                        FPrintf(StdErr, ".\n");
                    else
                        FPrintf(StdErr, " (%lu loop(s) left).\n", loop);
                }
            }
        }
    }

    if (IsInteractive((StdIn = Input())))
        StdIn = BNULL; /* Don't use StdIn if it isn't redirected */

    for (i=0; i<loop; i+=step)
    {
        if (seconds > 0)
        {
            TimerReq->tr_time.tv_secs = seconds;
            TimerReq->tr_time.tv_micro = 0;
            TimerReq->tr_node.io_Command = TR_ADDREQUEST;

            SendIO((struct IORequest *)TimerReq);
            signal = Wait(timesig | usersig);

            if (signal & usersig)
            {
                AbortIO((struct IORequest *)TimerReq);
                WaitIO((struct IORequest *)TimerReq);
                SetSignal(0L, timesig | usersig); /* Clear signalbits since WaitIO most likely preserves them */

                if (signal & SIGBREAKF_CTRL_C)
                    break;
            }
        }

        if (cmdline[0] == '\0')
            break; /* There's no point in going on */
        if (!(signal & SIGBREAKF_CTRL_D))
        {
            if (!Execute(cmdline, StdIn, Output()))
            {
                FPrintf(StdErr, "Unable to execute \"%s\".\n", cmdline);
                ret = 5;
                goto exit;
            }
        }
        if (seconds == 0)
            break; /* Don't go into tight unbreakable loop */

        if (clock)
        {
            clock->year += interval->years;
            clock->month += interval->months;

            if (clock->month > 12) /* We need some annual magic */
            {
                clock->month %= 12;
                clock->year += clock->month / 12;
            }

            if (!(seconds = CheckDate(clock)))
            {
                clock->mday -= 1; /* If date doesn't exist, try previous day */
                if (!(seconds = CheckDate(clock)))
                {
                    clock->mday -= 1;
                    if (!(seconds = CheckDate(clock)))
                    {
                        clock->mday -= 1;
                        if (!(seconds = CheckDate(clock)))
                        {
                            FPrintf(StdErr, "Invalid date/time!\n");
                            ret = 5;
                            goto exit;
                        } /* Give up */
                        clock->mday += 1;
                    }
                    clock->mday += 1;
                }
                clock->mday += 1; /* Restore day for future reference */
            }

            if (interval->seconds)
            {
                seconds += interval->seconds;
                Amiga2Date(seconds, clock);
            }
        }

        if (ArgArray[TEM_VERBOSE] && (i+1<loop || step == 0))
        {
            FPrintf(StdErr, "Next scheduled execution ");

            if (clock)
                FPrintf(StdErr, "at %02lu/%02lu/%lu %02lu:%02lu:%02lu", (ULONG)clock->mday, (ULONG)clock->month, (ULONG)clock->year,
                (ULONG)clock->hour, (ULONG)clock->min, (ULONG)clock->sec);
            else
                FPrintf(StdErr, "in %lu hours, %lu minutes and %lu seconds", seconds / 3600, (seconds % 3600) / 60, seconds % 60);

            if (step == 0)
                FPrintf(StdErr, ".\n");
            else
                FPrintf(StdErr, " (%lu loop(s) left).\n", loop-i-1);
        }
    }

exit:
    if (clock)
        FreeMem(clock, sizeof(struct ClockData));
    if (interval)
        FreeMem(interval, sizeof(struct Interval));

    if (!TimerDevice)
        CloseDevice((struct IORequest *)TimerReq);
    if (TimerReq)
        DeleteIORequest((struct IORequest *)TimerReq);
    if (TimerPort)
        DeleteMsgPort(TimerPort);

    if (rdargs)
        FreeArgs(rdargs);

    Close(StdErr);
    if (DOSBase)
        CloseLibrary((struct Library *)DOSBase);

    return ret;
}


/* A simple atoi()-alike function because it does what we need, and no more. */
int strtoi(STRPTR string)
{
    int i, num;

    for (i=0,num=0; string[i]>='0' && string[i]<='9'; ++i)
        num = 10 * num + (string[i] - '0');

    return num;
}
