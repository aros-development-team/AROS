/*
    (C) Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: The shell program.
    Lang: English
*/

/******************************************************************************

    NAME

        Shell

    SYNOPSIS

        COMMAND/K/F,FROM

    LOCATION

        Workbench:C

    FUNCTION

        Start a shell (interactive or background).

    INPUTS

        COMMAND  --  command line to execute

	FROM     --  script to invoke before user interaction


    RESULT

    NOTES

    The script file is not a script in execute sense (as you may not use any
    .key, .bra or .ket and similar things).

    EXAMPLE

        shell FROM S:Startup-Sequence

        Starts a shell and executes the startup script.

    BUGS

    SEE ALSO

    Execute, NewShell

    INTERNALS

    The prompt support is not using SetCurrentDirName() as this function
    has improper limitations. More or less the same goes for GetProgramName().

    HISTORY

    2x.12.1999  SDuvan   completely rewritten; alias support, variable
                         support...
    0x.01.2000  SDuvan   support for embedded commands; support for resident
                         commands; C: multiassign capabilities

******************************************************************************/

/* TODO:

 (*) EndCli/EndShell support
  *  Alias [] support
  *  Break support (and SetSignal(0L) before execution) -- CreateNewProc()?
  *  Script file execution capabilities (if script bit set)
  *  $ must be taken care of differently than it is now so that things
     like cd SYS:Olle/$pelle works

 */

/* This is 1, because it is at the moment handled here in the Shell itself.
   Should it turn out that the correct place to do the CHANGE_SIGNAL is
   newshell.c instead, change this define to 0 and in newshell.c set the
   same define to 1. */

#define DO_CHANGE_SIGNAL 1

#define  DEBUG  1
#include <aros/debug.h>

#include <exec/memory.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <dos/rdargs.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <aros/asmcall.h>
#include <unistd.h>
#include <stdarg.h>

#include <aros/debug.h>

extern struct UtilityBase *UtilityBase;

static const char version[] = "$VER: shell 41.6 (" __DATE__ ")\n";

#define SET_HOMEDIR 1

#define  P(x)		/* Debug macro */
#define  P2(x) 		/* Debug macro */


#define  min(a,b)  ((a) < (b)) ? (a) : (b)

#define  COMMANDSTR_LEN  (256 + 2)  /* Maximum length of a 'command' */
#define  FILENAME_LEN    256	    /* Maximum length of a redirection filename */

struct Redirection
{
    BPTR  newIn;
    BPTR  newOut;
    BPTR  oldIn;
    BPTR  oldOut;

    BOOL  haveCommand;
    BOOL  haveOutRD;
    BOOL  haveInRD;
    BOOL  haveAppRD;

    STRPTR  commandStr;		/* The command to execute */
    STRPTR  outFileName;	/* Redirection file for > or >> */
    STRPTR  inFileName;		/* Redirection file for < */
};


struct CommandLine
{
    STRPTR  line;
    LONG    position;
    LONG    size;
};


struct ShellState
{
#if SET_HOMEDIR
    BPTR  oldHomeDir;		/* shared lock on program file's parent directory */
    BOOL  homeDirChanged;
#endif
    BOOL  residentCommand;	/* The last command executed was resident */
};


struct CommandLineInterface *cli;


/* Prototypes */

/* Function: convertLine
 *
 * Action:   Parses a command line and returns a filtered version (removing
 *           redirections, incorporating embedded commands, taking care of
 *           variable references and aliases.
 *
 * Input:    struct CSource     *filtered   --  output command string
 *           struct CSource     *cs         --  input string
 *           struct Redirection *rd         --  state
 *
 * Output:   BOOL --  FALSE if there was some error, TRUE otherwise
 */
BOOL convertLine(struct CSource *filtered, struct CSource *cs,
		 struct Redirection *rd);


/* Function: getCommand
 *
 * Action:
 *
 * Input:    struct CSource     *filtered  --  output buffer
 *           struct CSource     *cs        --  input string
 *           struct Redirection *rd        --  state
 *
 * Output:   BOOL --  FALSE if there was some error, TRUE otherwise
 */
BOOL getCommand(struct CSource *filtered, struct CSource *cs,
		struct Redirection *rd);


/* Function: executeFile
 *
 * Action:   Run all commands found in a command file.
 *
 * Input:    STRPTR   fileName  --  the name of the file containing the
 *                                  commands
 *
 * Output:   LONG  --  error code or 0 if everything went OK
 */
LONG executeFile(STRPTR fileName);


/* Function: executeLine
 *
 * Action:   Execute one line of commands
 *
 * Input:    STRPTR              command      --  command
 *           STRPTR              commandArgs  --  arguments of the 'command'
 *           struct Redirection *rd           --  state
 *
 * Output:   LONG  --  error code or 0 if everything went OK
 */
LONG executeLine(STRPTR command, STRPTR commandArgs, struct Redirection *rd);


/* Function: readLine
 *
 * Action:   Read one line of a stream into a buffer.
 *
 * Input:    struct CommandLine *cl           --  the result will be stored
 *                                                here
 *           BPTR                inputStream  --  stream to read the line from
 *
 * Note:     This routine deals with buffering internally so "infinite" command
 *           lines are supported. You may specify NULL as the cl->line. The
 *           cl->line buffer may be disposed of by calling FreeVec().
 *
 * Output:   BOOL  --  FALSE if error, TRUE if everything went OK
 */
