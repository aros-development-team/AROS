#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#include "debug.h"
#include "globals.h"
#include "prefs.h"
#include "aros_stuff.h"

extern struct Globals *global;

#define SysBase global->SysBase
#define DOSBase global->DOSBase

void SAVEDS Prefs_Process (void)
{
    ULONG Sigset;

    BUG(dbprintf("Prefs handler process started\n");)
    PutMsg(global->Dback, &global->DummyMsg);

    do
    {
	/*
	 * Init character set translation only from within here
	 * because this can trigger loading some files from disk,
	 * which in turn can trigger new requests to our handler.
	 */
	InitCharset();

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

struct TagItem PrefsProcTags[] = {
	{NP_Entry, (IPTR)Prefs_Process},
	{NP_Name, (IPTR)"CDVDFS prefs monitor"},
	{NP_StackSize, 4096},
#ifdef __MORPHOS__
	{NP_CodeType, CODETYPE_PPC},
#endif
	TAG_DONE
};

void Prefs_Init (void)
{
    global->PrefsProc = (struct Task *)CreateNewProc(PrefsProcTags);
    if (global->PrefsProc) {
      WaitPort(global->Dback);				    /* handshake startup    */
      GetMsg(global->Dback);				    /* remove dummy msg     */
    };
}

void Prefs_Uninit (void)
{
    MSG killmsg;

    if (global->PrefsProc) {
	Signal(global->PrefsProc, SIGBREAKF_CTRL_C);
	WaitPort(global->Dback);	    /*	He's dead jim!      */
	GetMsg(global->Dback);
	Delay(100);		    /*	ensure he's dead    */
    }
}

