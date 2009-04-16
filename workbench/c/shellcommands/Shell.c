/*
    Copyright (C) 1995-2009, The AROS Development Team. All rights reserved.
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

    NOTES

    The script file is not a script in execute sense (as you may not use any
    .key, .bra or .ket and similar things).
 
    Feb 2008 - initial support for .key/bra/ket/dot/dollar/default.

    EXAMPLE

        Shell FROM S:Startup-Sequence

        Starts a shell and executes the startup script.

    BUGS

    SEE ALSO

    Execute, NewShell

    INTERNALS

    The prompt	 support is not using SetCurrentDirName() as this function
    has improper limitations. More or less the same goes for GetProgramName().

******************************************************************************/

/* TODO:

  *  Alias [] support
  *  Break support (and +(0L) before execution) -- CreateNewProc()?
  *  Script file execution capabilities (if script bit set)
  *  $ must be taken care of differently than it is now so that things
     like cd SYS:Olle/$pelle works

 */

#define  DEBUG  0
#define  DEBUG1 1
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
    BOOL  script;		/* This command has the script bit set */
    BPTR  scriptLock;
};

struct CommandLineInterface *cli;

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
 *
 * Output:   --
 */
void releaseFiles(struct Redirection *rd);


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
BOOL appendString(struct CSource *cs, CONST_STRPTR from, LONG size);


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
 * Input:    struct InterpreterState *is  --  interpreter state
 *
 * Output:   --
 */
static void printPrompt(struct InterpreterState *is);


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

AROS_SH1(Shell, 41.1,
AROS_SHA(STRPTR, ,COMMAND,/F,NULL))
{
    AROS_SHCOMMAND_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    struct InterpreterState is;
    LONG error = RETURN_OK;

    D(bug("Executing shell\n"));

    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 39);
    if (!UtilityBase) return RETURN_FAIL;

    setupResidentCommands();

    cli = Cli();
    setPath(NULL);

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
 	struct CommandLine cl = {SHArgLine(),
       			         0,
				 strlen(SHArg(COMMAND))};
        
	if(Redirection_init(&rd))
	{
	    D(bug("Running command %s\n", SHArg(COMMAND)));
	    error = checkLine(&rd, &cl, &is);
	    Redirection_release(&rd);
	}
        
	D(bug("Command done\n"));
    }
    else
    {
        error = interact(&is);
    }
    
    D(bug("Exiting shell\n"));

    return error;

    AROS_SHCOMMAND_EXIT
}

struct UtilityBase *UtilityBase;

void setupResidentCommands(void)
{


}