BOOL readLine(struct CommandLine *cl, BPTR inputStream);


/* Function: checkLine
 *
 * Action:   Parse a command line and do consistency checks
 *
 * Input:    struct Redirection *rd  --  state
 *           struct CommandLine *cl  --  the command line
 *
 * Output:   BOOL  --  FALSE if error, TRUE if everything went OK
 */
BOOL checkLine(struct Redirection *rd, struct CommandLine *cl);


/* Function: releaseFiles
 *
 * Action:   Deallocate file resources used for redirecion and reinstall
 *           standard input and output streams.
 *
 * Input:    struct Redirection *rd  --  state
 *
 * Output:   --
 */
void releaseFiles(struct Redirection *rd);


/* Function: extractEmbeddedCommand
 *
 * Action:   Check if an item beginning with ' is an embedded command and if
 *           that is the case, extract the embedded command line and update
 *           the state of the input stream accordningly.
 *
 * Input:    struct CommandLine *cl  --  command line for embedded command
 *                                       (the result will be stored here)
 *           struct CSource     *cs  --  input stream
 *
 * Output:   --
 */
BOOL extractEmbeddedCommand(struct CommandLine *cl, struct CSource *fromCs);


/* Function: copyEmbedResult
 *
 * Action:   Insert the result of executing an embedded command into the
 *           commandline of the parent command.
 *
 * Input:    struct CSource     *filtered  --  output stream (command line)
 *           struct Redirection *rd        --  state
 *
 * Output:   BOOL  --  success/failure indicator
 */
BOOL copyEmbedResult(struct CSource *filtered, struct Redirection *embedRd);


/* Function: appendString
 *
 * Action:   Add a string to the filtered command line
 *
 * Input:    struct CSource *cs    --  output stream (command line)
 *           STRPTR          from  --  string to append
 *           LONG            size  --  number of chars to copy
 *
 * Output:   BOOL  --  success/failure indicator
 */
BOOL appendString(struct CSource *cs, STRPTR from, LONG size);


/* Function: printFlush
 *
 * Action:   Do a formatted print that will instantly be displayed.
 *
 * Input:    STRPTR   fmt   --  format string
 *           ...    ...     --  varagrs
 *
 * Output:   BOOL  --  success/failure indicator
 */
#define printFlush(format...) {PrintF(format); Flush(Output());}

/* Function: interact
 *
 * Action:   Execute a commandfile and then perform standard shell user
 *           interaction.
 *
 * Input:    STRPTR   script  --  command file to execute before interacting
 *                                (may be NULL)
 *
 * Output:   LONG  --  error code
 */
LONG interact(STRPTR script);


/* Function: loadCommand
 *
 * Action:   Load a command, searching the paths, C: and the resident lists.
 *
 * Input:    STRPTR             commandName  --  the command to load
 *           struct ShellState *ss           --  state
 *
 * Output:   BPTR  --  segment of the loaded command or NULL if there was an
 *                     error
 */
BPTR loadCommand(STRPTR commandName, struct ShellState *ss);


/* Function: unloadCommand
 *
 * Action:   Free the resources held by a (loaded) command.
 *
 * Input:    BPTR               commandSeg  --  segment of the program to
 *                                              unload
 *           struct ShellState *ss          --  state
 *
 * Output:   --
 */
void unloadCommand(BPTR commandSeg, struct ShellState *ss);


/* Function: Redirection_release
 *
 * Action:   Release resources allocated in the state
 *
 * Input:    struct Redirection *rd  --  state
 *
 * Output:   --
 */
void Redirection_release(struct Redirection *rd);


/* Function: Redirection_init
 *
 * Action:   Initialize a state structure
 *
 * Input:    struct Redirection *rd  --  state
 *
 * Output:   BOOL  --  success/failure indicator
 */
BOOL Redirection_init(struct Redirection *rd);


/* Function: setPath
 *
 * Action:   Set the current command (standard) path.
 *
 * Input:    BPTR lock  --  a lock on the directory
 *
 * Notes:    This will set the current directory name via
 *           SetCurrentDirName() eventhough this is not used later.
 *
 * Output:   --
 */
static void setPath(BPTR lock);


/* Function: printPath
 *
 * Action:   Print the current command path to Output().
 *
 * Input:    --
 *
 * Notes:    Used for Prompt purposes.
 *
 * Output:   --
 */
static void printPath(void);


/* Function: printPrompt
 *
 * Action:   Print the prompt to indicate that user input is viable.
 *
 * Input:    --
 *
 * Output:   --
 */
static void printPrompt(void);


/*****************************************************************************/


static void printPath(void)
{
    STRPTR  buf;
    ULONG   i;

    for(i = 256; ; i += 256)
    {
	buf = AllocVec(i, MEMF_ANY);

	if(buf == NULL)
	    break;

	if(GetCurrentDirName(buf, i) == DOSTRUE)
	{
	    FPuts(Output(), buf);
	    FreeVec(buf);
	    break;
	}

	FreeVec(buf);

	if(IoErr() != ERROR_OBJECT_TOO_LARGE)
	    break;
    }
}


