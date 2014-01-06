#ifndef DOS_DOSTAGS_H
#define DOS_DOSTAGS_H

/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tags for DOS routines
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

/**********************************************************************
 ****************************** Processes *****************************
 **********************************************************************/

/* Tags for CreateNewProc(). All tags, where no default is stated, the default
   is inherited from the parent process. Additionally you may use tags for
   AllocDosObject(DOS_CLI, ...). */

#define NP_Dummy        (TAG_USER + 1000)

/* Exactly one of NP_Seglist or NP_Entry must be specified. */
  /* (BPTR) Seglist of code for process. */
#define NP_Seglist	(NP_Dummy + 1)
  /* (BOOL) Free seglist on exit? (Default: TRUE) */
#define NP_FreeSeglist	(NP_Dummy + 2)
  /* (APTR) Entry point for process code. */
#define NP_Entry	(NP_Dummy + 3)

  /* (BPTR/struct FileHandle *) Input filehandle. (Default: NIL:) */
#define NP_Input	(NP_Dummy + 4)
  /* (BPTR/struct FileHandle *) Output filehandle. (Default: NIL:) */
#define NP_Output	(NP_Dummy + 5)
  /* (BOOL) Close input filehandle on exit? (Default: TRUE) */
#define NP_CloseInput	(NP_Dummy + 6)
  /* (BOOL) Close output filehandle in exit? (Default: TRUE) */
#define NP_CloseOutput	(NP_Dummy + 7)
  /* (BPTR/struct FileHandle *) Error filehandle. (Default: NIL:) */
#define NP_Error	(NP_Dummy + 8)
  /* (BOOL) Close error filehandle on exit? (Default: TRUE) */
#define NP_CloseError	(NP_Dummy + 9)

  /* (BPTR/struct FileLock *) Current directory for new task. */
#define NP_CurrentDir	(NP_Dummy + 10)
  /* (ULONG) Stacksize to use for the new process. Default is variable. */
#define NP_StackSize	(NP_Dummy + 11)
  /* (STRPTR) Name for the new process. (Default: "New Process") */
#define NP_Name 	(NP_Dummy + 12)
  /* (LONG) Priority of the new process. */
#define NP_Priority	(NP_Dummy + 13)

  /* (APTR) Pointer to the console task. */
#define NP_ConsoleTask	(NP_Dummy + 14)
  /* (struct Window *) The processes default window. */
#define NP_WindowPtr	(NP_Dummy + 15)
  /* (BPTR/struct FileLock *) The home directory of the new process. This
     defaults to the parents current directory. */
#define NP_HomeDir	(NP_Dummy + 16)
  /* (BOOL) Copy local environment variables? (Default: TRUE) */
#define NP_CopyVars	(NP_Dummy + 17)
  /* (BOOL) Create a CLI structure? (Default: FALSE) */
#define NP_Cli		(NP_Dummy + 18)

/* The following two tags are only valid for CLI processes. */
  /* (APTR) Path for the new process. */
#define NP_Path 	(NP_Dummy + 19)
  /* (STRPTR) Name of the called program. */
#define NP_CommandName	(NP_Dummy + 20)
  /* If this tag is used, NP_Input must not be NULL. */
#define NP_Arguments	(NP_Dummy + 21)

/* The following two tags do not work, yet. */
  /* (BOOL) Notify parent, when process exits? (Default: FALSE) */
#define NP_NotifyOnDeath (NP_Dummy + 22)
  /* (BOOL) Wait until called process returns. (Default: FALSE) */
#define NP_Synchronous	(NP_Dummy + 23)

  /* (APTR) Code that is to be called, when process exits. (Default: NULL) */
#define NP_ExitCode	(NP_Dummy + 24)
  /* (APTR) Optional data for NP_ExitCode. (Default: NULL) */
#define NP_ExitData	(NP_Dummy + 25)

/* The following tags are AROS specific. */
  /* (IPTR) User dependant data. Do with it, what you want to. (Default: NULL)
  */
#define NP_UserData	(NP_Dummy + 26)


/* Tags for SystemTagList(). Additionally you may use all the tags for
   CreateNewProc(). */

