/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    The shell program.
*/

/******************************************************************************

    NAME

        Shell

    SYNOPSIS

        COMMAND/K/F,FROM

    LOCATION

        C:

    FUNCTION

        Start a shell (interactive or background).

    INPUTS

        COMMAND  --  command line to execute

	FROM     --  script to invoke before user interaction


    RESULT

    HISTORY

    Jul 2010 - improved handling of $ and `: things like cd SYS:Olle/$pelle
               work now. Non-alphanumerical var-names must be enclosed in
               braces.

    Feb 2008 - initial support for .key/bra/ket/dot/dollar/default.

    EXAMPLE

        Shell FROM S:Startup-Sequence

        Starts a shell and executes the startup script.

    BUGS

    SEE ALSO

    Execute, NewShell

    INTERNALS

    The prompt support is not using SetCurrentDirName() as this function
    has improper limitations. More or less the same goes for GetProgramName().

******************************************************************************/

/* TODO:

  *  Alias [] support
  *  Break support = done. (and +(0L) before execution) -- CreateNewProc()?

 */

#define  DEBUG  0
#define  DEBUG1 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <dos/filesystem.h>
#include <dos/bptr.h>
#include <dos/stdio.h>
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

#define SH_GLOBAL_SYSBASE 1
#define SH_GLOBAL_DOSBASE 1
#include <aros/shcommands.h>

#define SET_HOMEDIR 1

#define  min(a,b)  ((a) < (b)) ? (a) : (b)

#define  COMMANDSTR_LEN  (256 + 2)  /* Maximum length of a 'command' */
#define  FILENAME_LEN    256	    /* Maximum length of a redirection filename */

struct InterpreterState;

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
    BOOL  embedded;             /* True when running an embedded command */

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
    BOOL  script;		/* This command has the script bit set */
    BPTR  scriptLock;
};


#define MAXARGS    32
#define MAXARGLEN  32

/* Template options (copied *and mofified* from Dos_ReadArgs) */
#define REQUIRED 0x80           /* /A */
#define KEYWORD  0x40           /* /K */
#define MULTIPLE 0x20           /* /M */
#define NORMAL   0x00           /* No option */
#define SWITCH   0x01           /* /S, implies /K */
#define TOGGLE   0x02           /* /T, implies /K */
#define NUMERIC  0x04           /* /N */
#define REST     0x08           /* /F */

struct ShellBase {
    char   sb_avBuffer[256];
    char   sb_varBuffer[256];
    char   sb_argBuffer[256];
    struct CommandLineInterface *sb_Cli;
    APTR   sb_DOSBase;
    APTR   sb_SysBase;
    APTR   sb_UtilityBase;
};

struct InterpreterState
{
    BOOL   isBootShell;
    LONG   cliNumber;
    LONG   argcount;
    TEXT   argname[MAXARGS][MAXARGLEN];
    LONG   argnamelen[MAXARGS];
    IPTR   arg[MAXARGS];
    LONG   arglen[MAXARGS];
    IPTR   argdef[MAXARGS];
    LONG   argdeflen[MAXARGS];
    UBYTE  argtype[MAXARGS];

    TEXT   bra, ket, dollar, dot;

    struct RDArgs *rdargs;

    struct InterpreterState *stack;

    struct ShellBase *sb;
};



/* Prototypes */

/* Function: convertLine
 *
 * Action:   Parses a command line and returns a filtered version (removing
 *           redirections, incorporating embedded commands, taking care of
 *           variable references and aliases.
 *
 * Input:    struct CSource          *filtered   --  output command string
 *           struct CSource          *cs         --  input string
 *           struct Redirection      *rd         --  state
 *           struct InterpreterState *is    --  interpreter state
 *
 * Output:   LONG  --  error code or 0 if everything went OK
 */
LONG convertLine(struct CSource *filtered, struct CSource *cs,
		 struct Redirection *rd, struct InterpreterState *is);


/* Function: getCommand
 *
 * Action:
 *
 * Input:    struct CSource          *filtered  --  output buffer
 *           struct CSource          *cs        --  input string
 *           struct Redirection      *rd        --  state
 *           struct InterpreterState *is    --  interpreter state
 *
 * Output:   BOOL --  FALSE if there was some error, TRUE otherwise
 */
BOOL getCommand(struct CSource *filtered, struct CSource *cs,
		struct Redirection *rd, struct InterpreterState *is);



/* Function: executeLine
 *
 * Action:   Execute one line of commands
 *
 * Input:    STRPTR                  command      --  command
 *           STRPTR                  commandArgs  --  arguments of the 'command'
 *           struct Redirection      *rd          --  state
 *           struct InterpreterState *is          --  interpreter state
 *
 * Output:   LONG  --  error code or 0 if everything went OK
 */
LONG executeLine(STRPTR command, STRPTR commandArgs, struct Redirection *rd,
		 struct InterpreterState *is);


/* Function: readLine
 *
 * Action:   Read one line of a stream into a buffer.
 *
 * Input:    struct CommandLine *cl           --  the result will be stored
 *                                                here
 *           BPTR                inputStream  --  stream to read the line from
 *           struct InterpreterState *is      --  interpreter
 *
 * Note:     This routine deals with buffering internally so "infinite" command
 *           lines are supported. You may specify NULL as the cl->line. The
 *           cl->line buffer may be disposed of by calling FreeVec().
 *
 * Output:   BOOL  --  FALSE if error, TRUE if everything went OK
 */
BOOL readLine(struct CommandLine *cl, BPTR inputStream, struct InterpreterState *is);


/* Function: checkLine
 *
 * Action:   Parse a command line and do consistency checks
 *
 * Input:    struct Redirection      *rd  --  state
 *           struct CommandLine      *cl  --  the command line
 *           struct InterpreterState *is  --  interpreter state
 *
 * Output:   LONG --  DOS error code
 */
LONG checkLine(struct Redirection *rd, struct CommandLine *cl,
	       struct InterpreterState *is);


/* Function: releaseFiles
 *
 * Action:   Deallocate file resources used for redirecion and reinstall
 *           standard input and output streams.
 *
 * Input:    struct Redirection *rd  --  state
 *           struct InterpreterState *is  --  interpreter state
 *
 * Output:   --
 */
void releaseFiles(struct Redirection *rd, struct InterpreterState *is);


/* Function: appendString
 *
 * Action:   Add a string to the filtered command line
 *
 * Input:    struct CSource *cs    --  output stream (command line)
 *           STRPTR          from  --  string to append
 *           LONG            size  --  number of chars to copy
 *           struct InterpreterState *is -- interpreter
 *
 * Output:   BOOL  --  success/failure indicator
 */
BOOL appendString(struct CSource *cs, CONST_STRPTR from, LONG size, struct InterpreterState *is);


/* Function: printFlush
 *
 * Action:   Do a formatted print that will instantly be displayed.
 *
 * Input:    STRPTR   fmt   --  format string
 *           ...    ...     --  varagrs
 *
 * Output:   BOOL  --  success/failure indicator
 */
#define printFlush(format...) do {Printf(format); Flush(Output());} while (0)

/* Function: interact
 *
 * Action:   Execute a commandfile and then perform standard shell user
 *           interaction.
 *
 * Input:    struct InterpreterState *is  --  interpreter state
 *
 * Output:   LONG  --  error code
 */
LONG interact(struct InterpreterState *is);


/* Function: loadCommand
 *
 * Action:   Load a command, searching the paths, C: and the resident lists.
 *
 * Input:    STRPTR             commandName  --  the command to load
 *           struct ShellState *ss           --  state
 *           struct InterpreterState *is    -- interpreter
 *
 * Output:   BPTR  --  segment of the loaded command or NULL if there was an
 *                     error
 */
BPTR loadCommand(STRPTR commandName, struct ShellState *ss, struct InterpreterState *is);


/* Function: unloadCommand
 *
 * Action:   Free the resources held by a (loaded) command.
 *
 * Input:    BPTR               commandSeg  --  segment of the program to
 *                                              unload
 *           struct ShellState *ss          --  state
 *           struct InterpreterState *is    -- interpreter
 *
 * Output:   --
 */