static void setPath(BPTR lock)
{
    BPTR    dir;
    STRPTR  buf;
    ULONG   i;

    if(lock == NULL)
	dir = CurrentDir(NULL);
    else
	dir = lock;

    for(i = 256; ; i += 256)
    {
	buf = AllocVec(i, MEMF_ANY);

	if(buf == NULL)
	    break;

	if(NameFromLock(dir, buf, i))
	{
	    SetCurrentDirName(buf);
	    FreeVec(buf);
	    break;
	}

	FreeVec(buf);
    }

    if(lock == NULL)
	CurrentDir(dir);
}


#define PROCESS(x) ((struct Process *)(x))

static void PrintF(char *format, ...)
{
    va_list args;
    va_start(args, format);

    VPrintf(format, args);

    va_end(args);
}


static void printPrompt(void)
{
    BSTR prompt = Cli()->cli_Prompt;
    LONG length = AROS_BSTR_strlen(prompt);
    ULONG i;

    for(i = 0; i < length; i++)
    {
	if(AROS_BSTR_getchar(prompt, i) == '%')
	{
	    i++;

	    if(i == length)
		break;

	    switch(AROS_BSTR_getchar(prompt, i))
	    {
	    case 'N':
	    case 'n':
		PrintF("%ld", PROCESS(FindTask(NULL))->pr_TaskNum);
		break;
	    case 'R':
	    case 'r':
		PrintF("%ld", Cli()->cli_ReturnCode);
		break;
	    case 'S':
	    case 's':
		printPath();
		break;
	    default:
		FPutC(Output(), '%');
		FPutC(Output(), AROS_BSTR_getchar(prompt, i));
		break;
	    }
	}
	else
	    FPutC(Output(), AROS_BSTR_getchar(prompt, i));
    }

    Flush(Output());
}

#if DO_CHANGE_SIGNAL

static void changeSignalTo(BPTR filehandle, struct Task *task)
{
    struct FileHandle *fh;
    struct IOFileSys  iofs;

    if (filehandle)
    {
    	fh = (struct FileHandle *)BADDR(filehandle);

	iofs.IOFS.io_Message.mn_Node.ln_Type   = NT_REPLYMSG;
	iofs.IOFS.io_Message.mn_ReplyPort      = &((struct Process *)task)->pr_MsgPort;
	iofs.IOFS.io_Message.mn_Length         = sizeof(struct IOFileSys);
	iofs.IOFS.io_Command                   = FSA_CHANGE_SIGNAL;
	iofs.IOFS.io_Flags                     = 0;

	iofs.IOFS.io_Device    	    	       = fh->fh_Device;
	iofs.IOFS.io_Unit      	    	       = fh->fh_Unit;
	iofs.io_Union.io_CHANGE_SIGNAL.io_Task = task;

	DoIO(&iofs.IOFS);

    }
}

#endif

enum { ARG_FROM = 0, ARG_COMMAND, NOOFARGS };

struct RDArgs *rda;

int __nocommandline = 1;

void setupResidentCommands(void);

int main(void)
{
    STRPTR         args[NOOFARGS] = { "S:Shell-Startup", NULL };
    LONG           error          = RETURN_OK;

    P(kprintf("Executing shell\n"));

    setupResidentCommands();

    cli = Cli();
    cli->cli_StandardInput  = cli->cli_CurrentInput  = Input();
    cli->cli_StandardOutput = cli->cli_CurrentOutput = Output();
    setPath(NULL);

#if DO_CHANGE_SIGNAL
    changeSignalTo(cli->cli_StandardInput, FindTask(NULL));
    changeSignalTo(cli->cli_StandardOutput, FindTask(NULL));
#endif

    rda = ReadArgs("FROM,COMMAND/K/F", (IPTR *)args, NULL);

    if(rda != NULL)
    {
	if(args[ARG_COMMAND] != NULL)
	{
	    struct Redirection rd;
	    struct CommandLine cl = {(STRPTR)args[ARG_COMMAND],
		  		     strlen((STRPTR)args[ARG_COMMAND]),
				     0};

	    if(Redirection_init(&rd))
	    {
		cli->cli_Interactive = DOSFALSE;
		cli->cli_Background  = DOSTRUE;
		P(kprintf("Running command %s\n",
		          (STRPTR)args[ARG_COMMAND]));
		error = checkLine(&rd, &cl);
		Redirection_release(&rd);
	    }

	    P(kprintf("Command done\n"));
	 }
	 else
	 {
	    error = interact((STRPTR)args[ARG_FROM]);
	    kprintf("error = %d\n");
	 }

	 FreeArgs(rda);
    }
    else
    {
	PrintFault(IoErr(), "Shell");
	error = RETURN_FAIL;
    }

    kprintf("Exiting shell\n");
    P(kprintf("USERDATA 2 = %p\n", FindTask(0)->tc_UserData));

    return error;
}


void setupResidentCommands(void)
{


}


