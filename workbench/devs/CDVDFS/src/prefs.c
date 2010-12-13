#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#include "debug.h"
#include "globals.h"
#include "charset.h"
#include "prefs.h"
#include "aros_stuff.h"

extern struct Globals *global;

#ifdef SysBase
#	undef SysBase
#endif
#define SysBase global->SysBase
#ifdef DOSBase
#	undef DOSBase
#endif
#define DOSBase global->DOSBase

void SAVEDS Prefs_Process (void)
{
    ULONG Sigset;

    BUG(dbprintf("Prefs handler process started\n");)
    PutMsg(global->Dback, &global->DummyMsg);
    do {
	InitUnicodeTable();
	if (global->g_unicodetable_name[0]) {
	    BUG(dbprintf("Loading character set translation table: %s\n", global->g_unicodetable_name);)
	    ReadUnicodeTable(global->g_unicodetable_name);
	}
	    BUG(else dbprintf("Character set conversion table reset to default\n");)
	Sigset = Wait(SIGBREAKF_CTRL_D|SIGBREAKF_CTRL_C);
    } while (!(Sigset & SIGBREAKF_CTRL_C));
    Forbid();
    PutMsg(global->Dback,&global->DummyMsg);	      /*  Kill handshake  */
}

struct TagItem PrefsProcTags[] = {
	{NP_Entry, Prefs_Process},
	{NP_Name, "CDVDFS prefs monitor"},
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