/* First we execute the script, then we interact with the user */
LONG interact(struct InterpreterState *is)
{
    LONG  error = 0;
    BOOL  moreLeft = FALSE;

    if (!cli->cli_Background)
    {
	SetVBuf(Output(), NULL, BUF_FULL, -1);
	if (is->isBootShell)
	{
	    PutStr
	    (
	    	"AROS - The AROS Research Operating System\n"
		"Copyright © 1995-2009, The AROS Development Team. All rights reserved.\n"
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

	if(Redirection_init(&rd))
	{
	    if (cli->cli_Interactive)
	        printPrompt(is);

	    moreLeft = readLine(&cl, cli->cli_CurrentInput);
	    error = checkLine(&rd, &cl, is);

	    Redirection_release(&rd);
	    FreeVec(cl.line);
	}

	if (!moreLeft)
	{
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
void releaseFiles(struct Redirection *rd)
{
   if (rd->newIn) Close(rd->newIn);
   rd->newIn = NULL;

   if (rd->newOut) Close(rd->newOut);
   rd->newOut = NULL;
}

char avBuffer[256];
char varBuffer[256];
char argBuffer[256];


/* Take care of one command line */
LONG checkLine(struct Redirection *rd, struct CommandLine *cl,
	       struct InterpreterState *is)
{
    /* The allocation is taken care of by appendString */
    struct CSource filtered = { NULL, 0, 0 };
    struct CSource cs       = { cl->line, strlen(cl->line), 0 };
    struct LocalVar *lv;
    LONG result = ERROR_UNKNOWN;
    
    lv = FindVar("echo", LV_VAR);
    if (lv != NULL)
    {
	/* AmigaDOS's shell is content also with echo being set to anything
	   that begins with "on" in order to trigger commands echoing on, 
	   it doesn't really have to be set to just "on".  */
	if (lv->lv_Len >= 2)
	    if (strncasecmp(lv->lv_Value, "on", 2) == 0)
	    {
		/* Ok, commands echoing is on.  */
		PutStr(cl->line);
	    }
    }

    D(bug("Calling convertLine(), line = %s\n", cl->line));

    if ((result = convertLine(&filtered, &cs, rd, is)) == 0)
    {
	D(bug("Position %i\n", filtered.CS_CurChr));

	/* End string */
	appendString(&filtered, "\n\0", 2);

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
    	    SelectOutput(rd->newOut);
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
    	    SelectOutput(rd->newOut);
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

    while (s < send)
    {
	if (s[0] == is->bra)
	{
	    char buf[32];

	    ++s;
	    if (s[0] == is->dollar && s[1] == is->dollar && s[2] == is->ket)
	    {
		/* <$$> CLI number substitution */
		s += 3;
		len = sprintf(buf, "%d", is->cliNumber);
		appendString(filtered, buf, len);
	    }
	    else for (i = 0; i < is->argcount; ++i)
	    {
		len = is->argnamelen[i];
		if (strncmp(s, is->argname[i], len) == 0)
		{
		    CONST_STRPTR pos = strchr(s + len, is->ket);

		    if (s[len] == is->dollar && !is->arg[i] && pos)
		    {
			/* default argument */
			s += len + 1;
			appendString(filtered, s, pos - s);
			s = pos + 1;
			break;
		    }
		    else if (s[len] == is->ket || s[len] == is->dollar && pos)
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
					    appendString(filtered, " ", 1);

					value = **m;
					bug("%d ", value);
			 		len = sprintf(buf, "%d", value);
					appendString(filtered, buf, len);
					++m;
				    }

				    bug("\n");
				    break;
				}
				else
				{
				    value = *(LONG*)is->arg[i];
				    len = sprintf(buf, "%d", value);
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
				    appendString(filtered, arg, len);
				break;
			    }

			    arg = (STRPTR)1;
			    while (*m)
			    {
				if (arg == (STRPTR)1)
				    arg = 0;
				else
				    appendString(filtered, " ", 1);

				bug("%s ", *m);
				len = strlen(*m);
				appendString(filtered, *m, len);

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
			    appendString(filtered, arg, len);
			break;
		    }
		}
	    }

	    if (is->argcount == i)
	        appendString(filtered, s - 1, 1);
	}
	else
	    appendString(filtered, s++, 1);
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
	    appendString(filtered, &is->dot, 1);
	    appendString(filtered, start, len);
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

#define item       cs->CS_Buffer[cs->CS_CurChr]
#define from       cs->CS_Buffer
#define advance(x) cs->CS_CurChr += x;

    LONG result;

    while(TRUE)
    {
	D(bug("Str: %s\n", cs->CS_Buffer+cs->CS_CurChr));

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
		return ERROR_ACTION_NOT_KNOWN;

	    item = 0;

	    /* There must be something after a pipe... */
	    for
	    (
	        i = cs->CS_CurChr + 1;
		cs->CS_Buffer[i] == ' ' || cs->CS_Buffer[i] == '\t';
		i++
	    );

	    if(cs->CS_Buffer[i] == '\n' || cs->CS_Buffer[i] == ';' || cs->CS_Buffer[i] == '\0')
	    {
		SetIoErr(ERROR_LINE_TOO_LONG); /* what kind of error must we report? */
	        return ERROR_LINE_TOO_LONG;
	    }

	    D(bug("commannd = %S\n", &item+1));

	    if(rd->haveOutRD)
	    {
	        if (SystemTagList(&item+1, tags) == -1)
	            return IoErr();
	    }
	    else
	    {
                if (Pipe("PIPEFS:", &pipefhs[0], &pipefhs[1]) != DOSTRUE)
	            return IoErr();

	        tags[0].ti_Data = (IPTR)pipefhs[0];

	        if (SystemTagList(&item+1, tags) == -1)
		{
		    LONG error = IoErr();

		    Close(pipefhs[0]);
		    Close(pipefhs[1]);
		    
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
		return ERROR_ACTION_NOT_KNOWN;

	    /* Multiple redirections not allowed */
	    if(rd->haveInRD)
		return ERROR_TOO_MANY_LEVELS;

	    advance(1);
	    result = ReadItem(rd->inFileName, FILENAME_LEN, cs);

	    D(bug("Found input redirection\n"));

	    if(result == ITEM_ERROR || result == ITEM_NOTHING)
		return ERROR_OBJECT_NOT_FOUND;

	    rd->haveInRD = TRUE;
	}
	else if(item == '>')
	{
	    /* Prevent command lines like "Prompt> >>Olle echo Oepir" */
	    if(!rd->haveCommand)
		return ERROR_ACTION_NOT_KNOWN;

	    advance(1);

	    if(item == '>')
	    {
		/* Multiple redirections not allowed */
		if(rd->haveAppRD)
		    return ERROR_TOO_MANY_LEVELS;

		advance(1);
		result = ReadItem(rd->outFileName, FILENAME_LEN, cs);

		D(bug("Found append redirection\n"));

		if(result == ITEM_ERROR || result == ITEM_NOTHING)
		    return ERROR_OBJECT_NOT_FOUND;

		rd->haveAppRD = TRUE;
	    }
	    else
	    {
		/* Multiple redirections not allowed */
		if(rd->haveOutRD)
		    return ERROR_TOO_MANY_LEVELS;

		result = ReadItem(rd->outFileName, FILENAME_LEN, cs);

		D(bug("Found output redirection\n"));

		if(result == ITEM_ERROR || result == ITEM_NOTHING)
		    return ERROR_OBJECT_NOT_FOUND;

		rd->haveOutRD = TRUE;
	    }
	}
	else if (item == is->dollar) /* Possible environment variable usage */
	{
	    LONG size = cs->CS_CurChr;

	    advance(1);
	    result = ReadItem(avBuffer, sizeof(avBuffer), cs);

	    D(bug("Found variable\n"));

	    if(result == ITEM_ERROR || ITEM_NOTHING)
		return ERROR_REQUIRED_ARG_MISSING;

	    if
	    (
	        (GetVar(avBuffer, varBuffer, sizeof(varBuffer),
		       GVF_GLOBAL_ONLY | LV_VAR) != -1) &&
		!(varBuffer[0] == '$' && !strcmp(varBuffer+1, avBuffer))
            )
	    {
		LONG result;
		struct CSource varCs = { varBuffer, sizeof(varBuffer), 0 };

		D(bug("Real variable! Value = %s\n", varBuffer));

		if ((result = convertLine(filtered, &varCs, rd, is)) != 0)
		    return result;
	    }
	    else
		/* If this "variable" wasn't defined, we use the '$' as a
		   regular character */
	    {
		D(bug("No real variable\n"));

		if(!rd->haveCommand)
		{
		    cs->CS_CurChr = size;
		    getCommand(filtered, cs, rd, is);
		}
		else
		    appendString(filtered, cs->CS_Buffer + size,
				 cs->CS_CurChr - size);
	    }
	}
	else
	{
	    STRPTR s = &item;

	    if (strncmp(s, ".popis\n", 7) == 0)
	    {
		popInterpreterState(is);
		return 0;
	    }
	    else if (strncmp(s, ".pushis\n", 8) == 0)
	    {
		pushInterpreterState(is);
		return 0;
	    }
	    else if (item == is->dot)
	    {
		LONG error = 0, i, j, len;

		++s;

		if (s[0] == ' ') /* dot comment */
		    return 0;
		else if (doCommand(cs, filtered, "bra ", 4, NULL, 0, 
				   &is->bra, &error, is))
		{
		    return error;
		}
		else if (doCommand(cs, filtered, "ket ", 4, NULL, 0, 
				   &is->ket, &error, is))
		{
		    return error;
		}
		else if (doCommand(cs, filtered, "dollar ", 7, "dol ", 4,
				   &is->dollar, &error, is))
		{
		    return error;
		}
		else if (doCommand(cs, filtered, "dot ", 4, NULL, 0, 
				   &is->ket, &error, is))
		{
		    return error;
		}
		else if (strncasecmp(s, "key ", 4) == 0 ||
			 strncasecmp(s, "k ", 2) == 0)
		{
		    /* .key must be the first line of the script */
		    struct RDArgs *rd;

		    len = s[1] == ' ' ? 2 : 4;
		    appendString(filtered, &is->dot, 1);
		    appendString(filtered, s, len);
		    advance(++len);

		    if (is->rdargs)
		    {
			appendString(filtered, "duplicate", 10);
			FreeDosObject(DOS_RDARGS, is->rdargs);
			is->rdargs = NULL;
			return ERROR_ACTION_NOT_KNOWN;
		    }

		    is->rdargs = AllocDosObject(DOS_RDARGS, NULL);
		    if (is->rdargs)
		    {
			result = ReadItem(argBuffer, sizeof(argBuffer), cs);

			if (result == ITEM_ERROR)
			    return ERROR_UNKNOWN;

			if (result == ITEM_NOTHING)
			    return ERROR_KEY_NEEDS_ARG;

			len = strlen(argBuffer);
		        appendString(filtered, argBuffer, len + 1);

			s = AROS_BSTR_ADDR(cli->cli_CommandName);
			len = AROS_BSTR_strlen(cli->cli_CommandName);
			is->rdargs->RDA_Source.CS_Buffer = s;
			is->rdargs->RDA_Source.CS_Length = len;
			is->rdargs->RDA_Source.CS_CurChr = 0;

			rd = ReadArgs(argBuffer, is->arg, is->rdargs);
			if (rd)
			{
			    UBYTE t;

			    s = argBuffer;
			    is->argcount = 0;

			    for (i = 0; i < MAXARGS; ++i)
			    {
				len = 0;

				while (s[0] == ' ' || s[0] == '\t')
				    ++s;

				while (s[0] != '/' && s[0] != '\0')
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
				    CONST_STRPTR *m = (CONST_STRPTR*)is->arg[i];

				    kprintf("[Shell] .key[%s]/%02x = ",
					is->argname[i], (int)is->argtype[i]);
				    while (*m)
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
			    D(bug("[Shell] bad args for: %s\n", argBuffer));
			    FreeDosObject(DOS_RDARGS, is->rdargs);
			    is->rdargs = NULL;
			    return error;
			}

			return 0;
		    }
		    else
		    {
			D(bug("[Shell] memory exhausted\n"));
			return ERROR_NO_FREE_STORE;
		    }
		}
		else if (strncasecmp(s, "default ", 8) == 0 ||
			 strncasecmp(s, "def ", 4) == 0)
		{
		    len = s[3] == ' ' ? 5 : 9;
		    advance(len);

		    i = cs->CS_CurChr;
		    result = ReadItem(argBuffer, sizeof(argBuffer), cs);

		    if (result == ITEM_UNQUOTED)
		    {
			len = cs->CS_CurChr - i;

			i = getArgumentIdx(is, argBuffer, len);
			if (i < 0)
			    return ERROR_UNKNOWN;

			advance(1);
			len = cs->CS_CurChr;
		    	result = ReadItem(argBuffer, sizeof(argBuffer), cs);
			len = cs->CS_CurChr - len;
			switch (result)
			{
			case ITEM_ERROR:
			    return ERROR_UNKNOWN;
			case ITEM_NOTHING:
			    return ERROR_REQUIRED_ARG_MISSING;
			default:
			    if (is->argdef[i])
				FreeMem((APTR)is->argdef[i], is->argdeflen[i] + 1);

			    is->argdef[i] = (IPTR)AllocMem(len + 1, MEMF_LOCAL);
			    CopyMem(argBuffer, (APTR)is->argdef[i], len);
			    ((STRPTR)is->argdef[i])[len] = '\0';
			    is->argdeflen[i] = len;
			    advance(len);
			    return 0;
			}
		    }
		}
	    }

	    /* This is a regular character -- that is, we have a command */
	    if(!rd->haveCommand)
	    {
		D(bug("Found possible command\n"));

    		getCommand(filtered, cs, rd, is);
	    }
	    else
	    {
		/* Copy argument */
		LONG size = cs->CS_CurChr;

		result = ReadItem(argBuffer, sizeof(argBuffer), cs);

		/*??AGR who coded this shit ? */
		if(result == ITEM_ERROR || ITEM_NOTHING)
		    return ERROR_UNKNOWN;

		D(bug("\nFound argument %s\n", argBuffer));
		substArgs(filtered, from + size, cs->CS_CurChr - size, is);
	    }
	}
    }

    D(bug("Exiting convertLine()\n"));

    return 0;
}




BOOL getCommand(struct CSource *filtered, struct CSource *cs,
		struct Redirection *rd, struct InterpreterState *is)
{
    LONG  result;

    rd->haveCommand = TRUE;

    D(bug("Command found!\n"));

    result = ReadItem(rd->commandStr, COMMANDSTR_LEN, cs);

    if(result == ITEM_ERROR || result == ITEM_NOTHING)
	return FALSE;

    /* Is this command an alias? */
    if(GetVar(rd->commandStr, avBuffer, sizeof(avBuffer),
	      GVF_LOCAL_ONLY | LV_ALIAS) != -1)
    {
	struct CSource aliasCs = { avBuffer, sizeof(avBuffer), 0 };

	result = ReadItem(rd->commandStr, COMMANDSTR_LEN, &aliasCs);

	D(bug("Found alias! value = %s\n", avBuffer));

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


BOOL readLine(struct CommandLine *cl, BPTR inputStream)
{
    LONG letter; 

    while(TRUE)
    {
	letter = inputStream ? FGetC(inputStream) : EOF;

	D(bug("Read character %c (%d)\n", letter, letter));

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
BOOL appendString(struct CSource *cs, CONST_STRPTR fromStr, LONG size)
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

void unloadCommand(BPTR commandSeg, struct ShellState *ss)
{
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
		return MKBADDR(residentSeg);
	    }
	}

	Permit();
    }

    ss->residentCommand = FALSE;

    D(bug("Trying to load command1: %s\n", commandName));

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

    module = loadCommand(command, &ss);

    /* Set command name even if we couldn't load the command to be able to
       report errors correctly */
    SetProgramName(command);

    if(module != NULL)
    {
	struct Task *me = FindTask(NULL);
	STRPTR oldtaskname = me->tc_Node.ln_Name;
	BOOL  __debug_mem;
	LONG mem_before;

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
		unloadCommand(module, &ss);
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
	SetSignal(0, SIGBREAKF_CTRL_C);

	cli->cli_Module = seglist;

	me->tc_Node.ln_Name = command;
	
        __debug_mem = FindVar("__debug_mem", LV_VAR) != NULL;

	if (__debug_mem)
	{
	    FreeVec(AllocVec(~0ul/2, MEMF_ANY)); /* Flush memory */

	    mem_before = AvailMem(MEMF_ANY);
	    Printf("Available total memory before command execution: %10ld\n", mem_before);
	}

	cli->cli_ReturnCode = RunCommand(seglist, cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT,
					 cmd, len);
	if (__debug_mem)
	{
	    LONG mem_after;
	    FreeVec(AllocVec(~0ul/2, MEMF_ANY)); /* Flush memory */

	    mem_after = AvailMem(MEMF_ANY);
	    Printf("Available total memory after command execution:  %10ld\n", mem_after);
	    Printf("Memory difference (before - after):              %10ld\n", mem_before - mem_after);
	}

	me->tc_Node.ln_Name = oldtaskname;

	D(bug("Returned from command %s\n", command));
	unloadCommand(module, &ss);

	cli->cli_Result2 = IoErr();
    }
    else
    {
	/* Implicit cd? */
        /* SFS returns ERROR_INVALID_COMPONENT_NAME if you try to open "" */
	if(!(rd->haveInRD || rd->haveOutRD || rd->haveAppRD) &&
           (IoErr() == ERROR_OBJECT_WRONG_TYPE || IoErr() == ERROR_OBJECT_NOT_FOUND || IoErr() == ERROR_INVALID_COMPONENT_NAME))
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
	    PrintFault(IoErr(), command);
        }
    }

    D(bug("Done with the command...\n"));

    return error;
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

static void printPrompt(struct InterpreterState *is)
{
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