/* First we execute the script, then we interact with the user */
LONG interact(STRPTR script)
{
    ULONG cliNumber = PROCESS(FindTask(NULL))->pr_TaskNum;
    LONG  error = 0;
    BOOL  moreLeft = FALSE;

    if (stricmp(script, "S:Startup-Sequence") != 0)
    	printFlush("New Shell process %ld\n", cliNumber);

    cli->cli_Interactive = DOSTRUE;
    cli->cli_Background  = DOSFALSE;

    P(kprintf("Calling executeFile()\n"));

    executeFile(script);

    P(kprintf("User interaction\n"));

    /* Reset standard failure level */
    cli->cli_FailLevel = RETURN_ERROR;
    SelectInput(cli->cli_StandardInput);

    /* Reset cli_CurrentInput after the execution of the file. This
       marks the fact that we've now entered interactive mode. */
    cli->cli_CurrentInput = cli->cli_StandardInput;

    P(kprintf("Input now comes from the terminal.\n"));

    do
    {
	struct CommandLine cl = { NULL, 0, 0 };
	struct Redirection rd;

	if(Redirection_init(&rd))
	{
	    printPrompt();

	    moreLeft = readLine(&cl, Input());
	    error = checkLine(&rd, &cl);

	    Redirection_release(&rd);
	    FreeVec(cl.line);
	}

    } while(moreLeft);

    printFlush("Process %ld ending\n", cliNumber);

    return error;
}


/* Close redirection files and install regular input and output streams */
void releaseFiles(struct Redirection *rd)
{
#if 1 /* stegerg */
    if (rd->oldIn)
    {
    	SelectInput(rd->oldIn);
	if (rd->newIn) Close(rd->newIn);

	rd->oldIn = rd->newIn = NULL;
    }

    if (rd->oldOut)
    {
    	SelectOutput(rd->oldOut);
	if (rd->newOut) Close(rd->newOut);

	rd->oldOut = rd->newOut = NULL;
    }
#else
    if(rd->newIn != NULL)
    {
	Close(SelectInput(rd->oldIn));
	rd->newIn = NULL;
    }

    if(rd->newOut != NULL)
    {
	Close(SelectOutput(rd->oldOut));
	rd->newOut = NULL;
    }
#endif
}

char avBuffer[256];
char varBuffer[256];
char argBuffer[256];


/* Take care of one command line */
BOOL checkLine(struct Redirection *rd, struct CommandLine *cl)
{
    /* The allocation is taken care of by appendString */
    struct CSource filtered = { NULL, 0, 0 };

    struct CSource cs = { cl->line, strlen(cl->line), 0 };

    BOOL           result = FALSE;

    P(kprintf("Calling convertLine(), line = %s\n", cl->line));

    if(convertLine(&filtered, &cs, rd))
    {
	P2(kprintf("Position %i\n", filtered.CS_CurChr));

	/* End string */
	appendString(&filtered, "\n\0", 2);

	/* Consistency checks */
	if(rd->haveOutRD && rd->haveAppRD)
	{
	    PutStr("Cannot combine > with >>\n");
	    goto exit;
	}

	/* Only a comment? */
	if(!rd->haveCommand)
	{
	    result = TRUE;
	    goto exit;
	}

	/* stegerg: Set redirection to default in/out handles */

	rd->oldIn  = SelectInput(cli->cli_StandardInput);
	rd->oldOut = SelectOutput(cli->cli_StandardOutput);

	if(rd->haveOutRD)
	{
	    P(kprintf("Redirecting output to file %s\n", rd->outFileName));

	    rd->newOut = Open(rd->outFileName, MODE_NEWFILE /*FMF_WRITE|FMF_CREATE*/);

	    P(kprintf("Output stream opened\n"));

	    if(BADDR(rd->newOut) == NULL)
	    {
		goto exit;
	    }


#if 1 /* stegerg */
    	    SelectOutput(rd->newOut);
#else
	    cli->cli_CurrentOutput = rd->newOut;
	    rd->oldOut = SelectOutput(rd->newOut);
#endif
	}

	if(rd->haveAppRD)
	{
	    rd->newOut = Open(rd->outFileName, FMF_MODE_OLDFILE | FMF_CREATE | FMF_APPEND & ~FMF_AMIGADOS);

	    if(BADDR(rd->newOut) == NULL)
	    {
	        goto exit;
	    }

#if 1 /* stegerg */
    	    SelectOutput(rd->newOut);
#else
	    cli->cli_CurrentOutput = rd->newOut;
	    rd->oldOut = SelectOutput(rd->newOut);
#endif
	}

	if(rd->haveInRD)
	{
	    rd->newIn = Open(rd->inFileName, MODE_OLDFILE/*FMF_READ*/);

	    if(BADDR(rd->newIn) == NULL)
	    {
	       goto exit;
	    }

#if 1 /* stegerg */
    	    SelectInput(rd->newIn);
#else
	    cli->cli_CurrentInput = rd->newIn;
	    rd->oldIn = SelectInput(rd->newIn);
#endif
	}

	P(kprintf("Calling executeLine()\n"));

	/* OK, we've got a command. Let's execute it! */
	executeLine(rd->commandStr, filtered.CS_Buffer, rd);
	
	result = TRUE;
    }
    else
    {
	PutStr("Erroneous command line.\n");
    }

exit:
    FreeVec(filtered.CS_Buffer);

    return result;
}


