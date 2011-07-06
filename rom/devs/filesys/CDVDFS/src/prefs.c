#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#include "charset.h"
#include "debug.h"
#include "globals.h"
#include "prefs.h"
#include "aros_stuff.h"

void SAVEDS Prefs_Process (void)
{
    ULONG Sigset;
    struct CDVDBase *global;
    struct MsgPort *mp;
    struct Message *msg;

    mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    WaitPort(mp);
    msg =  GetMsg(mp);
    global = (APTR)msg->mn_Node.ln_Name;

    BUG(dbprintf(global, "Prefs handler process started for CDVDBase %p\n", global);)
    ReplyMsg(msg);

    do
    {
	/*
	 * Init character set translation only from within here
	 * because this can trigger loading some files from disk,
	 * which in turn can trigger new requests to our handler.
	 */
	InitCharset(global);

	/*
	 * TODO:
	 * 1. In future this process will be responsible for reading
	 *    preferences file from disk.
	 * 2. For AROS we need some way to trigger late codesets.library
	 *    initialization. CDVDFS is mounder before SYS: is available.
	 */
			  
	Sigset = Wait(SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_D);
    } while (!(Sigset & SIGBREAKF_CTRL_C));

    Forbid();
    PutMsg(global->Dback,&global->DummyMsg);	      /*  Kill handshake  */
}

static struct TagItem const PrefsProcTags[] = {
	{NP_Entry, (IPTR)Prefs_Process},
	{NP_Name, (IPTR)"CDVDFS prefs monitor"},
#ifdef __mc68000
	{NP_StackSize, 4096},
#endif
#ifdef __MORPHOS__
	{NP_CodeType, CODETYPE_PPC},
#endif
	{ TAG_DONE }
};

void Prefs_Init (struct CDVDBase *global)
{

    global->PrefsProc = (struct Task *)CreateNewProc(PrefsProcTags);
    if (global->PrefsProc) {
      global->DummyMsg.mn_ReplyPort = global->Dback;
      global->DummyMsg.mn_Node.ln_Name = (APTR)global;
      PutMsg(&((struct Process *)global->PrefsProc)->pr_MsgPort, &global->DummyMsg);
      WaitPort(global->Dback);				    /* handshake startup    */
      GetMsg(global->Dback);				    /* remove dummy msg     */
    };
}

void Prefs_Uninit (struct CDVDBase *global)
{
    if (global->PrefsProc)
    {
	Signal(global->PrefsProc, SIGBREAKF_CTRL_C);
	WaitPort(global->Dback);	    /*	He's dead jim!      */
	GetMsg(global->Dback);
	Delay(100);		    /*	ensure he's dead    */
    }
}