void unloadCommand(BPTR commandSeg, struct ShellState *ss, struct InterpreterState *is);


/* Function: Redirection_release
 *
 * Action:   Release resources allocated in the state
 *
 * Input:    struct Redirection *rd  --  state
 *
 * Output:   --
 */
void Redirection_release(struct Redirection *rd, struct InterpreterState *is);


/* Function: Redirection_init
 *
 * Action:   Initialize a state structure
 *
 * Input:    struct Redirection *rd  --  state
 *           struct InterpreterState *is -- interpreter
 *
 * Output:   BOOL  --  success/failure indicator
 */
BOOL Redirection_init(struct Redirection *rd, struct InterpreterState *is);


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
static void setPath(BPTR lock, struct InterpreterState *is);


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
static void printPath(struct InterpreterState *is);


/* Function: printPrompt
 *
 * Action:   Print the prompt to indicate that user input is viable.
 *
 * Input:    struct InterpreterState *is  --  interpreter state
 *
 * Output:   --
 */
static void printPrompt(struct InterpreterState *is);



/*****************************************************************************/


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
BOOL extractEmbeddedCommand(struct CommandLine *cl, struct CSource *fromCs, struct InterpreterState *is);


/* Function: copyEmbedResult
 *
 * Action:   Insert the result of executing an embedded command into the
 *           commandline of the parent command.
 *
 * Input:    struct CSource     *filtered  --  output stream (command line)
 *           struct Redirection *rd        --  state
 *           struct InterpreterState *is   --  interpreter
 *
 * Output:   BOOL  --  success/failure indicator
 */
BOOL copyEmbedResult(struct CSource *filtered, struct Redirection *embedRd, struct InterpreterState *is);


/*****************************************************************************/


void setupResidentCommands(void);

static void initDefaultInterpreterState(struct InterpreterState *is)
{
    int i;

    is->argcount = 0;

    for (i =  0; i < MAXARGS; ++i)
    {
	is->argnamelen[i] = 0;
	is->argname[i][0] = '\0';
	is->arg[i]        = (IPTR)NULL;
	is->arglen[i]     = 0;
	is->argdef[i]     = (IPTR)NULL;
	is->argdeflen[i]  = 0;
	is->argtype[i]    = NORMAL;
    }

    is->bra    = '<';
    is->ket    = '>';
    is->dollar = '$';
    is->dot    = '.';

    is->rdargs = NULL;
    is->stack  = NULL;
}

static LONG pushInterpreterState(struct InterpreterState *is)
{
    struct InterpreterState *tmp_is;
    struct ExecBase *SysBase = is->sb->sb_SysBase;

    tmp_is = (struct InterpreterState *)AllocMem(sizeof(*is), MEMF_LOCAL);

    if (!tmp_is)
	return ERROR_NO_FREE_STORE;

    *tmp_is = *is;
    initDefaultInterpreterState(is);
    is->stack = tmp_is;
    return 0;
}

static void popInterpreterState(struct InterpreterState *is)
{
    struct InterpreterState *tmp_is = is->stack;
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;
    LONG i;

    for (i = 0; i < is->argcount; ++i)
	if (is->argdef[i])
	    FreeMem((APTR)is->argdef[i], is->argdeflen[i] + 1);

    if (is->rdargs)
    {
	FreeDosObject(DOS_RDARGS, is->rdargs);
	is->rdargs = NULL;
    }

    if (tmp_is)
    {
	*is = *tmp_is;
	FreeMem(tmp_is, sizeof(*is));
    }
    else
	initDefaultInterpreterState(is);
}

static LONG getArgumentIdx(struct InterpreterState *is,
			   CONST_STRPTR name, LONG len)
{
    LONG i;
    struct ExecBase *SysBase = is->sb->sb_SysBase;

    for (i = 0; i < is->argcount; ++i)
    {
	if (strncmp(is->argname[i], name, len) == 0)
	   return i;
    }

    if (is->argcount >= MAXARGS)
	return -1;

    i = is->argcount++;
    CopyMem(name, is->argname[i], len);
    is->argname[i][len] = '\0';
    is->argnamelen[i] = len;
    return i;
}

AROS_SH1(Shell, 41.2,
AROS_SHA(STRPTR, ,COMMAND,/F,NULL))
{
    AROS_SHCOMMAND_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    struct ShellBase sb;
    struct InterpreterState is;
    LONG error = RETURN_OK;

    D(bug("Executing shell\n"));

    /* Setup up private data for this instance */
    memset(&sb, 0, sizeof(sb));
    sb.sb_SysBase = SysBase;
    sb.sb_DOSBase = DOSBase;
    sb.sb_Cli = Cli();

    is.sb = &sb;

    setupResidentCommands();

    setPath(BNULL, &is);

    is.cliNumber = me->pr_TaskNum;

    if (strcmp(me->pr_Task.tc_Node.ln_Name, "Boot Shell") == 0)
	is.isBootShell = TRUE;
    else
	is.isBootShell = FALSE;

    initDefaultInterpreterState(&is);

    if (is.isBootShell)
        SetPrompt("%N> ");

    if(SHArg(COMMAND) && SHArg(COMMAND)[0])
    {
	struct Redirection rd;
 	struct CommandLine cl = {(STRPTR)SHArgLine(),
       			         0,
				 strlen(SHArg(COMMAND))};
        
	if(Redirection_init(&rd, &is))
	{
	    D(bug("Running command %s\n", SHArg(COMMAND)));
	    error = checkLine(&rd, &cl, &is);
	    Redirection_release(&rd, &is);
	}
        
	D(bug("Command done\n"));
    }
    else
    {
        error = interact(&is);
    }

    D(bug("Exiting shell, error=%d\n", error));

    return error;

    AROS_SHCOMMAND_EXIT
}

void setupResidentCommands(void)
{


}


/* First we execute the script, then we interact with the user */
LONG interact(struct InterpreterState *is)
{
    LONG  error = 0;
    BOOL  moreLeft = FALSE;
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;
    struct CommandLineInterface *cli = is->sb->sb_Cli;

    if (!cli->cli_Background)
    {
	SetVBuf(Output(), NULL, BUF_FULL, -1);
	if (is->isBootShell)
	{
	    PutStr
	    (
	    	"AROS - The AROS Research Operating System\n"
		"Copyright © 1995-2010, The AROS Development Team. All rights reserved.\n"
		"AROS is licensed under the terms of the AROS Public License (APL),\n"
		"a copy of which you should have received with this distribution.\n"
		"Visit http://www.aros.org/ for more information.\n"
	    );
	}
	else
	{
	    IPTR data[] = {(IPTR)is->cliNumber};

	    VPrintf("New Shell process %ld\n", data);
	}
	SetVBuf(Output(), NULL, BUF_LINE, -1);
    }

    do
    {
 	struct CommandLine cl = { NULL, 0, 0 };
	struct Redirection rd;

	if(Redirection_init(&rd, is))
	{
	    if (cli->cli_Interactive)
	        printPrompt(is);

	    moreLeft = readLine(&cl, cli->cli_CurrentInput, is);
	    error = checkLine(&rd, &cl, is);
	    Redirection_release(&rd, is);
	    FreeVec(cl.line);
	    D(bug("error=%d moreleft=%d interactive=%d background=%d\n", error, moreLeft, cli->cli_Interactive, cli->cli_Background));
	    if (error && !cli->cli_Interactive) {
	    	if (IoErr() == ERROR_BREAK)
		    PrintFault(ERROR_BREAK, "Shell");
	    	moreLeft = FALSE;
	    }
	}

	if (!moreLeft)
	{
	    struct CommandLineInterface *cli = is->sb->sb_Cli;

	    popInterpreterState(is);

	    if (!cli->cli_Interactive)
	    {
		Close(cli->cli_CurrentInput);

	        if (AROS_BSTR_strlen(cli->cli_CommandFile))
		{
	            DeleteFile(AROS_BSTR_ADDR(cli->cli_CommandFile));
		    AROS_BSTR_setstrlen(cli->cli_CommandFile, 0);
		}

		if (!cli->cli_Background)
	        {
		    cli->cli_CurrentInput = cli->cli_StandardInput;
 		    cli->cli_Interactive = TRUE;
		    moreLeft = TRUE;
		    Flush(Output());
		    Flush(Error());
	        }
	    }
	}
    } while(moreLeft);

    if (cli->cli_Interactive)
	printFlush("Process %ld ending\n", is->cliNumber);

    return error;
}