/* The shell has the following semantics when it comes to command lines:
   Redirection (<,>,>>) may be written anywhere (except before the command
   itself); the following item (as defined by ReadItem() is the redirection
   file. The first item of the command line is the command to be executed.
   This may be an alias, that is there is a Local LV_ALIAS variable that
   should be substituted for the command text. Aliasing only applies to
   commands and not to options, for instance. Variables (set by SetEnv or Set)
   may be referenced by prepending a '$' to the variable name. */

BOOL convertLine(struct CSource *filtered, struct CSource *cs,
		 struct Redirection *rd)
{

#define item       cs->CS_Buffer[cs->CS_CurChr]
#define from       cs->CS_Buffer
#define advance(x) cs->CS_CurChr += x;

    LONG result;

    while(TRUE)
    {
	P(kprintf("Str: %s\n", cs->CS_Buffer+cs->CS_CurChr));

	while(item == ' ' || item == '\t')
	{
	    STRPTR temp = " ";

	    temp[0] = item;

	    appendString(filtered, temp, 1);
	    advance(1);
	}

	/* Are we done yet? */
	if(item == '\n' || item == ';' || item == '\0')
	    break;

	if(item == '<')
	{
	    /* Prevent command lines like "Prompt> <Olle type" */
	    if(!rd->haveCommand)
		return FALSE;

	    /* Multiple redirections not allowed */
	    if(rd->haveInRD)
		return FALSE;

	    advance(1);
	    result = ReadItem(rd->inFileName, FILENAME_LEN, cs);

	    P(kprintf("Found input redirection\n"));

	    if(result == ITEM_ERROR || result == ITEM_NOTHING)
		return FALSE;

	    rd->haveInRD = TRUE;
	}
	else if(item == '>')
	{
	    /* Prevent command lines like "Prompt> >>Olle echo Oepir" */
	    if(!rd->haveCommand)
		return FALSE;

	    advance(1);

	    if(item == '>')
	    {
		/* Multiple redirections not allowed */
		if(rd->haveAppRD)
		    return FALSE;

		advance(1);
		result = ReadItem(rd->outFileName, FILENAME_LEN, cs);

		P(kprintf("Found append redirection\n"));

		if(result == ITEM_ERROR || result == ITEM_NOTHING)
		    return FALSE;

		rd->haveAppRD = TRUE;
	    }
	    else
	    {
		/* Multiple redirections not allowed */
		if(rd->haveOutRD)
		    return FALSE;

		result = ReadItem(rd->outFileName, FILENAME_LEN, cs);

		P(kprintf("Found output redirection\n"));

		if(result == ITEM_ERROR || result == ITEM_NOTHING)
		    return FALSE;

		rd->haveOutRD = TRUE;
	    }
	}
	else if(item == '$')
	    /* Possible environment variable usage */
	{
	    LONG size = cs->CS_CurChr;

	    advance(1);
	    result = ReadItem(avBuffer, sizeof(avBuffer), cs);

	    P(kprintf("Found variable\n"));

	    if(result == ITEM_ERROR || ITEM_NOTHING)
		return FALSE;

	    if
	    (
	        (GetVar(avBuffer, varBuffer, sizeof(varBuffer),
		       GVF_GLOBAL_ONLY | LV_VAR) != -1) &&
		!(varBuffer[0] == '$' && !strcmp(varBuffer+1, avBuffer))
            )
	    {
		struct CSource varCs = { varBuffer, sizeof(varBuffer), 0 };

		P(kprintf("Real variable! Value = %s\n", varBuffer));

		if(convertLine(filtered, &varCs, rd) == FALSE)
		    return FALSE;
	    }
	    else
		/* If this "variable" wasn't defined, we use the '$' as a
		   regular character */
	    {
		P(kprintf("No real variable\n"));

		if(!rd->haveCommand)
		{
		    cs->CS_CurChr = size;
		    getCommand(filtered, cs, rd);
		}
		else
		    appendString(filtered, cs->CS_Buffer + size,
				 cs->CS_CurChr - size);
	    }
	}
	else
	{
	    /* Embedded command? */
	    if(item == '`')
	    {
		struct CommandLine embedCl = { NULL, 0, 0 };

		advance(1);

		P(kprintf("Found possible embedded command.\n"));
		
		if(extractEmbeddedCommand(&embedCl, cs))
		{
		    /* The Amiga shell has severe problems when using
		       redirections in embedded commands so here, the
		       semantics differ somewhat. Unix shells seems to be
		       a little bit sloppy with this, too.
		           If you really wanted to, you could track down
		       uses of > and >> and make them work inside ` `, too,
		       but this seems to be rather much work for little gain. 
		    */

		    char embedOutputFilename[sizeof("T:Shell$embed") + 
					     sizeof("9999999999999")];
		    struct Redirection embedRd;

		    /* No memory? */
		    if(!Redirection_init(&embedRd))
			return FALSE;

		    /* Construct temporary output filename */
		    __sprintf(embedOutputFilename, "T:Shell%ld$embed",
			      PROCESS(FindTask(NULL))->pr_TaskNum);

		    /* Temporary */
		    strcpy(embedRd.outFileName, embedOutputFilename);

		    embedRd.haveOutRD = TRUE;   /* ` _ ` is an implicit output
						   redirection */

		    P(kprintf("Doing embedded command.\n"));

		    checkLine(&embedRd, &embedCl);

		    P(kprintf("Embedded command done.\n"));

		    copyEmbedResult(filtered, &embedRd);

		    Redirection_release(&embedRd);

		    /* Now, go on with the original argument string */
		    continue;
		}

		/* If this was just "`command", extractEmbeddedCommand will
		   have made sure that the '`' is included in the command
		   name */
	    }

	    /* This is a regular character -- that is, we have a command */
	    if(!rd->haveCommand)
	    {
		P(kprintf("Found possible command\n"));

    		getCommand(filtered, cs, rd);
	    }
	    else
	    {
		/* Copy argument */
		LONG size = cs->CS_CurChr;

		// P(kprintf("Checking argument\n"));

		result = ReadItem(argBuffer, sizeof(argBuffer), cs);

		// P(kprintf("Found possible argument\n"));

		if(result == ITEM_ERROR || ITEM_NOTHING)
		    return FALSE;

		appendString(filtered, from + size, cs->CS_CurChr - size);

		P(kprintf("\n"));

		P(kprintf("Found argument %s\n", argBuffer));
	    }
	}
    }

