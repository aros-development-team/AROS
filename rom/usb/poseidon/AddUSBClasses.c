/*
** AddUSBClasses by Chris Hodges <chrisly@platon42.de>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <exec/exec.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <dos/exall.h>
#include <libraries/poseidon.h>
#include <proto/poseidon.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARGS_QUIET    0
#define ARGS_REMOVE   1
#define ARGS_SIZEOF   2

static const char *template = "QUIET/S,REMOVE/S";
const char *version = "$VER: AddUSBClasses 1.8 (24.05.2017) © The AROS Development Team";
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
    UBYTE               buf[1024];
    struct Library      *ps;
    struct ExAllControl *exall;
    struct ExAllData    *exdata;
    STRPTR              errmsg = NULL;
    struct List         *puclist;
    struct Node         *puc;
    STRPTR              classpath;
    STRPTR              sbuf;
    BPTR                lock;
    ULONG               ents;
    ULONG               namelen;
    BOOL                exready, isvalid;

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        fail("Wrong arguments!\n");
    }
    
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        if(ArgsArray[ARGS_REMOVE])
        {
            psdLockWritePBase();
            psdGetAttrs(PGA_STACK, NULL, PA_ClassList, &puclist, TAG_END);
            while(puclist->lh_Head->ln_Succ)
            {
                if(!ArgsArray[ARGS_QUIET])
                {
                    Printf("Removing class %s...\n", puclist->lh_Head->ln_Name);
                }
                psdUnlockPBase();
                psdRemClass(puclist->lh_Head);
                psdLockWritePBase();
            }
            psdUnlockPBase();
        } else {
            if((exall = AllocDosObject(DOS_EXALLCONTROL, NULL)))
            {
                classpath = "SYS:Classes/USB";
                lock = Lock(classpath, ACCESS_READ);
                if(lock)
                {
                    exall->eac_LastKey = 0;
                    exall->eac_MatchString = NULL;
                    exall->eac_MatchFunc = NULL;
                    do
                    {
                        exready = ExAll(lock, (struct ExAllData *) buf, 1024, ED_NAME, exall);
                        exdata = (struct ExAllData *) buf;
                        ents = exall->eac_Entries;
                        while(ents--)
                        {
                            isvalid = TRUE;
                            sbuf = psdCopyStrFmt("%s/%s", classpath, exdata->ed_Name);
                            if(!sbuf)
                            {
                                break;
                            }
                            namelen = strlen(sbuf);
                            if (((namelen > 4) && (!strcmp(&sbuf[namelen-4], ".dbg"))) || ((namelen > 5) && (!strcmp(&sbuf[namelen-5], ".info"))))
                                isvalid = FALSE;
                            if (isvalid)
                            {
                                if(namelen > 4)
                                {
                                    if(!strcmp(&sbuf[namelen-4], ".elf"))
                                    {
                                        sbuf[namelen-4] = 0;
                                    }
                                }
                                psdGetAttrs(PGA_STACK, NULL, PA_ClassList, &puclist, TAG_END);
                                Forbid();
                                puc = puclist->lh_Head;
                                while(puc->ln_Succ)
                                {
                                    if(!strncmp(puc->ln_Name, exdata->ed_Name, strlen(puc->ln_Name)))
                                    {
                                        break;
                                    }
                                    puc = puc->ln_Succ;
                                }
                                if(puc->ln_Succ)
                                {
                                    Permit();
                                    if(!ArgsArray[ARGS_QUIET])
                                    {
                                        Printf("Skipping class %s...\n", exdata->ed_Name);
                                    }
                                    exdata = exdata->ed_Next;
                                    continue;
                                }
                                Permit();

                                if(!ArgsArray[ARGS_QUIET])
                                {
                                    Printf("Adding class %s...", exdata->ed_Name);
                                }
                                if(psdAddClass(sbuf, 0))
                                {
                                    if(!ArgsArray[ARGS_QUIET])
                                    {
                                        PutStr("okay!\n");
                                    }
                                } else {
                                    if(!ArgsArray[ARGS_QUIET])
                                    {
                                        PutStr("failed!\n");
                                    }
                                }
                            }
                            psdFreeVec(sbuf);
                            exdata = exdata->ed_Next;
                        }
                    } while(exready);
                    UnLock(lock);
                    psdClassScan();
                } else {
                    errmsg = "Could not lock on SYS:Classes/USB.\n";
                }
                FreeDosObject(DOS_EXALLCONTROL, exall);
            }
        }
        CloseLibrary(ps);
    } else {
        errmsg = "Unable to open poseidon.library\n";
    }
    fail(errmsg);
    return(0); // never gets here, just to shut the compiler up
}