/* Close redirection files and install regular input and output streams */
void releaseFiles(struct Redirection *rd, struct InterpreterState *is)
{
   APTR DOSBase = is->sb->sb_DOSBase;

   if (rd->newIn) Close(rd->newIn);
   rd->newIn = BNULL;

   if (rd->newOut) Close(rd->newOut);
   rd->newOut = BNULL;
}


/* Take care of one command line */
LONG checkLine(struct Redirection *rd, struct CommandLine *cl,
	       struct InterpreterState *is)
{
    /* The allocation is taken care of by appendString */
    struct CSource filtered = { NULL, 0, 0 };
    struct CSource cs       = { cl->line, strlen(cl->line), 0 };
    struct LocalVar *lv;
    LONG result = ERROR_UNKNOWN;
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;
    struct CommandLineInterface *cli = is->sb->sb_Cli;
    
    D(bug("[Shell] checkLine() Calling convertLine(), line = %s\n", cl->line));

    if ((result = convertLine(&filtered, &cs, rd, is)) == 0)
    {
	D(bug("Position %i\n", filtered.CS_CurChr));

	/* End string */
	appendString(&filtered, "\n\0", 2, is);

	/* Consistency checks */
	if(rd->haveOutRD && rd->haveAppRD)
	{
	    PutStr("Cannot combine > with >>\n");
	    result = ERROR_TOO_MANY_LEVELS;
	    goto exit;
	}

	/* Only a comment? */
	if(!rd->haveCommand)
	{
	    result = 0;
	    goto exit;
	}

	/* stegerg: Set redirection to default in/out handles */

	if(rd->haveOutRD)
	{
	    D(bug("Redirecting output to file %s\n", rd->outFileName));

	    rd->newOut = Open(rd->outFileName, MODE_NEWFILE);

	    if(BADDR(rd->newOut) == NULL)
	    {
		result = IoErr();
		PrintFault(IoErr(), rd->outFileName);
		goto exit;
	    }

	    D(bug("Output stream opened\n"));
    	    rd->oldOut = SelectOutput(rd->newOut);
	}

	if(rd->haveAppRD)
	{
	    rd->newOut = Open(rd->outFileName, MODE_READWRITE);

	    if(BADDR(rd->newOut) == NULL)
	    {
		result = IoErr();
		PrintFault(IoErr(), rd->outFileName);
	        goto exit;
	    }

	    Seek(rd->newOut, 0, OFFSET_END);
	    D(bug("Output stream opened (append)\n"));
    	    rd->oldOut = SelectOutput(rd->newOut);
	}

	if(rd->haveInRD)
	{
	    rd->newIn = Open(rd->inFileName, MODE_OLDFILE/*FMF_READ*/);

	    if(BADDR(rd->newIn) == NULL)
	    {
		result = IoErr();
		PrintFault(IoErr(), rd->inFileName);
		goto exit;
	    }

	    D(bug("Input stream opened\n"));
    	    SelectInput(rd->newIn);
	}

        lv = FindVar("echo", LV_VAR);
        /* AmigaDOS's shell is content also with echo being set to anything
           that begins with "on" in order to trigger commands echoing on, 
           it doesn't really have to be set to just "on". */
        /* Embedded command isn't echo'ed, but its result will be integrated
           in final command line, which will be echo'ed if the var is set. */
        if ( (lv != NULL)                              &&
             (lv->lv_Len >= 2)                         &&
             (strncasecmp(lv->lv_Value, "on", 2) == 0) &&
             (!rd->embedded)                              )
        {
            /* Ok, commands echoing is on. */
            /* If a redirection is present, echoing isn't expected to go to
               it. If a script is running, building commandLine allows us
               to show what the command line looks like after arguments
               substitution. */
            BPTR echoOut = ( (rd->haveOutRD) || (rd->haveAppRD) ) ? rd->oldOut : Output();
            if ( cli->cli_Interactive )
                FPuts(echoOut, cl->line);
            else
            {
                STRPTR commandLine = AllocVec(1024, MEMF_ANY);
                snprintf(commandLine, 1024, "%s%s%s%s",
                    rd->commandStr,
                    rd->haveOutRD ? " >" : rd->haveAppRD ? " >>" : "",
                    rd->outFileName,
                    filtered.CS_Buffer);
                FPuts(echoOut, commandLine);
                FreeVec(commandLine);
            }
        }

	D(bug("Calling executeLine()\n"));

	/* OK, we've got a command. Let's execute it! */
	result = executeLine(rd->commandStr, filtered.CS_Buffer, rd, is);

	SelectInput(cli->cli_StandardInput);
	SelectOutput(cli->cli_StandardOutput);
    }
    else
    {
	/* PutStr("Erroneous command line.\n"); */
	PrintFault(result, filtered.CS_Buffer);
    }

exit:
    FreeVec(filtered.CS_Buffer);

    if (cli->cli_Interactive)
    {
        Flush(Output());
        Flush(Error());
    }

    return result;
}

static void substArgs(struct CSource *filtered, CONST_STRPTR s, LONG size,
	              struct InterpreterState *is)
{
    CONST_STRPTR send = s + size;
    LONG i, len;
    struct ExecBase *SysBase = is->sb->sb_SysBase;

    while (s < send)
    {
	if (s[0] == is->bra)
	{
	    char buf[32];

	    ++s;

            for (i = 0; i < is->argcount; ++i)
	    {
		len = is->argnamelen[i];
		if (strncmp(s, is->argname[i], len) == 0)
		{
		    CONST_STRPTR pos = strchr(s + len, is->ket);

		    if (s[len] == is->dollar && !is->arg[i] && pos)
		    {
			/* default argument */
			s += len + 1;
			appendString(filtered, s, pos - s, is);
			s = pos + 1;
			break;
		    }
		    else if ( (s[len] == is->ket) || (s[len] == is->dollar && pos) )
		    {
			/* argument substitution */
			STRPTR arg = NULL;
			UBYTE t = is->argtype[i];

			s = pos + 1;
			if (t & KEYWORD)
			{
			    arg = is->argname[i];
			    len = is->argnamelen[i];
			}
			else if (t & (SWITCH | TOGGLE))
			{
			    arg = is->arg[i] ? "1" : "0";
			    len = 1;
			}
			else if (t & NUMERIC)
			{
			    if (is->arg[i])
			    {
				LONG value;

				if (t & MULTIPLE)
				{
				    LONG **m = (LONG**)is->arg[i];

			    	    bug("[Shell] substArgs: %s -> ",
					is->argname[i]);

				    arg = (STRPTR)1;
				    while (*m)
				    {
					if (arg == (STRPTR)1)
					    arg = 0;
					else
					    appendString(filtered, " ", 1, is);

					value = **m;
					bug("%ld ", value);
			 		len = sprintf(buf, "%ld", value);
					appendString(filtered, buf, len, is);
					++m;
				    }

				    bug("\n");
				    break;
				}
				else
				{
				    value = *(LONG*)is->arg[i];
				    len = sprintf(buf, "%ld", value);
				    arg = buf;
				}
			    }
			    else
			    {
				arg = (STRPTR)is->argdef[i];
				len = is->argdeflen[i];
			    }
			}
			else if (t & MULTIPLE)
			{
			    CONST_STRPTR *m = (CONST_STRPTR*)is->arg[i];

			    bug("[Shell] substArgs: %s -> ", is->argname[i]);
			    if (!m || !*m)
			    {
				arg = (STRPTR)is->argdef[i];
				len = is->argdeflen[i];

				bug("%s\n", arg);

				if (arg > 0)
				    appendString(filtered, arg, len, is);
				break;
			    }

			    arg = (STRPTR)1;
			    while (*m)
			    {
				if (arg == (STRPTR)1)
				    arg = 0;
				else
				    appendString(filtered, " ", 1, is);

				bug("%s ", *m);
				len = strlen(*m);
				appendString(filtered, *m, len, is);

				++m;
			    }
			    bug("\n");

			    break;
			}
			else
			{
			    arg = (STRPTR)is->arg[i];
			    if (!arg)
			    {
				arg = (STRPTR)is->argdef[i];
				len = is->argdeflen[i];
			    }
			    else
				len = is->arglen[i];
			}

			bug("[Shell] substArgs: %s -> %s\n",
			    is->argname[i], arg);

			if (arg > 0)
			    appendString(filtered, arg, len, is);
			break;
		    }
		}
	    }
	    if (is->argcount == i)
		appendString(filtered, s - 1, 1, is);
	}
	else
	    appendString(filtered, s++, 1, is);
    }
}

