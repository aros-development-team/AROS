/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Date CLI command
    Lang: English
*/

#include <stdio.h>
#include <proto/exec.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/datetime.h>
#include <devices/timer.h>

#include <string.h>
#include <stdlib.h>

static const char version[] = "$VER: Date 41.4 (5.3.2000)\n";

#define ARG_STRING "DAY,DATE,TIME,TO=VER/K"
#define ARG_DAY 0
#define ARG_DATE 1
#define ARG_TIME 2
#define ARG_VER 3
#define ARG_COUNT 4

static WORD chrcount(STRPTR s, UBYTE c)
{
    UBYTE sc;
    WORD retval = 0;
    
    while((sc = *s++))
    {
    	if (sc == c) retval++;
    }
    
    return retval;    
}

int setdate(STRPTR *day_date_time)
{
    int                error = RETURN_OK;
    BYTE               timererror;
    struct timerequest *timerReq;
    struct MsgPort     *timerMP;
    struct DateTime    dt;
    WORD    	       i, count;
    UBYTE   	       fulltime[9];
    STRPTR  	       realtime = NULL, realdate = NULL;
    
    for(i = 0; i < 3; i++)
    {
    	if (day_date_time[i] == NULL) continue;
	
	if (chrcount(day_date_time[i], '-'))
	{
	    /* must be date */
	    
	    realdate = day_date_time[i];
	}
	else if ((count = chrcount(day_date_time[i], ':')))
	{
	    /* must be time */
	    if (count == 1)
	    {
	    	/* seconds are missing */
		
		if (strlen(day_date_time[i]) <= 5)
		{
		    strcpy(fulltime, day_date_time[i]);
		    strcat(fulltime, ":00");
		    realtime = fulltime;
		}
		else
		{
		    realtime = day_date_time[i];
		}
	    }
	    else
	    {
	    	realtime = day_date_time[i];
	    }
	}
	else
	{
	    /* must be week day name */
	    
	    if (!realdate) realdate = day_date_time[i];
	}
	
    }

    timerMP = CreateMsgPort();
    if (timerMP)
    {
	timerReq = (struct timerequest *)CreateIORequest(timerMP, sizeof(struct timerequest));

    	if (timerReq)
	{
	    timererror = OpenDevice(TIMERNAME, UNIT_VBLANK, 
				    &timerReq->tr_node, 0L);
	    if(timererror == 0)
	    {
    		dt.dat_Format  = FORMAT_DOS;
		dt.dat_Flags   = DTF_FUTURE;
		dt.dat_StrDay  = NULL; /* StrToDate ignores this anyway */	
		dt.dat_StrDate = realdate;
		dt.dat_StrTime = realtime;

		DateStamp(&dt.dat_Stamp);

		if((!realdate && !realtime) || (StrToDate(&dt) == 0))
		{
		    PutStr("***Bad args:\n"
			   "- use DD-MMM-YY or <dayname> or yesterday etc. to set date\n"
			   "      HH:MM:SS or HH:MM to set time\n");
		    error = RETURN_FAIL;
		} else {

			timerReq->tr_time.tv_secs = dt.dat_Stamp.ds_Days*60*60*24 +
	                        	   dt.dat_Stamp.ds_Minute*60 +
	                        	   dt.dat_Stamp.ds_Tick / TICKS_PER_SECOND;
			timerReq->tr_time.tv_micro = 0;
			timerReq->tr_node.io_Command = TR_SETSYSTIME;
			timerReq->tr_node.io_Flags |= IOF_QUICK;

			DoIO(&timerReq->tr_node);
		}

        	CloseDevice(&timerReq->tr_node);
	    } 
	    else
	    {
        	PutStr("Date: Error opening timer.device\n");
        	error = RETURN_FAIL;
	    }
	    DeleteIORequest(&timerReq->tr_node);
	}
	else
	{
            PutStr("Date: Error creating timerequest\n");
            error = RETURN_FAIL; 	    
	}
	DeleteMsgPort(timerMP);
    }
    else
    {
        PutStr("Date: Error creating MsgPort\n");
        error = RETURN_FAIL;
    }
    
    return error;
}


int printdate(STRPTR filename)
{
    BPTR file = Output();
    int ownfile = 0;
    int error = RETURN_OK;
    struct DateTime dt;
    char dowstring[LEN_DATSTRING * 2], datestring[LEN_DATSTRING * 2],
	timestring[LEN_DATSTRING * 2], resstring[LEN_DATSTRING*6+1];
    
    if(filename != NULL)
    {
        file = Open(filename, MODE_NEWFILE);
        ownfile = 1;
    }
    
    if(file != (BPTR)NULL)
    {
        int pos = 0;
	
        DateStamp(&dt.dat_Stamp);

        dt.dat_Format = FORMAT_DEF;
        dt.dat_Flags = 0;
        dt.dat_StrDay = dowstring;
        dt.dat_StrDate = datestring;
        dt.dat_StrTime = timestring;
        DateToStr(&dt);

        CopyMem(dowstring, resstring, strlen(dowstring));
        pos += strlen(dowstring);
        resstring[pos++] = ' ';
        CopyMem(datestring, resstring + pos, strlen(datestring));
        pos += strlen(datestring);
        resstring[pos++] = ' ';
        CopyMem(timestring, resstring + pos, strlen(timestring));
        pos += strlen(timestring);
        resstring[pos++] = 0x0a;

        if(Write(file, resstring, pos) < pos)
        {
            PrintFault(IoErr(), "Date");
            error = RETURN_FAIL;
        }

        if(ownfile == 1)
            Close(file);
    }
    else
    {
        PrintFault(IoErr(), "Date");
        error = RETURN_FAIL;
    }

    return error;
}

int __nocommandline = 1;

int main(void)
{
    int            error = RETURN_OK;
    STRPTR         args[ARG_COUNT] = {NULL, NULL, NULL, NULL};
    struct RDArgs *rda;

    rda = ReadArgs(ARG_STRING, (IPTR *)args, NULL);

    if (rda != NULL)
    {
        if (args[ARG_DAY] != NULL || args[ARG_DATE] != NULL ||
	    args[ARG_TIME] != NULL)
        {
            if ((error = setdate(args) == RETURN_OK))
            {
                if (args[ARG_VER] != NULL)
                    printdate(args[ARG_VER]);
            }
        } 
	else
            error = printdate(args[ARG_VER]);

        FreeArgs(rda);
    }
    else
    {
        PrintFault(IoErr(), "Date");
        error = RETURN_FAIL;
    }
    
    return error;
}
