/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    The shell program.
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
#define printFlush(format...) do {Printf(format); Flush(Output());} while (0)

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
LONG interact(void);


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
void setupResidentCommands(void);
#define PROCESS(x) ((struct Process *)(x))

AROS_SH1(Shell, 41.1,
AROS_SHA(STRPTR, ,COMMAND,/F,NULL))
{
    AROS_SHCOMMAND_INIT

    LONG error = RETURN_OK;

    D(bug("Executing shell\n"));

    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 39);
    if (!UtilityBase) return RETURN_FAIL;

    setupResidentCommands();

    cli = Cli();
    setPath(NULL);

    if (strcmp(FindTask(NULL)->tc_Node.ln_Name, "Boot Shell") == 0)
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
	    error = checkLine(&rd, &cl);
	    Redirection_release(&rd);
	}
        
	D(bug("Command done\n"));
    }
    else
    {
        error = interact();
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
LONG interact(void)
{
    ULONG cliNumber = PROCESS(FindTask(NULL))->pr_TaskNum;
    LONG  error = 0;
    BOOL  moreLeft = FALSE;

    if (!cli->cli_Background)
    {
	SetVBuf(Output(), NULL, BUF_FULL, -1);
	if (strcmp(FindTask(NULL)->tc_Node.ln_Name, "Boot Shell") == 0)
	{
	    PutStr
	    (
	    	"AROS - The Amiga® Research Operating System\n"
		"Copyright © 1995-2003, The AROS Development Team. All rights reserved.\n"
		"AROS is licensed under the terms of the AROS Public License (APL),\n"
		"a copy of which you should have received with this distribution.\n"
		"Visit http://www.aros.org/ for more information.\n"
	    );
	}
	else
	{
	    IPTR data[] = {(IPTR)cliNumber};

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
	        printPrompt();

	    moreLeft = readLine(&cl, cli->cli_CurrentInput);
	    error = checkLine(&rd, &cl);

	    Redirection_release(&rd);
	    FreeVec(cl.line);
	}

	if (!moreLeft)
	{
	    if (!cli->cli_Interactive)
	    {
		Close(cli->cli_CurrentInput);

	        if (AROS_BSTR_strlen(cli->cli_CommandFile))
		{
	            DeleteFile(BADDR(cli->cli_CommandFile));
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

    if (cli->cli_Interactive) printFlush("Process %ld ending\n", cliNumber);

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
BOOL checkLine(struct Redirection *rd, struct CommandLine *cl)
{
    /* The allocation is taken care of by appendString */
    struct CSource filtered = { NULL, 0, 0 };
    struct CSource cs       = { cl->line, strlen(cl->line), 0 };
    struct LocalVar *lv;
    BOOL  result = FALSE;
    
    lv = FindVar("echo", LV_VAR);
    if (lv != NULL)
    {
	/* AmigaDOS' shell is content also with echo being set to anything
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

    if(convertLine(&filtered, &cs, rd))
    {
	D(bug("Position %i\n", filtered.CS_CurChr));

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

	if(rd->haveOutRD)
	{
	    D(bug("Redirecting output to file %s\n", rd->outFileName));

	    rd->newOut = Open(rd->outFileName, MODE_NEWFILE);

	    D(bug("Output stream opened\n"));

	    if(BADDR(rd->newOut) == NULL)
	    {
		goto exit;
	    }


    	    SelectOutput(rd->newOut);
	}

	if(rd->haveAppRD)
	{
	    rd->newOut = Open(rd->outFileName, (FMF_MODE_OLDFILE | FMF_CREATE | FMF_APPEND) & ~FMF_AMIGADOS);

	    if(BADDR(rd->newOut) == NULL)
	    {
	        goto exit;
	    }

    	    SelectOutput(rd->newOut);
	}

	if(rd->haveInRD)
	{
	    rd->newIn = Open(rd->inFileName, MODE_OLDFILE/*FMF_READ*/);

	    if(BADDR(rd->newIn) == NULL)
	    {
	       goto exit;
	    }

    	    SelectInput(rd->newIn);
	}

	D(bug("Calling executeLine()\n"));

	/* OK, we've got a command. Let's execute it! */
	executeLine(rd->commandStr, filtered.CS_Buffer, rd);

	SelectInput(cli->cli_StandardInput);
	SelectOutput(cli->cli_StandardOutput);
	result = TRUE;
    }
    else
    {
	PutStr("Erroneous command line.\n");
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

static BPTR DupFH(BPTR fh, LONG mode)
{
    BPTR ret = NULL;

    if (fh)
    {
        BPTR olddir = CurrentDir(fh);
        ret    = Open("", mode);

        CurrentDir(olddir);
    }

    return ret;
}

static BOOL Pipe(BPTR pipefhs[2])
{
    pipefhs[0] = Open("PIPEFS:__UNNAMED__", FMF_READ|FMF_NONBLOCK);
    pipefhs[1] = DupFH(pipefhs[0], FMF_WRITE);

    if (pipefhs[1])
    {
        ChangeMode(CHANGE_FH, pipefhs[0], FMF_READ);
	return TRUE;
    }

    Close(pipefhs[0]);

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

BOOL convertLine(struct CSource *filtered, struct CSource *cs,
		 struct Redirection *rd)
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
		{ NP_StackSize, Cli()->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT },
		{ TAG_DONE    , 0 	    	    	    	    	    	}
    	    };

	    /* Prevent command lines like "Prompt> | Olle echo Oepir" */
	    if(!rd->haveCommand)
		return FALSE;

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
	        return FALSE;
	    }

	    D(bug("commannd = %S\n", &item+1));

	    if(rd->haveOutRD)
	    {
	        if (SystemTagList(&item+1, tags) == -1)
	            return FALSE;
	    }
	    else
	    {
	        if (!Pipe(pipefhs))
	            return FALSE;

	        tags[0].ti_Data = (IPTR)pipefhs[0];

	        if (SystemTagList(&item+1, tags) == -1)
		{
		    Close(pipefhs[0]);
		    Close(pipefhs[1]);
		    
		    return FALSE;
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
		return FALSE;

	    /* Multiple redirections not allowed */
	    if(rd->haveInRD)
		return FALSE;

	    advance(1);
	    result = ReadItem(rd->inFileName, FILENAME_LEN, cs);

	    D(bug("Found input redirection\n"));

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

		D(bug("Found append redirection\n"));

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

		D(bug("Found output redirection\n"));

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

	    D(bug("Found variable\n"));

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

		D(bug("Real variable! Value = %s\n", varBuffer));

		if(convertLine(filtered, &varCs, rd) == FALSE)
		    return FALSE;
	    }
	    else
		/* If this "variable" wasn't defined, we use the '$' as a
		   regular character */
	    {
		D(bug("No real variable\n"));

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

	    /* This is a regular character -- that is, we have a command */
	    if(!rd->haveCommand)
	    {
		D(bug("Found possible command\n"));

    		getCommand(filtered, cs, rd);
	    }
	    else
	    {
		/* Copy argument */
		LONG size = cs->CS_CurChr;

		// P(kprintf("Checking argument\n"));

		result = ReadItem(argBuffer, sizeof(argBuffer), cs);

		// D(bug("Found possible argument\n"));

		if(result == ITEM_ERROR || ITEM_NOTHING)
		    return FALSE;

		appendString(filtered, from + size, cs->CS_CurChr - size);

		D(bug("\n"));

		D(bug("Found argument %s\n", argBuffer));
	    }
	}
    }

    D(bug("Exiting convertLine()\n"));

    return TRUE;
}




BOOL getCommand(struct CSource *filtered, struct CSource *cs,
		struct Redirection *rd)
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
	return convertLine(filtered, &aliasCs, rd);
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
    char letter; 

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
	D(bug("Command loaded!\n"));

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
					 commandArgs, strlen(commandArgs));
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
		Printf("%ld", PROCESS(FindTask(NULL))->pr_TaskNum);
		break;
	    case 'R':
	    case 'r':
		Printf("%ld", Cli()->cli_ReturnCode);
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