static BOOL doCommand(struct CSource *cs, struct CSource *filtered,
		      CONST_STRPTR cmd1, LONG l1,
		      CONST_STRPTR cmd2, LONG l2,
		      TEXT *res, LONG *error,
		      struct InterpreterState *is)
{
    CONST_STRPTR s = cs->CS_Buffer + cs->CS_CurChr + 1;
    CONST_STRPTR start = s;
    LONG len = l1;
    BOOL match = FALSE;

    if (strncasecmp(s, cmd1, l1) == 0)
	match = TRUE;
    else if (cmd2 && strncasecmp(s, cmd2, l2) == 0)
    {
	match = TRUE;
	len = l2;
    }

    if (match)
    {
	s += len;
	while (s[0] == ' ' || s[0] == '\t')
	    ++s;
		
	if (s[0] == '\n' || s[0] == '\0')
	{
	    appendString(filtered, &is->dot, 1, is);
	    appendString(filtered, start, len, is);
	    *error = ERROR_REQUIRED_ARG_MISSING;
	    return TRUE;
	}
	else
	{
	    *res = s[0];
	    cs->CS_CurChr += s - start + 2;
	}

	return TRUE;
    }

    return FALSE;
}


/* The shell has the following semantics when it comes to command lines:
   Redirection (<,>,>>) may be written anywhere (except before the command
   itself); the following item (as defined by ReadItem() is the redirection
   file. The first item of the command line is the command to be executed.
   This may be an alias, that is there is a Local LV_ALIAS variable that
   should be substituted for the command text. Aliasing only applies to
   commands and not to options, for instance. Variables (set by SetEnv or Set)
   may be referenced by prepending a '$' to the variable name. */

LONG convertLine(struct CSource *filtered, struct CSource *cs,
		 struct Redirection *rd, struct InterpreterState *is)
{

#define item       cookingCS.CS_Buffer[cookingCS.CS_CurChr]
#define from       cookingCS.CS_Buffer
#define advance(x) cookingCS.CS_CurChr += x;

    LONG   result, i, len;
    char   buf[32];
    BOOL   foundOpeningBrace = FALSE,
           foundClosingBrace = FALSE,
           doNotExpandVar    = FALSE;
    STRPTR varNameString     = NULL;
    struct CSource cookingCS = {NULL, 0, 0};
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;
    struct CommandLineInterface *cli = is->sb->sb_Cli;
    BOOL   isQuoted  = FALSE;
    
    appendString(&cookingCS, cs->CS_Buffer, cs->CS_Length + 1, is);
    cookingCS.CS_CurChr = cs->CS_CurChr;
    cookingCS.CS_Length = cs->CS_Length;

    /*
       Vars and BackTicks can't be properly handled by using FindItem() as
       it wouldn't find them when they aren't surrounded with blanck spaces,
       so we handle them ourselves here. Environment variables are always
       referenced by prepending a '$' to their name, it's only scripts
       argument variables that can be referenced by prepending a modified
       (.dollar) sign. Environment variable names containing non-alpha-
       numerical characters must be surrounded with braces ( ${_} ).
        CLI number substitution <$$> handles .dollar and .bra and .ket
       signs subtitution.
        <$$> and Variables and BackTicks need to be handled only once per
       line, but in right order: Commodore's DPAT script builds an
       environment variable per script used, by including the current CLi's
       number in the variable name: $qw{$$} so we must first handle CLI
       number substitution, then extract variables, and then handle 
       BackTicks nested commands.
     */

    if ( cookingCS.CS_CurChr == 0 )
    {

        /* <$$> CLI number substitution */
        while ( cookingCS.CS_CurChr < cookingCS.CS_Length )
        {

            if ( (item == '\0') || (!isQuoted && item == ';') || (item == '\n') )
            {
                break;
            }

            if (item == '"')
            	isQuoted = !isQuoted;

	    if ( (item == is->bra)                                            &&
                 (cookingCS.CS_Buffer[cookingCS.CS_CurChr + 1] == is->dollar) &&
                 (cookingCS.CS_Buffer[cookingCS.CS_CurChr + 2] == is->dollar) &&
                 (cookingCS.CS_Buffer[cookingCS.CS_CurChr + 3] == is->ket)       )
	    {
                len = sprintf(buf, "%ld", is->cliNumber);
                appendString(filtered, buf, len, is);
                advance(4);
	    }
            else
            {
                appendString(filtered, (CONST_STRPTR) &item, 1, is);
                advance(1);
            }
        }

        appendString(filtered, "\0", 1, is);

        if ( filtered->CS_Buffer != NULL )
        {
            FreeVec(cookingCS.CS_Buffer);
            cookingCS.CS_Buffer = filtered->CS_Buffer;
        }

        cookingCS.CS_Length = strlen(cookingCS.CS_Buffer);
        cookingCS.CS_CurChr = 0;
        filtered->CS_Buffer = NULL;
        filtered->CS_CurChr = filtered->CS_Length = 0;
        isQuoted = FALSE;

        /* Environment variables handling (locals and globals) */
        while ( cookingCS.CS_CurChr < cookingCS.CS_Length )
        {

            if ( (item == '\0') || (!isQuoted && item == ';') || (item == '\n') )
            {
                break;
            }

            if (item == '"')
            	isQuoted = !isQuoted;

            if (item == '*')
            {
                if (cookingCS.CS_Buffer[cookingCS.CS_CurChr - 1] == '*')
                {
                    doNotExpandVar = !doNotExpandVar;
                }
                else
                {
                    doNotExpandVar = TRUE;
                }
            }

            if (item == '$')
            {
                if (cookingCS.CS_Buffer[cookingCS.CS_CurChr - 1] != '*')
                    doNotExpandVar = FALSE;
            }

            if ( (item != '$') || (doNotExpandVar) )
            {
                appendString(filtered, (CONST_STRPTR) &item, 1, is);
            }
            else
            {

                advance(1);

                if (item == '$')
                {
                    appendString(filtered, (CONST_STRPTR) &cookingCS.CS_Buffer[cookingCS.CS_CurChr - 1], 1, is);
                    appendString(filtered, (CONST_STRPTR) &item, 1, is);
                    advance(1);
                    continue;
                }

                if ( (item == '\0') || (!isQuoted && item == ';') || (item == '\n') )
                {
                    break;
                }

		if (item == '"')
		    isQuoted = !isQuoted;

                if (item == '{')
                {
                    foundOpeningBrace = TRUE;
                    advance(1);
                }

                varNameString = AllocVec( 256, MEMF_ANY );

                for
                (
                    i = 0 ;
                    foundOpeningBrace ? ( (item != '}')  &&
                                          (isQuoted || item != ';') &&
                                          (item != '\n')    ) : isalnum(item) ;
                )
                {
                    varNameString[i++] = item;
                    advance(1);
                }

                varNameString[i] = '\0';

                if (item == '}')
                {
                    foundClosingBrace = TRUE;
                }
                else
                {
                    cookingCS.CS_CurChr--;
                }

                D(bug("[Shell] varNameString = '%s'\n", varNameString));

                if
                (
                    ( foundOpeningBrace == foundClosingBrace )                           &&
                    ( !doNotExpandVar )                                                  &&
                    ( GetVar(varNameString, is->sb->sb_varBuffer, sizeof(is->sb->sb_varBuffer), LV_VAR) != -1)
                )
                {
                    D(bug("[Shell] Real variable! Value = '%s'\n", is->sb->sb_varBuffer));
                    appendString(filtered, is->sb->sb_varBuffer, strlen(is->sb->sb_varBuffer), is);
                }
                else
                {
                    for ( i += (foundOpeningBrace ? 1 : 0), i += (foundClosingBrace ? 1 : 0) ; i-- ; )
                    {
                        cookingCS.CS_CurChr--;
                    }
                    /* We don't reach back the dollar char as it would lead to
                       endless loop, but we put it in the string nevertheless */
                    appendString(filtered, "$", 1, is);
                }

                foundOpeningBrace = foundClosingBrace = FALSE;
                FreeVec(varNameString);
                
            }
            advance(1);

        } // while( cookingCS.CS_CurChr < cookingCS.CS_Length )

        appendString(filtered, "\0", 1, is);
        {
            FreeVec(cookingCS.CS_Buffer);
            cookingCS.CS_Buffer = filtered->CS_Buffer;
        }

        cookingCS.CS_Length = strlen(cookingCS.CS_Buffer);
        cookingCS.CS_CurChr = 0;
        filtered->CS_Buffer = NULL;
        filtered->CS_CurChr = filtered->CS_Length = 0;
        isQuoted = FALSE;

        /* BackTicks handling... we allow several embedded commands
           for now, while original AmigaDOS only allows one per line */
        while ( item != '\0' )
        {

            if (item == '`')
            {
                struct CommandLine embedCl = { NULL, 0, 0 };

                advance(1);

                if(extractEmbeddedCommand(&embedCl, &cookingCS, is))
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
                                             sizeof("9999999999999")   ];
                    struct Redirection embedRd;

                    /* No memory? */
                    if(!Redirection_init(&embedRd, is))
                    {
                        FreeVec(cookingCS.CS_Buffer);
                        return FALSE;
                    }

                    /* Construct temporary output filename */
                    __sprintf(embedOutputFilename, "T:Shell%ld$embed",
                                ((struct Process *)FindTask(NULL))->pr_TaskNum);

                    /* Temporary */
                    strcpy(embedRd.outFileName, embedOutputFilename);

                    embedRd.embedded  = TRUE;   /* So the command won't be echo'ed */
                    embedRd.haveOutRD = TRUE;   /* ` _ ` is an implicit output
                                                   redirection */

                    D(bug("Doing embedded command.\n"));

                    checkLine(&embedRd, &embedCl, is);

                    D(bug("Embedded command done.\n"));

                    copyEmbedResult(filtered, &embedRd, is);

                    Redirection_release(&embedRd, is);

                    /* Now, go on with the original argument string */
                    continue;

                } //if(extractEmbeddedCommand(&embedCl, cs, is))

                /* If this was just "`command", extractEmbeddedCommand will have
                   made sure that the '`' is included in the command name */

            } // if (item = '`')

