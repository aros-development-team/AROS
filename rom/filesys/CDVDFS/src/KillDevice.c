/* WARNING!!!
   This little program is actually a VERY BAD EVIL HACK! Avoid using it if possible!
   Please think 10 times before you implement any technique used here!!!
   Thanks to MorphOS team for so inflexible OS. :-( I hope some day unmounting issue
   will be fixed and this crap will go into history.
   Pavel Fedin <sonic_amiga@rambler.ru>
*/

#include <devices/input.h>
#include <devices/inputevent.h>
#include <dos/dosextens.h>
#include <exec/execbase.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
unsigned char name[256];

ULONG __abox__ = 1;

int Start()
{
    BPTR lock;
    STRPTR dev;
    struct RDArgs *Args;
    struct Task *task;
    struct DosList *dl;
    struct IOStdReq *InputRequest;
    struct MsgPort *InputPort;
    struct InputEvent InputEvent;

    /* Initialise */
    SysBase = *(struct ExecBase **)(4L);
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",36);
    if (!DOSBase)
	return RETURN_FAIL;

    /* Get device name from user */
    Args = ReadArgs("NAME/A",(LONG *)&dev,0);
    if (!Args)
    {
        PutStr("KillDevice: required argument missing\n");
        CloseLibrary((struct Library *)DOSBase);
        return RETURN_FAIL;
    }

    /* Suppress "Please replace volume in any drive" requester */
    task=FindTask(NULL);
    ((struct Process *)task)->pr_WindowPtr = (APTR)-1;

    /* Check if there's a disk in drive and figure out volume name in the case */
    lock=Lock(dev,ACCESS_READ);
    if (lock)
    {
        NameFromLock(lock,name,256);
        name[strlen(name)-1]='\0';  /* Strip the trailing colon */
        UnLock(lock);
    }

    /* Cut off the trailing ':' */
    dev[strlen(dev)-1]=0;

    /* Well, let's play. Since we're doing REALLY evil thing we have to stop multitasking.
       Otherwise the OS will not forgive us! */
    Forbid();

    /* First we destroy the handler's task. We cannot do RemTask() here because OS hangs
       afterwards. This code is taken from Scout <http://sourceforge.net/projects/scoutos>
       from Freeze function, it seems to be the safest way to stop a task in MorphOS.
       If we just remove task from exec list and drop it without adding to TaskWait
       list we well also hang.
       Hope this really works. Thanks again to MorphOS team :-( */
    if (task = FindTask (dev))
    {
        Remove ((struct Node *) task);
        task->tc_State = 0xFF;
        Enqueue ((struct List *) &SysBase->TaskWait, (struct Node *) task);
    }

    /* After that we unbind the corpse from the OS. */
    dl = LockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_WRITE);

    /* Remove physical device name from DOS list */
    RemDosEntry(FindDosEntry(dl, dev, LDF_DEVICES));

    /* If there is a disk in drive we need to get rid of volume name also */
    if (lock)
    {
        /* Remove it from DOS list */
        RemDosEntry(FindDosEntry(dl, name, LDF_VOLUMES));

        /* And send IECLASS_DISKREMOVED event in order to put away icon on Ambient */
        InputPort = (struct MsgPort *) CreateMsgPort ();
        if (InputPort)
        {
            InputRequest = (struct IOStdReq *)CreateIORequest (InputPort, sizeof (struct IOStdReq));
            if (InputRequest)
            {
                if (!OpenDevice ("input.device", 0, (struct IORequest *) InputRequest, 0))
                {
	            memset (&InputEvent, 0, sizeof (struct InputEvent));
	            InputEvent.ie_Class = IECLASS_DISKREMOVED;
            	    InputRequest->io_Command = IND_WRITEEVENT;
            	    InputRequest->io_Data = &InputEvent;
            	    InputRequest->io_Length = sizeof (struct InputEvent);
                    /* DoIO() will break Forbid() state but this doesn't matter for us any more */
            	    DoIO ((struct IORequest *) InputRequest);
         	    CloseDevice ((struct IORequest *) InputRequest);
                }
                DeleteIORequest ((struct IORequest *)InputRequest);
            }
            DeleteMsgPort (InputPort);
        }
    }

    /* Huh... Let's hope everything is OK. Release multitasking, free resources and return. R.I.P. */
    UnLockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_WRITE);
    Permit();
    FreeArgs(Args);
    CloseLibrary((struct Library *)DOSBase);
    return RETURN_OK;
}