    P(kprintf("Exiting convertLine()\n"));

    return TRUE;
}


BOOL extractEmbeddedCommand(struct CommandLine *cl, struct CSource *fromCs)
{
    LONG  position  = fromCs->CS_CurChr;
    BOOL  foundPrim = FALSE;

    while(position <= fromCs->CS_Length)
    {
	if(fromCs->CS_Buffer[position] == '`')
	{
	    foundPrim = TRUE;
	    break;
	}

	position++;
    }

    if(!foundPrim)
    {
	/* Back input stream to include the preceding ` in the command name */

	D(bug("Found end of embedded command\n"));
	fromCs->CS_CurChr--;
	return FALSE;
    }

    /* Initialize stream data structure for embedded command */
    cl->position = 0;
    cl->size = position - fromCs->CS_CurChr;

    D(bug("Embedded command size = %i\n", cl->size));

    /* Just `` ? */
    if (cl->size == 0)
    {
	return FALSE;
    }

    cl->line = &fromCs->CS_Buffer[fromCs->CS_CurChr];

    /* End string */
    fromCs->CS_Buffer[position] = 0;

    /* Correct the original stream data structure */
    fromCs->CS_CurChr = min(position + 1, fromCs->CS_Length);

    return TRUE;
}


BOOL getCommand(struct CSource *filtered, struct CSource *cs,
		struct Redirection *rd)
{
    LONG  result;

    rd->haveCommand = TRUE;

    P(kprintf("Command found!\n"));

    result = ReadItem(rd->commandStr, COMMANDSTR_LEN, cs);
    
    if(result == ITEM_ERROR || result == ITEM_NOTHING)
	return FALSE;
    
    /* Is this command an alias? */
    if(GetVar(rd->commandStr, avBuffer, sizeof(avBuffer),
	      GVF_LOCAL_ONLY | LV_ALIAS) != -1)
    {
	struct CSource aliasCs = { avBuffer, sizeof(avBuffer), 0 };
	
	result = ReadItem(rd->commandStr, COMMANDSTR_LEN, &aliasCs);

	P(kprintf("Found alias! value = %s\n", avBuffer));
	    
	if(result == ITEM_ERROR || result == ITEM_NOTHING)
	    return FALSE;
	
	/* We don't check if the alias was an alias as that might
	   lead to infinite loops (alias Copy Copy) */
		
	/* Make a recursive call to take care of the rest of the
	   alias string */
	return convertLine(filtered, &aliasCs, rd);
    }

    P(kprintf("Command = %s\n", rd->commandStr));

    return TRUE;
}

#undef item
#undef from
#undef advance

#define __extendSize  512	/* How much to increase buffer if it's full */


BOOL readLine(struct CommandLine *cl, BPTR inputStream)
{
    char letter; 

    while(TRUE)
    {
	letter = FGetC(inputStream);

	P2(kprintf("Read character %c (%d)\n", letter, letter));

	/* -2 to skip test for boundary for terminating '\n\0' */
	if(cl->position > (cl->size - 2))
	{
	    STRPTR newString = AllocVec(cl->size + __extendSize, MEMF_ANY);

	    P2(kprintf("Allocated new buffer %p\n", newString));

	    if(cl->line  != NULL)
		CopyMem(cl->line, newString, cl->size);

	    cl->size += __extendSize;
	    FreeVec(cl->line);
	    cl->line = newString;
	}

	if(letter == '\n' || letter == EOF)
	{
	    P2(kprintf("Found end of line\n"));
	    break;
	}

	cl->line[cl->position++] = letter;
    }

    /* Terminate the line with a newline and a NULL terminator */
    cl->line[cl->position++] = '\n';
    cl->line[cl->position++] = '\0';

    P2(kprintf("commandline: %s\n", cl->line));

    if(letter == EOF)
	return FALSE;

    return TRUE;
}


