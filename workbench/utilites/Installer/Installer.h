#ifndef _INSTALLER_H
#define _INSTALLER_H

#define DEBUG 1

#ifdef LINUX
#define FALSE	0
#define TRUE	1
#define IPTR	int
#define PrintFault(x,y)	/* */
#define IoErr()		/* */
#endif /* LINUX */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LINUX
#include <dos/dos.h>
#include <exec/exec.h>
#include <exec/execbase.h>
#include <proto/dos.h>
#include <proto/exec.h>

extern struct ExecBase *SysBase;
#endif /* !LINUX */


typedef struct ScriptArg
{
  struct  ScriptArg *next;	/* Next argument				*/
  struct  ScriptArg *parent;	/* Parent argument				*/
  char  * arg;			/* Either string or				*/
  struct  ScriptArg *cmd;	/* ptr to list of arguments			*/
				/* set one of them to NULL			*/
  int     intval;		/* If argument is an integer *arg will get NULL	*/
  int     ignore;		/* Parameters set this to 1 to disappear	*/

} ScriptArg;

typedef struct InstallerPrefs
{
  char * transcriptfile;
  FILE * transcriptstream;
  int debug;
  int welcome;
  int copyfail, copyflags;
} InstallerPrefs;

#define COPY_FAIL	1
#define COPY_NOFAIL	2
#define COPY_OKNODELETE	4
#define COPY_FORCE	1
#define COPY_ASKUSER	2

struct VariableList
{
  char * varsymbol;
  char * vartext;
  int varinteger;
};

struct ProcedureList
{
  char * procname;
  ScriptArg * procbody;
};

struct ParameterList
{
  char ** arg;
  int intval, intval2;
  int used;
};

#define SEMICOLON	0x3B
#define LINEFEED	0x0A
#define BACKSLASH	0x5C
#define SQUOTE		0x27
#define DQUOTE		0x22
#define LBRACK		0x28
#define RBRACK		0x29
#define DOLLAR		0x24
#define PERCENT		0x25
#define PLUS		0x2B
#define MINUS		0x2D
#define SPACE		0x20

#define MAXARGSIZE	1024

/* User levels */
#define _NOVICE		0
#define _AVERAGE	1
#define _EXPERT		2


#endif /* _INSTALLER_H */