            appendString(filtered, (CONST_STRPTR) &item, 1, is);
            advance(1);

        } // while ( item != '\0' )

        appendString(filtered, &item, 1, is);

        if ( filtered->CS_Buffer != NULL )
        {
            FreeVec(cookingCS.CS_Buffer);
            cookingCS.CS_Buffer = filtered->CS_Buffer;
        }

        cookingCS.CS_Length = strlen(cookingCS.CS_Buffer);
        cookingCS.CS_CurChr = 0;
        filtered->CS_Buffer = NULL;
        filtered->CS_CurChr = filtered->CS_Length = 0;
        isQuoted = FALSE;

        D(bug("[Shell] BackTicks handling done... cookingCS.CS_Buffer = '%s'\n", cookingCS.CS_Buffer));


    } // if ( cookingCS.CS_CurChr == 0 )
    else
    {
        D(bug("\n[Shell] Vars and BackTicks handling were skipped... cookingCS.CS_Buffer+cookingCS.CS_CurChr = '%s'\n", cookingCS.CS_Buffer+cookingCS.CS_CurChr));
    }

    while(TRUE)
    {
	D(bug("Str: %s\n", cookingCS.CS_Buffer+cookingCS.CS_CurChr));

	while(item == ' ' || item == '\t')
	{
	    TEXT temp[2];

	    temp[0] = item;
	    temp[1] = 0;

	    appendString(filtered, temp, 1, is);
	    advance(1);
	}

	/* Are we done yet? */
	if(item == '\n' || (!isQuoted && item == ';') || item == '\0')
	    break;

	if (item == '"')
	    isQuoted = !isQuoted;

	if(item == '|')
	{
            BPTR pipefhs[2] = {0, 0};
	    int i;
	    struct TagItem tags[] =
    	    {
		{ SYS_Input   , (IPTR) NULL                                     },
		{ SYS_Output  , SYS_DupStream                        	    	},
		{ SYS_Error   , SYS_DupStream                                   },
		{ SYS_Asynch  , TRUE    	    	    	    	    	},
		{ NP_StackSize, cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT },
		{ TAG_DONE    , 0 	    	    	    	    	    	}
    	    };

	    /* Prevent command lines like "Prompt> | Olle echo Oepir" */
	    if(!rd->haveCommand)
	    {
		FreeVec(cookingCS.CS_Buffer);
		return ERROR_ACTION_NOT_KNOWN;
	    }

	    item = 0;

	    /* There must be something after a pipe... */
	    for
	    (
	        i = cookingCS.CS_CurChr + 1;
		cookingCS.CS_Buffer[i] == ' ' || cookingCS.CS_Buffer[i] == '\t';
		i++
	    );

	    if(cookingCS.CS_Buffer[i] == '\n' || (!isQuoted && cookingCS.CS_Buffer[i] == ';') || cookingCS.CS_Buffer[i] == '\0')
	    {
		FreeVec(cookingCS.CS_Buffer);
		SetIoErr(ERROR_LINE_TOO_LONG); /* what kind of error must we report? */
	        return ERROR_LINE_TOO_LONG;
	    }

	    D(bug("commannd = %S\n", &item+1));

	    if(rd->haveOutRD)
	    {
	        if (SystemTagList(&item+1, tags) == -1)
	        {
	            FreeVec(cookingCS.CS_Buffer);
	            return IoErr();
	        }
	    }
	    else
	    {
                if (Pipe("PIPEFS:", &pipefhs[0], &pipefhs[1]) != DOSTRUE)
                {
	            FreeVec(cookingCS.CS_Buffer);
	            return IoErr();
	        }

	        tags[0].ti_Data = (IPTR)pipefhs[0];

	        if (SystemTagList(&item+1, tags) == -1)
		{
		    LONG error = IoErr();

		    Close(pipefhs[0]);
		    Close(pipefhs[1]);
		    
		    FreeVec(cookingCS.CS_Buffer);
		    return error;
		}
		    
		rd->oldOut = SelectOutput(pipefhs[1]);
		rd->newOut = pipefhs[1];
	    }
        }
	else
	if(item == '<')
	{
	    /* Prevent command lines like "Prompt> <Olle type" */
	    if(!rd->haveCommand)
	    {
		FreeVec(cookingCS.CS_Buffer);
		return ERROR_ACTION_NOT_KNOWN;
	    }

	    /* Multiple redirections not allowed */
	    if(rd->haveInRD)
	    {
		FreeVec(cookingCS.CS_Buffer);
		return ERROR_TOO_MANY_LEVELS;
	    }

	    advance(1);
	    result = ReadItem(rd->inFileName, FILENAME_LEN, &cookingCS);

	    D(bug("Found input redirection\n"));

	    if(result == ITEM_ERROR || result == ITEM_NOTHING)
	    {
		FreeVec(cookingCS.CS_Buffer);
		return ERROR_OBJECT_NOT_FOUND;
	    }

	    rd->haveInRD = TRUE;
	}
	else if(item == '>')
	{
	    /* Prevent command lines like "Prompt> >>Olle echo Oepir" */
	    if(!rd->haveCommand)
	    {
		FreeVec(cookingCS.CS_Buffer);
		return ERROR_ACTION_NOT_KNOWN;
	    }

	    advance(1);

	    if(item == '>')
	    {
		/* Multiple redirections not allowed */
		if(rd->haveAppRD)
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return ERROR_TOO_MANY_LEVELS;
		}

		advance(1);
		result = ReadItem(rd->outFileName, FILENAME_LEN, &cookingCS);

		D(bug("Found append redirection\n"));

		if(result == ITEM_ERROR || result == ITEM_NOTHING)
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return ERROR_OBJECT_NOT_FOUND;
		}

		rd->haveAppRD = TRUE;
	    }
	    else
	    {
		/* Multiple redirections not allowed */
		if(rd->haveOutRD)
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return ERROR_TOO_MANY_LEVELS;
		}

		result = ReadItem(rd->outFileName, FILENAME_LEN, &cookingCS);

		D(bug("Found output redirection\n"));

		if(result == ITEM_ERROR || result == ITEM_NOTHING)
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return ERROR_OBJECT_NOT_FOUND;
	        }

		rd->haveOutRD = TRUE;
	    }
	}

	else
	{
	    STRPTR s = &item;

            if (strncmp(s, ".popis", 6) == 0)
	    {
		popInterpreterState(is);
		FreeVec(cookingCS.CS_Buffer);
		return 0;
	    }
	    else if (strncmp(s, ".pushis", 7) == 0)
	    {
		pushInterpreterState(is);
		FreeVec(cookingCS.CS_Buffer);
		return 0;
	    }
	    else if (item == is->dot)
	    {
		LONG error = 0, i, j, len;

		++s;

		if (s[0] == ' ') /* dot comment */
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return 0;
		}
		else if (doCommand(&cookingCS, filtered, "bra ", 4, NULL, 0, 
				   &is->bra, &error, is))
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return error;
		}
		else if (doCommand(&cookingCS, filtered, "ket ", 4, NULL, 0, 
				   &is->ket, &error, is))
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return error;
		}
		else if (doCommand(&cookingCS, filtered, "dollar ", 7, "dol ", 4,
				   &is->dollar, &error, is))
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return error;
		}
		else if (doCommand(&cookingCS, filtered, "dot ", 4, NULL, 0, 
				   &is->ket, &error, is))
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return error;
		}
		else if (strncasecmp(s, "key ", 4) == 0 ||
			 strncasecmp(s, "k ", 2) == 0)
		{
		    /* .key must be the first line of the script */
		    struct RDArgs *rd;

		    len = s[1] == ' ' ? 2 : 4;
		    appendString(filtered, &is->dot, 1, is);
		    appendString(filtered, s, len, is);
		    advance(++len);

		    if (is->rdargs)
		    {
			appendString(filtered, "duplicate", 10, is);
			FreeDosObject(DOS_RDARGS, is->rdargs);
			is->rdargs = NULL;
			FreeVec(cookingCS.CS_Buffer);
			return ERROR_ACTION_NOT_KNOWN;
		    }

		    is->rdargs = AllocDosObject(DOS_RDARGS, NULL);
		    if (is->rdargs)
		    {
			result = ReadItem(is->sb->sb_argBuffer, sizeof(is->sb->sb_argBuffer), &cookingCS);

			if (result == ITEM_ERROR)
			{
			    FreeVec(cookingCS.CS_Buffer);
			    return ERROR_UNKNOWN;
			}

			if (result == ITEM_NOTHING)
			{
			    FreeVec(cookingCS.CS_Buffer);
			    return ERROR_KEY_NEEDS_ARG;
			}

			len = strlen(is->sb->sb_argBuffer);
		        appendString(filtered, is->sb->sb_argBuffer, len + 1, is);

			s = AROS_BSTR_ADDR(cli->cli_CommandName);
			len = AROS_BSTR_strlen(cli->cli_CommandName);
			is->rdargs->RDA_Source.CS_Buffer = s;
			is->rdargs->RDA_Source.CS_Length = len;
			is->rdargs->RDA_Source.CS_CurChr = 0;

			rd = ReadArgs(is->sb->sb_argBuffer, is->arg, is->rdargs);
			if (rd)
			{
			    UBYTE t;

			    s = is->sb->sb_argBuffer;
			    is->argcount = 0;

			    for (i = 0; i < MAXARGS; ++i)
			    {
				len = 0;

				while (s[0] == ' ' || s[0] == '\t')
				    ++s;

				while (s[0] != '/' && s[0] != ',' && s[0] != '\0')
				{
				    ++len;
				    ++s;
				}

				j = getArgumentIdx(is, s - len, len);
				if (j < 0)
				    return ERROR_UNKNOWN;

				t = NORMAL;
				while (s[0] == '/')
				{
				    switch (*++s)
				    {
				    case 'A': t |= REQUIRED; break;
				    case 'K': t |= KEYWORD;  break;
				    case 'M': t |= MULTIPLE; break;
				    case 'S': t |= SWITCH;   break;
				    case 'T': t |= TOGGLE;   break;
				    case 'N': t |= NUMERIC;  break;
				    case 'F': t |= REST;     break;
				    default: return ERROR_UNKNOWN;
				    }
				    ++s;
				}

				is->argtype[j] = t;

				while (s[0] != ',' && s[0] != '\0')
				    ++s;

				if (!(t & (NUMERIC | SWITCH | TOGGLE)))
				    if (is->arg[j])
					is->arglen[j] =
					    strlen((STRPTR)is->arg[j]);

				if (s[0] == '\0')
				    break;
				++s;
			    }

#if DEBUG1
			    kprintf("[Shell] argcount=%d\n", is->argcount);
			    for (i = 0; i < is->argcount; ++i)
			    {
				t = is->argtype[i];
				if (t & (SWITCH | TOGGLE | NUMERIC))
				{
				    kprintf("[Shell] .key[%s]/%02x = %d\n",
					is->argname[i], (int)is->argtype[i],
					is->arg[i]);
				}
				else if (t & MULTIPLE)
				{
				    CONST_STRPTR *m = NULL;
                                    if (is->arg[i])
                                        m = (CONST_STRPTR*)is->arg[i];

				    kprintf("[Shell] .key[%s]/%02x = ",
					is->argname[i], (int)is->argtype[i]);
				    while (m && *m)
				    {
					kprintf("%s ", *m);
					++m;
				    }
				    kprintf("\n");
				}
				else
				{
				    kprintf("[Shell] .key[%s]/%02x = %s\n",
					is->argname[i], (int)is->argtype[i],
					is->arg[i]);
				}
			    }
#endif
			}
			else
			{
			    error = IoErr();
			    D(bug("[Shell] bad args for: %s\n", is->sb->sb_argBuffer));
			    FreeDosObject(DOS_RDARGS, is->rdargs);
			    is->rdargs = NULL;
			    FreeVec(cookingCS.CS_Buffer);
			    return error;
			}

			FreeVec(cookingCS.CS_Buffer);
			return 0;
		    }
		    else
		    {
			FreeVec(cookingCS.CS_Buffer);
			D(bug("[Shell] memory exhausted\n"));
			return ERROR_NO_FREE_STORE;
		    }
		}
		else if (strncasecmp(s, "default ", 8) == 0 ||
			 strncasecmp(s, "def ", 4) == 0)
		{
		    len = s[3] == ' ' ? 5 : 9;
		    advance(len);

		    i = cookingCS.CS_CurChr;
		    result = ReadItem(is->sb->sb_argBuffer, sizeof(is->sb->sb_argBuffer), &cookingCS);

		    if (result == ITEM_UNQUOTED)
		    {
			len = cookingCS.CS_CurChr - i;

			i = getArgumentIdx(is, is->sb->sb_argBuffer, len);
			if (i < 0)
			{
			    FreeVec(cookingCS.CS_Buffer);
			    return ERROR_UNKNOWN;
			}

			advance(1);
			len = cookingCS.CS_CurChr;
		    	result = ReadItem(is->sb->sb_argBuffer, sizeof(is->sb->sb_argBuffer), &cookingCS);
			len = cookingCS.CS_CurChr - len;
			switch (result)
			{
			case ITEM_ERROR:
			    FreeVec(cookingCS.CS_Buffer);
			    return ERROR_UNKNOWN;
			case ITEM_NOTHING:
			    FreeVec(cookingCS.CS_Buffer);
			    return ERROR_REQUIRED_ARG_MISSING;
			default:
			    if (is->argdef[i])
				FreeMem((APTR)is->argdef[i], is->argdeflen[i] + 1);

			    is->argdef[i] = (IPTR)AllocMem(len + 1, MEMF_LOCAL);
			    CopyMem(is->sb->sb_argBuffer, (APTR)is->argdef[i], len);
			    ((STRPTR)is->argdef[i])[len] = '\0';
			    is->argdeflen[i] = len;
			    advance(len);
			    FreeVec(cookingCS.CS_Buffer);
			    return 0;
			}
		    }
		}
	    }

	    /* This is a regular character -- that is, we have a command */
	    if(!rd->haveCommand)
	    {
		D(bug("Found possible command\n"));

    		getCommand(filtered, /*cs*/&cookingCS, rd, is);
	    }
	    else
	    {
		/* Copy argument */
		LONG size = cookingCS.CS_CurChr;

		result = ReadItem(is->sb->sb_argBuffer, sizeof(is->sb->sb_argBuffer), /*cs*/&cookingCS);

		/*??AGR who coded this shit ? */
		if(result == ITEM_ERROR || ITEM_NOTHING)
		{
		    FreeVec(cookingCS.CS_Buffer);
		    return ERROR_UNKNOWN;
		}

		D(bug("\nFound argument %s\n", is->sb->sb_argBuffer));
		substArgs(filtered, from + size, cookingCS.CS_CurChr - size, is);
	    }
	}
    }

    FreeVec(cookingCS.CS_Buffer);

    D(bug("Exiting convertLine()\n"));

    return 0;
}