/* Currently, there is no error checking involved */
BOOL appendString(struct CSource *cs, STRPTR fromStr, LONG size)
{
    /* +2 for additional null bytes, '\n', \0' */
    while(cs->CS_CurChr + size + 2 > (cs->CS_Length - cs->CS_CurChr))
    {
	STRPTR newString = AllocVec(cs->CS_Length + __extendSize, MEMF_ANY);

	CopyMem(cs->CS_Buffer, newString, cs->CS_Length);
	cs->CS_Length += __extendSize;
	FreeVec(cs->CS_Buffer);
	cs->CS_Buffer = newString;
    }

    while(size > 0)
    {
	cs->CS_Buffer[cs->CS_CurChr++] = *fromStr++;
	size--;
    }

    return TRUE;
}



LONG executeFile(STRPTR fileName)
{
    BPTR  scriptFile;		/* Well, this is actually not really a script
				   file, but anyway. */

    scriptFile = Open(fileName, MODE_OLDFILE);

    if(BADDR(scriptFile) != NULL)
    {
	BOOL               moreLeft; /* Script ended? */
	BOOL	    	   breakD;   /* User hit CTRL D? */
	
	struct Redirection rd;

	cli->cli_CurrentInput = scriptFile; /* Set current input for script
					       commands */
	P(kprintf("Loaded script\n"));

    	SetSignal(0, SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);
	
	do
	{
	    struct CommandLine cl = { NULL, 0, 0 };

	    if(!Redirection_init(&rd))
	    	break;

	    moreLeft = readLine(&cl, cli->cli_CurrentInput);

	    P(kprintf("Calling checkLine()\n"));
	    checkLine(&rd, &cl);
	    FreeVec(cl.line);
	    
	    Redirection_release(&rd);

    	    breakD = CheckSignal(SIGBREAKF_CTRL_D);

	} while(moreLeft && !breakD && (cli->cli_ReturnCode < cli->cli_FailLevel));
	
	/* Was there an error encountered in the script file that had a
	   higher fail code than what was specified with FailAt? */
	if(cli->cli_ReturnCode >= cli->cli_FailLevel)
	{
	    /* The interface for GetProgramName() is unbelieveably stupid,
	       so I use the pointer here instead. This ought to be CHANGED
	       in dos.library (to C strings). */
	    IPTR pArgs[] = { (IPTR)cli->cli_CommandName, cli->cli_ReturnCode };

	    VFWritef(Output(), "%T0: failed returncode %N\n", pArgs);
	}

	Close(scriptFile);
    }

    return 0;			/* Temporary */
}


void unloadCommand(BPTR commandSeg, struct ShellState *ss)
{
    if(ss->residentCommand)
    {
	struct Segment *residentSeg = (struct Segment *)BADDR(commandSeg);

	Forbid();

	/* Decrease usecount */
	if(residentSeg->seg_UC > 0)
	    residentSeg->seg_UC--;

	Permit();

	ss->residentCommand = FALSE;
    }
    else
	UnLoadSeg(commandSeg);

#if SET_HOMEDIR
    if (ss->homeDirChanged)
    {
        UnLock(SetProgramDir(ss->oldHomeDir));
	ss->homeDirChanged = FALSE;
    }
#endif
}


BPTR loadCommand(STRPTR commandName, struct ShellState *ss)
{
    BPTR   oldCurDir;
    BPTR   commandSeg = NULL;
    BPTR  *paths;
    struct Segment *residentSeg;
    BOOL   absolutePath = strpbrk(commandName, "/:") != NULL;
    BPTR   file;

    /* We check the resident lists only if we do not have an absolute path */
    if(!absolutePath)
    {
	/* Before checking the resident list, we check if we should we shut
	   down this Shell */
	if(Stricmp("EndCli"  , commandName) == 0 ||
	   Stricmp("EndShell", commandName) == 0)
	{
	    FreeArgs(rda);
	    CloseLibrary((struct Library *)UtilityBase);
	    P(kprintf("Shutting down the shell\n"));

	    /* For now, we don't deal with (closing) redirections or freeing
	       cl.line -- to be able to do this we must return FALSE, as this
	       might be a recursive call of an embedded command */
	    exit(RETURN_OK);
	}

	Forbid();

	/* Check regular list first... */
	residentSeg = FindSegment(commandName, NULL, FALSE);

	if(residentSeg == NULL)
	{
	    /* ... then the system list */
	    residentSeg = FindSegment(commandName, NULL, TRUE);
	}

	if(residentSeg != NULL)
	{
	    /* Can we use this command? */
	    if(residentSeg->seg_UC !=  CMD_DISABLED)
	    {
		if(residentSeg->seg_UC >= 0)
		    residentSeg->seg_UC++;

		ss->residentCommand = TRUE;
		Permit();
		return MKBADDR(residentSeg);
	    }
	}

	Permit();
    }

    ss->residentCommand = FALSE;

    P(kprintf("Trying to load command1: %s\n", commandName));

    oldCurDir = CurrentDir(NULL);
    CurrentDir(oldCurDir);

    file = Open(commandName, MODE_OLDFILE);

    if (!file)
    {
	if
	(
	    absolutePath ||                 /* If this was an absolute path, we don't check the paths set by
	                                       'path' or the C: multiassign */
	    IoErr() == ERROR_OBJECT_IN_USE  /* The object might be exclusively locked */
	)
	return NULL;

        /* Search the command in the path */

	for
	(
	    paths = (BPTR *)BADDR(cli->cli_CommandDir);
	    file == NULL && paths != NULL;
 	    paths = (BPTR *)BADDR(paths[0])    /* Go on with the next path */
	)
	{
	    CurrentDir(paths[1]);
	    file = Open(commandName, MODE_OLDFILE);
	}

	/* The last resort -- the C: multiassign */
	if (!file)
	{
	    commandName-=2;
	    file = Open(commandName, MODE_OLDFILE);
	}
    }

    if (file)
    {
        commandSeg = LoadSeg(commandName);

	#if SET_HOMEDIR
	if (commandSeg)
	{
	    BPTR lock = ParentOfFH(file);

            if (lock)
	    {
	        ss->oldHomeDir = SetProgramDir(lock);
	        ss->homeDirChanged = TRUE;
	    }
	}
        #endif

	Close(file);
    }

    CurrentDir(oldCurDir);

    return commandSeg;
}


