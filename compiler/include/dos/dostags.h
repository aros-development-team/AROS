#ifndef DOS_DOSTAGS_H
#define DOS_DOSTAGS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Tags for DOS routines
    Lang: english
*/
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

/* System() */
#if !(AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
#   define SYS_Dummy	   (DOS_TAGBASE)
#else
#   define SYS_Dummy	   (TAG_USER + 32)
#endif
#define SYS_Input	(SYS_Dummy + 1) /* input filehandle  */
#define SYS_Output	(SYS_Dummy + 2) /* output filehandle */
#define SYS_Asynch	(SYS_Dummy + 3) /* run asynchronous, close I/O on exit */
#define SYS_UserShell	(SYS_Dummy + 4) /* send to user shell instead of boot shell */
#define SYS_CustomShell (SYS_Dummy + 5) /* send to a specific shell (data is name) */

/* CreateNewProc() */
/* One of NP_Seglist or NP_Entry MUST be given. Everything else is optional.
   Defaults are in parenthese. */
#if !(AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
#   define NP_Dummy	   (DOS_TAGBASE + 1000)
#else
#   define NP_Dummy	   (TAG_USER + 1000)
#endif
#define NP_Seglist	(NP_Dummy + 1) /* seglist of code */
#define NP_FreeSeglist	(NP_Dummy + 2) /* free seglist on exit (TRUE) */
#define NP_Entry	(NP_Dummy + 3) /* entry point to run - mutually exclusive
					  with NP_Seglist! */
#define NP_Input	(NP_Dummy + 4) /* filehandle (NIL:) */
#define NP_Output	(NP_Dummy + 5) /* filehandle (NIL:) */
#define NP_CloseInput	(NP_Dummy + 6) /* close input filehandle on exit (TRUE) */
#define NP_CloseOutput	(NP_Dummy + 7) /* close output filehandle on exit (TRUE) */
#define NP_Error	(NP_Dummy + 8) /* filehandle (NIL:) */
#define NP_CloseError	(NP_Dummy + 9) /* close error filehandle on exit (TRUE) */
#define NP_CurrentDir	(NP_Dummy + 10) /* lock (current dir)  */
#define NP_StackSize	(NP_Dummy + 11) /* stacksize for process (>= 4000) */
#define NP_Name 	(NP_Dummy + 12) /* name for process ("New Process") */
#define NP_Priority	(NP_Dummy + 13) /* priority (same as parent) */
#define NP_ConsoleTask	(NP_Dummy + 14) /* consoletask (same as parent) */
#define NP_WindowPtr	(NP_Dummy + 15) /* window ptr (same as parent) */
#define NP_HomeDir	(NP_Dummy + 16) /* home directory  (curr dir) */
#define NP_CopyVars	(NP_Dummy + 17) /* boolean to copy local vars (TRUE) */
#define NP_Cli		(NP_Dummy + 18) /* create cli structure (FALSE) */
#define NP_Path 	(NP_Dummy + 19) /* path (copy of parents path) */
					/* only valid for cli process! */
#define NP_CommandName	(NP_Dummy + 20) /* commandname, valid only for CLI */
#define NP_Arguments	(NP_Dummy + 21)
	    /* cstring of arguments - passed with str in A0, length in D0.  */
	    /* (copied and freed on exit.)  Default is 0-length NULL ptr.   */
	    /* If you use NP_Arguments, NP_Input must be non-NULL.     */

#define NP_NotifyOnDeath (NP_Dummy + 22) /* TODO notify parent on death (FALSE) */
#define NP_Synchronous	(NP_Dummy + 23) /* TODO don't return until process finishes (FALSE) */
#define NP_ExitCode	(NP_Dummy + 24) /* code to be called on process exit */
#define NP_ExitData	(NP_Dummy + 25) /* optional argument for NP_EndCode rtn (NULL) */

/* AROS Extensions */
#define NP_UserData	(NP_Dummy + 26) /* IPTR to put into tc_UserData (NULL) */

/* AllocDosObject() */
#if !(AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
#   define ADO_Dummy	   (DOS_TAGBASE + 2000)
#else
#   define ADO_Dummy	   (TAG_USER + 2000)
#endif
    /* DOS_FILEHANDLE */
#define ADO_FH_Mode	(ADO_Dummy + 1) /* sets up FH to the specified mode. */

	/* If you do not specify these, dos will use it's preferred values */
	/* which may change from release to release.  The BPTRs to these   */
	/* will be set up correctly for you.  Everything will be zero,	   */
	/* except cli_FailLevel (10) and cli_Background (DOSTRUE).         */
	/* NOTE: you may also use these 4 tags with CreateNewProc.	   */

    /* DOS_CLI (May be used in CreateNewProc, too) */
    /* If you don't specify these, the DOS defaults will be used which can
       change from release to release. */
#define ADO_DirLen	(ADO_Dummy + 2) /* size in bytes for current dir buffer */
#define ADO_CommNameLen (ADO_Dummy + 3) /* size in bytes for command name buffer */
#define ADO_CommFileLen (ADO_Dummy + 4) /* size in bytes for command file buffer */
#define ADO_PromptLen	(ADO_Dummy + 5) /* size in bytes for the prompt buffer */

/* NewLoadSeg() */
    /* none yet */

#endif /* DOS_DOSTAGS_H */
