#include <conf.h>

#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_netdb.h>
#include <stdio.h>

extern UBYTE InitFlags;
extern TEXT db_path[];
struct Task * AmiTCP_Task;
UBYTE RcCommand[FILENAME_MAX];
UBYTE RcStartDone = 0;
UBYTE RC_Stopped = 1;

void SAVEDS rc_start_process(void)
{
	BPTR lock, oldcd;
	struct RcentNode *rc;

	lock = Lock(db_path, ACCESS_READ);
	if (lock) {
		oldcd = CurrentDir(lock);
		LOCK_R_NDB(NDB);
		for (rc = (struct RcentNode *)NDB->ndb_Rc.mlh_Head;
		     rc->rn_Node.mln_Succ;
		     rc = (struct RcentNode *)rc->rn_Node.mln_Succ) {
		       sprintf(RcCommand, "%s start", rc->rn_Ent);
		       Execute(RcCommand, NULL, NULL);
/* FIXME: Why doesn't SystemTags() doesn't work here?
		       SystemTags(rc->rn_Ent, NP_Arguments, "start", TAG_DONE);*/
		}
		UNLOCK_NDB(NDB);
		CurrentDir(oldcd);
		UnLock(lock);
	}
	Forbid();
	InternalProc--;
}

void SAVEDS rc_stop_process(void)
{
	BPTR lock, oldcd;
	struct RcentNode *rc;

	lock = Lock(db_path, ACCESS_READ);
	if (lock) {
		oldcd = CurrentDir(lock);
		LOCK_R_NDB(NDB);
		for (rc = (struct RcentNode *)NDB->ndb_Rc.mlh_TailPred;
		     rc->rn_Node.mln_Pred;
		     rc = (struct RcentNode *)rc->rn_Node.mln_Pred) {
		       sprintf(RcCommand, "%s stop", rc->rn_Ent);
		       Execute(RcCommand, NULL, NULL);
/*		       SystemTags(rc->rn_Ent, NP_Arguments, "stop", TAG_DONE);*/
		}
		UNLOCK_NDB(NDB);
		CurrentDir(oldcd);
		UnLock(lock);
	}
	Forbid();
	InternalProc--;
	RC_Stopped = 1;
	Signal(AmiTCP_Task, SIGBREAKF_CTRL_F);
}


void rc_start(void)
{
	InternalProc++;
	if (!(CreateNewProcTags(NP_Entry, (LONG)&rc_start_process,
			  NP_Name, "AROSTCP RC startup",
			  NP_Cli, TRUE,
#ifdef __MORPHOS__
			  NP_CodeType, CODETYPE_PPC,
#endif
			  TAG_DONE)))
		InternalProc--;
	else
		RC_Stopped = 0;
}

void rc_stop(void)
{
	InternalProc++;
	if (!(CreateNewProcTags(NP_Entry, (LONG)&rc_stop_process,
			  NP_Name, "AROSTCP RC shutdown",
			  NP_Cli, TRUE,
#ifdef __MORPHOS__
			  NP_CodeType, CODETYPE_PPC,
#endif
			  TAG_DONE)))
		InternalProc--;
}

