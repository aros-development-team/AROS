/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SHELL_H
#define SHELL_H 1

#include <dos/dosextens.h>
#include <dos/dos.h>	/* for BPTR		*/

#include "buffer.h"

#define FILE_MAX 256 /* max length of file name */
#define LINE_MAX 512 /* max length of full command line */

/* TODO move */
/* Template options (copied *AND MODIFIED* from ReadArgs) */
#define REQUIRED 0x80 /* /A */
#define KEYWORD  0x40 /* /K */
#define MULTIPLE 0x20 /* /M */
#define NORMAL   0x00 /* No option */
#define SWITCH   0x01 /* /S, implies /K */
#define TOGGLE   0x02 /* /T, implies /K */
#define NUMERIC  0x04 /* /N */
#define REST     0x08 /* /F */

#define MAXARGS    32
#define MAXARGLEN  32

struct SArg
{
    TEXT   name[MAXARGLEN];
    LONG   namelen;
    LONG   len;
    IPTR   def;
    LONG   deflen;
    UBYTE  type;
};

typedef struct _ShellState
{
    BPTR	newIn;
    BPTR	newOut;
    BPTR	oldIn;
    BPTR	oldOut;

    TEXT	command[FILE_MAX + 2];	/* command buffer */

    BPTR	oldHomeDir;	/* shared lock on program file's directory */

    LONG	cliNumber;

    LONG	argcount;	/* script args count */
    struct SArg	args[MAXARGS];	/* args definitions */
    IPTR	arg[MAXARGS];	/* args values */
    struct RDArgs *arg_rd;	/* Current RDArgs return state */

    TEXT	bra, ket, dollar, dot;
    TEXT        mchar0, pchar0;

    struct _ShellState *stack;

    ULONG	flags;		/* DOS/CliInit*() flags cache */

    APTR        ss_DOSBase;
    APTR        ss_SysBase;
} ShellState;

#define DOSBase (ss->ss_DOSBase)
#define SysBase ((struct ExecBase *)ss->ss_SysBase)

/* Function: convertLine
 *
 * Action:   Parses a command line and returns a filtered version (removing
 *           redirections, incorporating embedded commands, taking care of
 *           variable references and aliases.
 *
 * Input:    ShellState		*ss           --  this state
 *	     Buffer		*in           --  input string
 *           Buffer		*out          --  output command string
 *           BOOL		*haveCommand  --  true if line have command
 *
 * Output:   LONG  --  error code or 0 if everything went OK
 */
LONG convertLine(ShellState *ss, Buffer *in, Buffer *out, BOOL *haveCommand);

LONG convertLineDot(ShellState *ss, Buffer *in);

/* Function: readLine
 *
 * Action:   Read one line of a stream into a buffer.
 *
 * Input:    ShellState                  *ss        --  this state
 *           struct CommandLineInterface *cli       --  the CLI
 *           Buffer                      *out       --  the result
 *           BOOL                        *moreLeft  --  not end of stream result
 *
 * Note:     This routine reads a full command line.
 *           As a side effect, it also updates ss->pchar0 and ss->mchar0
 *
 * Output:   SIPTR --  DOS error code
 */
LONG readLine(ShellState *ss, struct CommandLineInterface *cli, Buffer *out, BOOL *moreLeft);

/* Function: checkLine
 *
 * Action:   Parse a command line and do consistency checks
 *
 * Input:    ShellState		     *ss    --  this state
 *           Buffer		     *in    --  the input buffer
 *           Buffer		     *out   --  the result will be stored here
 *           BOOL		      echo  --  true if command echoed
 *
 * Output:   LONG --  DOS error code
 */
LONG checkLine(ShellState *ss, Buffer *in, Buffer *out, BOOL echo);

/* Function: releaseFiles
 *
 * Action:   Deallocate file resources used for redirecion and reinstall
 *           standard input and output streams.
 *
 * Input:    ShellState	*ss  --  this state
 *
 * Output:   --
 */
void releaseFiles(ShellState *ss);

/* Function: interact
 *
 * Action:   Execute a commandfile and then perform standard shell user
 *           interaction.
 *
 * Input:    ShellState	    *is           --  this state
 *
 * Output:   LONG  --  error code
 */
LONG interact(ShellState *ss);


/* Function: Redirection_release
 *
 * Action:   Release resources allocated in the state
 *
 * Input:    ShellState	    *ss  --  this state
 *
 * Output:   --
 */
void Redirection_release(ShellState *ss);

/* Function: Redirection_init
 *
 * Action:   Initialize a state structure
 *
 * Input:    ShellState	    *ss  --  this state
 *
 * Output:   LONG  --  DOS error code
 */
LONG Redirection_init(ShellState *ss);

/* Function: setPath
 *
 * Action:   Set the current command (standard) path.
 *
 * Input:    BPTR lock  --  a lock on the directory
 *
 * Notes:    This will set the current directory name via
 *           SetCurrentDirName() even though this is not used later.
 *
 * Output:   --
 */
void setPath(ShellState *ss, BPTR lock);

/* Function: cliPrompt
 *
 * Action:   Print the prompt to indicate that user input is viable.
 *
 * Input:    ShellState	    *ss  --  this state
 *
 * Output:   --
 */
void cliPrompt(ShellState *ss);

/* Other internal functions
 *
 * FIXME: some doc ?
 */
void initDefaultInterpreterState(ShellState *ss);
void popInterpreterState(ShellState *ss);
LONG pushInterpreterState(ShellState *ss);

LONG convertArg(ShellState *ss, Buffer *in, Buffer *out, BOOL *quoted);
LONG convertBackTicks(ShellState *ss, Buffer *in, Buffer *out, BOOL *quoted);
LONG convertRedir(ShellState *ss, Buffer *in, Buffer *out);
LONG convertVar(ShellState *ss, Buffer *in, Buffer *out, BOOL *quoted);
LONG l2a(LONG x, STRPTR buf); /* long to ascii */

void cliEcho(ShellState *ss, CONST_STRPTR args);
LONG cliLen(CONST_STRPTR s);
BOOL cliNan(CONST_STRPTR s);
void cliVarNum(ShellState *ss, CONST_STRPTR name, LONG value);

#endif