/***********************************************/

BOOL extractEmbeddedCommand(struct CommandLine *cl, struct CSource *fromCs, struct InterpreterState *is)
{
#if DEBUG
    struct ExecBase *SysBase = is->sb->sb_SysBase;
#endif
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

/* Currently, no error checking is involved */
BOOL copyEmbedResult(struct CSource *filtered, struct Redirection *embedRd, struct InterpreterState *is)
{
    char a = 0;
    APTR DOSBase = is->sb->sb_DOSBase;

    Seek(embedRd->newOut, 0, OFFSET_BEGINNING);

    while(((a = FGetC(embedRd->newOut)) != '\0') && (a != EOF))
    {
        if (a != '\n')
            appendString(filtered, &a, 1, is);
        else
            appendString(filtered, " ", 1, is);
    }

    return TRUE;
}

/***********************************************/


BOOL getCommand(struct CSource *filtered, struct CSource *cs,
		struct Redirection *rd, struct InterpreterState *is)
{
#if DEBUG
    struct ExecBase *SysBase = is->sb->sb_SysBase;
#endif
    LONG  result;
    APTR DOSBase = is->sb->sb_DOSBase;

    rd->haveCommand = TRUE;

    D(bug("Command found!\n"));

    result = ReadItem(rd->commandStr, COMMANDSTR_LEN, cs);

    if(result == ITEM_ERROR || result == ITEM_NOTHING)
	return FALSE;

    /* Is this command an alias? */
    if(GetVar(rd->commandStr, is->sb->sb_avBuffer, sizeof(is->sb->sb_avBuffer),
	      GVF_LOCAL_ONLY | LV_ALIAS) != -1)
    {
	struct CSource aliasCs = { is->sb->sb_avBuffer, sizeof(is->sb->sb_avBuffer), 0 };

	result = ReadItem(rd->commandStr, COMMANDSTR_LEN, &aliasCs);

	D(bug("Found alias! value = %s\n", is->sb->sb_avBuffer));

	if(result == ITEM_ERROR || result == ITEM_NOTHING)
	    return FALSE;

	/* We don't check if the alias was an alias as that might
	   lead to infinite loops (alias Copy Copy) */

	/* Make a recursive call to take care of the rest of the
	   alias string */
	return convertLine(filtered, &aliasCs, rd, is);
    }

    D(bug("Command = %s\n", rd->commandStr));

    return TRUE;
}

#undef item
#undef from
#undef advance

#define __extendSize  512	/* How much to increase buffer if it's full */


BOOL readLine(struct CommandLine *cl, BPTR inputStream, struct InterpreterState *is)
{
    LONG letter; 
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;

    while(TRUE)
    {
	letter = inputStream ? FGetC(inputStream) : EOF;

//	D(bug("Read character %c (%d)\n", letter, letter));

	/* -2 to skip test for boundary for terminating '\n\0' */
	if(cl->position > (cl->size - 2))
	{
	    STRPTR newString = AllocVec(cl->size + __extendSize, MEMF_ANY);

	    D(bug("Allocated new buffer %p\n", newString));

	    if(cl->line  != NULL)
		CopyMem(cl->line, newString, cl->size);

	    cl->size += __extendSize;
	    FreeVec(cl->line);
	    cl->line = newString;
	}

	if(letter == '\n' || letter == EOF)
	{
	    D(bug("Found end of line\n"));
	    break;
	}

	cl->line[cl->position++] = letter;
    }

    /* Terminate the line with a newline and a NULL terminator */
    cl->line[cl->position++] = '\n';
    cl->line[cl->position++] = '\0';

    D(bug("commandline: %s\n", cl->line));

    if(letter == EOF)
	return FALSE;

    return TRUE;
}


/* Currently, there is no error checking involved */
BOOL appendString(struct CSource *cs, CONST_STRPTR fromStr, LONG size, struct InterpreterState *is)
{
    struct ExecBase *SysBase = is->sb->sb_SysBase;

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

void unloadCommand(BPTR commandSeg, struct ShellState *ss, struct InterpreterState *is)
{
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;
    struct CommandLineInterface *cli = is->sb->sb_Cli;

#if SET_HOMEDIR
    if (ss->homeDirChanged)
    {
        UnLock(SetProgramDir(ss->oldHomeDir));
	ss->homeDirChanged = FALSE;
    }
#endif

    if (!cli->cli_Module) return;
    
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
}


BPTR loadCommand(STRPTR commandName, struct ShellState *ss, struct InterpreterState *is)
{
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;
    struct CommandLineInterface *cli = is->sb->sb_Cli;

    BPTR   oldCurDir;
    BPTR   commandSeg = BNULL;
    BPTR  *paths;
    struct Segment *residentSeg;
    BOOL   absolutePath = strpbrk(commandName, "/:") != NULL;
    BPTR   file;

    /* We check the resident lists only if we do not have an absolute path */
    if(!absolutePath)
    {
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
	    if(residentSeg->seg_UC == CMD_INTERNAL || residentSeg->seg_UC >= 0)
	    {
		if(residentSeg->seg_UC >= 0)
		    residentSeg->seg_UC++;

		ss->residentCommand = TRUE;
		Permit();
		D(bug("[Shell] loadCommand() using resident '%s' command\n", commandName));
		return MKBADDR(residentSeg);
	    }
	}

	Permit();
    }

    ss->residentCommand = FALSE;

    D(bug("Trying to load command1: %s\n", commandName));

    oldCurDir = CurrentDir(BNULL);
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
	return BNULL;

        /* Search the command in the path */

	for
	(
	    paths = (BPTR *)BADDR(cli->cli_CommandDir);
	    file == BNULL && paths != NULL;
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
	else
        #endif
	{
	    struct FileInfoBlock fib;
	    if (Examine(file, &fib) && fib.fib_Protection & FIBF_SCRIPT)
	    {
        	commandSeg = LoadSeg("C:Execute");
		if (commandSeg)
		{
		    ss->script = TRUE;
		    ss->scriptLock = Lock(commandName, SHARED_LOCK);
		}
	    }
	    else
		SetIoErr(ERROR_FILE_NOT_OBJECT);
	}

	Close(file);
    }

    CurrentDir(oldCurDir);

    return commandSeg;
}


/* Execute one command */
LONG executeLine(STRPTR command, STRPTR commandArgs, struct Redirection *rd,
		 struct InterpreterState *is)
{
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;
    struct CommandLineInterface *cli = is->sb->sb_Cli;

    BPTR              module;
    LONG              error = 0;
    struct ShellState ss = {FALSE};
    TEXT cmd[4096];

    ss.script = FALSE;

/*
  if ss->residentCommand isn't initialized as FALSE, it's value is rather
  random ( loadCommand doesn't change it ) so unloadCommand almost always
  thinks that last Command was resident, and doesn't do an UnloadSeg...
*/

    D(bug("Trying to load command: %s\nArguments: %s\n", command,
	     commandArgs));

    module = loadCommand(command, &ss, is);

    /* Set command name even if we couldn't load the command to be able to
       report errors correctly */
    SetProgramName(command);

    if(module != BNULL)
    {
	struct Task *me = FindTask(NULL);
	STRPTR oldtaskname = me->tc_Node.ln_Name;
	BOOL  __debug_mem;
	LONG mem_before = 0;
	ULONG sig_before = ((struct Process *)me)->pr_Task.tc_SigAlloc;
	ULONG sig_after;
	BYTE sigbit;
	ULONG sigmask;

	BPTR seglist = ss.residentCommand ? ((struct Segment *)BADDR(module))->seg_Seg:module;

	STRPTR dst = cmd, src;
	LONG len = 0;

        if (ss.script)
	{
	    *dst++ = '"';
	    NameFromLock(ss.scriptLock, dst, sizeof(cmd));
	    while (*dst != '\0')
	    {
		++dst;
		++len;
	    }
	    *dst++ = '"';
	    *dst++ = ' ';
	    UnLock(ss.scriptLock);
	    len += 2;

#if 0
	    error = pushInterpreterState(is);
	    if (error)
	    {
		D(bug("Returned from command %s\n", command));
		unloadCommand(module, &ss, is);
		cli->cli_Result2 = error;
		return RETURN_FAIL;
	    }
#endif
	}

	src = commandArgs;
	if (src[0] == ' ')
	    ++src;

        for (; *src != '\0'; ++dst, ++src, ++len)
            *dst = *src;
	*dst = '\0';

	D(bug("Command loaded: len=%d, args=%s\n", len, cmd));

	SetIoErr(0);        	    	 /* Clear error before we execute this command */
	SetSignal(0, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

	cli->cli_Module = seglist;

	me->tc_Node.ln_Name = command;
	
        __debug_mem = FindVar("__debug_mem", LV_VAR) != NULL;

	if (__debug_mem)
	{
	    FreeVec(AllocVec((ULONG)(~0ul/2), MEMF_ANY)); /* Flush memory */

	    mem_before = AvailMem(MEMF_ANY);
	    Printf("Available total memory before command execution: %10ld\n", mem_before);
	}

	cli->cli_ReturnCode = RunCommand(seglist, cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT,
					 cmd, len);

	/*
	    Check if running the command has changed signal bits of the Shell process.
	    If there is a difference the signals will be set or freed to avoid that
	    the Shell runs out of free signals.
	*/
	sig_after = ((struct Process *)me)->pr_Task.tc_SigAlloc;
	if (sig_before != sig_after)
	{
	    for (sigbit = 0; sigbit < 32; sigbit++)
	    {
		sigmask = 1L << sigbit;
		if ((sig_before & sigmask) && !(sig_after & sigmask))
		{
		    /* Command has deleted signal => set it */
		    Printf("*** Command returned with freed signal 0x%lx\n", sigmask);
		    AllocSignal(sigbit);
		}
		else if ( !(sig_before & sigmask) && (sig_after & sigmask))
		{
		    /* Command has set signal => free it */
		    Printf("*** Command returned with unfreed signal 0x%lx\n", sigmask);
		    FreeSignal(sigbit);
		}
	    }
	}

	if (__debug_mem)
	{
	    LONG mem_after;
	    FreeVec(AllocVec((ULONG)(~0ul/2), MEMF_ANY)); /* Flush memory */

	    mem_after = AvailMem(MEMF_ANY);
	    Printf("Available total memory after command execution:  %10ld\n", mem_after);
	    Printf("Memory difference (before - after):              %10ld\n", mem_before - mem_after);
	}

	me->tc_Node.ln_Name = oldtaskname;

	D(bug("Returned from command %s rc=%d\n", command, cli->cli_ReturnCode));
	unloadCommand(module, &ss, is);

	if (SetSignal(0, 0) & SIGBREAKF_CTRL_D) {
	    SetIoErr(ERROR_BREAK);
	    D(bug("CTRL_D detected\n"));
	    error = RETURN_FAIL;
	}

	cli->cli_Result2 = IoErr();
	if (cli->cli_ReturnCode > 0 && cli->cli_ReturnCode >= cli->cli_FailLevel) {
	    D(bug("Err %d >= FailLevel %d\n", cli->cli_ReturnCode, cli->cli_FailLevel));
	    error = RETURN_FAIL;
	}
    }
    else
    {
	/* Implicit cd? */
        /* SFS returns ERROR_INVALID_COMPONENT_NAME if you try to open "" */
	if(!(rd->haveInRD || rd->haveOutRD || rd->haveAppRD) &&
           (IoErr() == ERROR_OBJECT_WRONG_TYPE || IoErr() == ERROR_OBJECT_NOT_FOUND || IoErr() == ERROR_INVALID_COMPONENT_NAME))
        {
	    BPTR lock = Lock(command, SHARED_LOCK);

	    if(lock != BNULL)
	    {
		struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

		if(fib != NULL)
		{
		    if(Examine(lock, fib))
		    {
			if(fib->fib_DirEntryType > 0)
			{
			    setPath(lock, is);
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
	    PrintFault(IoErr(), command);
        }
    }

    D(bug("Done with the command... error=%d\n", error));

    return error;
}


BOOL Redirection_init(struct Redirection *rd, struct InterpreterState *is)
{
    struct ExecBase *SysBase = is->sb->sb_SysBase;

    bzero(rd, sizeof(struct Redirection));

    rd->commandStr  = AllocVec(COMMANDSTR_LEN, MEMF_CLEAR);

    rd->outFileName = AllocVec(FILENAME_LEN, MEMF_CLEAR);
    rd->inFileName  = AllocVec(FILENAME_LEN, MEMF_CLEAR);

    if(rd->commandStr == NULL || rd->outFileName == NULL ||
       rd->inFileName == NULL)
    {
	Redirection_release(rd, is);
	return FALSE;
    }

    /* Preset the first bytes to "C:" to handle C: multiassigns */
    *rd->commandStr++ = 'C';
    *rd->commandStr++ = ':';

    return TRUE;
}


void Redirection_release(struct Redirection *rd, struct InterpreterState *is)
{
    struct ExecBase *SysBase = is->sb->sb_SysBase;

    /* -2 as we set pointer 2 bytes ahead to be able to use C: as a multi-
       assign in a smooth way */
    FreeVec(rd->commandStr - 2);
    FreeVec(rd->outFileName);
    FreeVec(rd->inFileName);

    releaseFiles(rd, is);
}

static void printPath(struct InterpreterState *is)
{
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    APTR DOSBase = is->sb->sb_DOSBase;

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


static void setPath(BPTR lock, struct InterpreterState *is)
{
    APTR DOSBase = is->sb->sb_DOSBase;
    struct ExecBase *SysBase = is->sb->sb_SysBase;
    BPTR    dir;
    STRPTR  buf;
    ULONG   i;

    if(lock == BNULL)
	dir = CurrentDir(BNULL);
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

    if(lock == BNULL)
	CurrentDir(dir);
}

static void printPrompt(struct InterpreterState *is)
{
    APTR DOSBase = is->sb->sb_DOSBase;
    struct CommandLineInterface *cli = is->sb->sb_Cli;

    BSTR prompt = cli->cli_Prompt;
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
		Printf("%ld", is->cliNumber);
		break;
	    case 'R':
	    case 'r':
		Printf("%ld", cli->cli_ReturnCode);
		break;
	    case 'S':
	    case 's':
		printPath(is);
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