#define SYS_Dummy       (TAG_USER + 32)

/* The supplied filehandles are automatically closed, when the new process
   exits, if SYS_Asynch is TRUE. If it is false, they remain opened. */
  /* (BPTR/struct FileHandle *) Input filehandle. This must be a different
     filehandle from that supplied to SYS_Output. Default is Input() of the
     current process. */
#define SYS_Input	(SYS_Dummy + 1)
  /* (BPTR/struct FileHandle *) Output filehandle. This must be a different
     filehandle from than supllied to SYS_Input. Default is Output() of the
     current process. */
#define SYS_Output	(SYS_Dummy + 2)
  /* (BOOL) If FALSE, the execution of calling process is stopped, until the
     new process has finished. */
#define SYS_Asynch	(SYS_Dummy + 3)
/* If neither of the following two tags is specified, the boot-shell is used.
   if SYS_UserShell is set to TRUE then the default user-shell will be used,
   while CustomShell expects a string, which names a new shell. */
#define SYS_UserShell	(SYS_Dummy + 4) /* (BOOL) */
#define SYS_CustomShell (SYS_Dummy + 5) /* (STRPTR) */

  /* The following are AmigaOS4-compatible */

  /* (BPTR/struct FileHandle *) Output filehandle. This must be a different
     filehandle from than supllied to SYS_Input. Default is Error() of the
     current process. */
#define SYS_Error	(SYS_Dummy + 6)

 /* The following are AROS-specific */

 /* (BPTR/struct FileHandle *) This is the FileHandle from which the shell will try to read
     commands to execute right after having executed the command supplied to SystemTagList()
     If not specified it defaults to NIL: */
#define SYS_ScriptInput (SYS_Dummy + 11)


  /* (BOOL) The shell is run as a "background shell", like when the Run command is
     used. If the shell is in background mode, when the EOF is reached on
     SYS_ScriptInput the shell will immediately exit, without trying to read from
     SYS_Input. By default it's set to TRUE. */
#define SYS_Background  (SYS_Dummy + 12)

   /* (LONG *) ti_Data points to a memory location in which SystemTagList will store the
      Cli number of the newly created cli process */
#define SYS_CliNumPtr   (SYS_Dummy + 13)

  /* (LONG) CLI type (see dos/cliinit.h) override. The defaults are:
   *             SYS_Asynch  SYS_Background
   *             ----------  --------------
   * CLI_SYSTEM     FALSE       TRUE
   * CLI_ASYSTEM    TRUE        TRUE
   * CLI_NEWCLI     ---         FALSE
   */
#define SYS_CliType     (SYS_Dummy + 14)

/* This is a *TAG VALUE*, not a tag. Use this together with SYS_Input, SYS_Output and
   SYS_Error, to tell SystemTagList to *duplicate* the respective caller's streams.

   Ie.: a TagItem like this

       { SYS_Input, SYS_DupStream }

   tells SystemTagList to duplicate the caller's Input() filehandle and use it as input file
   handle of the shell.

   The duplicated file handle is automatically closed. */
#define SYS_DupStream   1

/**********************************************************************
 **************************** Miscellaneous ***************************
 **********************************************************************/

/* Tags for AllocDosObject(). */

#define ADO_Dummy	(TAG_USER + 2000)

/* Tags for DOS_FILEHANDLE only. */
  /* Sets up the filehandle for the specified mode. Definitions are in
     <dos/dos.h> */
#define ADO_FH_Mode	(ADO_Dummy + 1) /* Sets up FH to the specified mode. */

/* Tags for DOS_CLI. If you do not specify these, dos will use reasonable
   default values. All these tags specify the buffer length for certain
   strings. */
  /* Length of current directory buffer. */
#define ADO_DirLen	(ADO_Dummy + 2)
  /* Length of command name buffer. */
#define ADO_CommNameLen (ADO_Dummy + 3)
  /* Length of command file buffer. */
#define ADO_CommFileLen (ADO_Dummy + 4)
  /* Length of buffer for CLI prompt. */
#define ADO_PromptLen	(ADO_Dummy + 5)

#endif /* DOS_DOSTAGS_H */
