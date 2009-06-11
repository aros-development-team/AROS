/*
** PsdErrorlog by Chris Hodges <chrisly@platon42.de>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/exec.h>
#include <dos/datetime.h>
#include <libraries/poseidon.h>
#include <proto/poseidon.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARGS_NOFLUSH  0
#define ARGS_NOTS     1
#define ARGS_SIZEOF   2

static const char *template = "NOFLUSH/S,NOTIMESTAMPS=NOTS/S";
static const char *version = "$VER: PsdErrorlog 4.0 (03.06.09) by Chris Hodges <chrisly@platon42.de>";
static IPTR ArgsArray[ARGS_SIZEOF];
static struct RDArgs *ArgsHook = NULL;

void fail(char *str)
{
    if(ArgsHook)
    {
        FreeArgs(ArgsHook);
        ArgsHook = NULL;
    }
    if(str)
    {
        PutStr(str);
        exit(20);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    struct Library *ps;
    struct List *errmsgs;
    struct Node *pem;
    ULONG level;
    IPTR origin;
    IPTR errstr;
    STRPTR errmsg = NULL;
    struct DateStamp *ds;
    struct DateTime dt;
    struct DateStamp currdate;
    UBYTE strdate[LEN_DATSTRING];
    UBYTE strtime[LEN_DATSTRING];

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
        fail("Wrong arguments!\n");

    if((ps = OpenLibrary("poseidon.library", 3)))
    {
        DateStamp(&currdate);
        if(ArgsArray[ARGS_NOFLUSH])
        {
            psdLockReadPBase();
        } else {
            psdLockWritePBase();
        }
        psdGetAttrs(PGA_STACK, NULL, PA_ErrorMsgList, &errmsgs, TAG_END);
        pem = errmsgs->lh_Head;
        while(pem->ln_Succ)
        {
            ds = NULL;
            psdGetAttrs(PGA_ERRORMSG, pem,
                        EMA_Level, &level,
                        EMA_Origin, &origin,
                        EMA_Msg, &errstr,
                        EMA_DateStamp, &ds,
                        TAG_END);
            if(ds && (!ArgsArray[ARGS_NOTS]))
            {
                dt.dat_Stamp.ds_Days = ds->ds_Days;
                dt.dat_Stamp.ds_Minute = ds->ds_Minute;
                dt.dat_Stamp.ds_Tick = ds->ds_Tick;
                dt.dat_Format = FORMAT_DEF;
                dt.dat_Flags = 0;
                dt.dat_StrDay = NULL;
                dt.dat_StrDate = strdate;
                dt.dat_StrTime = strtime;
                DateToStr(&dt);
                if(currdate.ds_Days == ds->ds_Days)
                {
                    Printf("%s| %2ld-%s: %s\n", strtime, level, origin, errstr);
                } else {
                    Printf("%s %s| %2ld-%s: %s\n", strdate, strtime, level, origin, errstr);
                }
            } else {
                Printf("%2ld-%s: %s\n", level, origin, errstr);
            }
            pem = pem->ln_Succ;
        }
        if(!ArgsArray[ARGS_NOFLUSH])
        {
            Forbid();
            while(errmsgs->lh_Head->ln_Succ)
            {
                psdRemErrorMsg(errmsgs->lh_Head);
            }
            Permit();
        }
        psdUnlockPBase();
        CloseLibrary(ps);
    } else {
        errmsg = "Unable to open poseidon.library\n";
    }
    fail(errmsg);
    return(0); // never gets here, just to shut the compiler up
}