/* Execute one command */
LONG executeLine(STRPTR command, STRPTR commandArgs, struct Redirection *rd)
{
    BPTR              module;
    LONG              error = 0;
    struct ShellState ss = {FALSE};
/*
  if ss->residentCommand isn't initialized as FALSE, it's value is rather
  random ( loadCommand doesn't change it ) so unloadCommand almost always
  thinks that last Command was resident, and doesn't do an UnloadSeg...
*/

    P(kprintf("Trying to load command: %s\nArguments: %s\n", command,
	     commandArgs));

    module = loadCommand(command, &ss);

    /* Set command name even if we couldn't load the command to be able to
       report errors correctly */
    SetProgramName(command);

    if(module != NULL)
    {
	BPTR seglist = ss.residentCommand ? ((struct Segment *)BADDR(module))->seg_Seg:module
	P(kprintf("Command loaded!\n"));

	SetIoErr(0);        	    	 /* Clear error before we execute this command */
	SetSignal(0, SIGBREAKF_CTRL_C);

	cli->cli_Module = seglist;
	cli->cli_ReturnCode = RunCommand(seglist, cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT,
					 commandArgs, strlen(commandArgs));

	P(kprintf("Returned from command %s\n", command));
	unloadCommand(module, &ss);

	cli->cli_Result2 = IoErr();
    }
    else
    {
	/* Implicit cd? */
	if(!(rd->haveInRD || rd->haveOutRD || rd->haveAppRD) && (IoErr() == ERROR_OBJECT_WRONG_TYPE || IoErr() == ERROR_OBJECT_NOT_FOUND))
	{
	    BPTR lock = Lock(command, SHARED_LOCK);

	    if(lock != NULL)
	    {
		struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

		if(fib != NULL)
		{
		    if(Examine(lock, fib))
		    {
			if(fib->fib_DirEntryType > 0)
			{
			    setPath(lock);
			    lock = CurrentDir(lock);
			}
			else
			    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		    }

		    FreeDosObject(DOS_FIB, fib);
		}

		/* UnLock the old currentdir */
		UnLock(lock);
	    }
	}

        if(IoErr())
        {
	    cli->cli_Result2 = IoErr();
	    PrintFault(IoErr(), cli->cli_CommandName);
        }
    }

    //    Flush(Output());

    // P(Delay(1*8));

    P(kprintf("Done with the command...\n"));

    return error;
}


/* Currently, no error checking is involved */
BOOL copyEmbedResult(struct CSource *filtered, struct Redirection *embedRd)
{
    char a = 0;

    Seek(embedRd->newOut, 0, OFFSET_BEGINNING);

    while((a = FGetC(embedRd->newOut)) != '\n')
	appendString(filtered, &a, 1);

    return TRUE;
}


BOOL Redirection_init(struct Redirection *rd)
{
    bzero(rd, sizeof(struct Redirection));

    rd->commandStr  = AllocVec(COMMANDSTR_LEN, MEMF_CLEAR);

    rd->outFileName = AllocVec(FILENAME_LEN, MEMF_CLEAR);
    rd->inFileName  = AllocVec(FILENAME_LEN, MEMF_CLEAR);

    if(rd->commandStr == NULL || rd->outFileName == NULL ||
       rd->inFileName == NULL)
    {
	Redirection_release(rd);
	return FALSE;
    }

    /* Preset the first bytes to "C:" to handle C: multiassigns */
    *rd->commandStr++ = 'C';
    *rd->commandStr++ = ':';

    return TRUE;
}


void Redirection_release(struct Redirection *rd)
{
    /* -2 as we set pointer 2 bytes ahead to be able to use C: as a multi-
       assign in a smooth way */
    FreeVec(rd->commandStr - 2);
    FreeVec(rd->outFileName);
    FreeVec(rd->inFileName);

    releaseFiles(rd);
}
