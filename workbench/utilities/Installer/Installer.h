/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INSTALLER_H
#define _INSTALLER_H

#define INSTALLER_NAME "Installer"
#define INSTALLER_VERSION 43
#define INSTALLER_REVISION 3

/*
   This flag is not only for internal verbosity, but embraces outputs
   where Intuition GUI is still missing (mostly errors), too.
*/
#define DEBUG 1
//#undef DEBUG

#ifdef DEBUG
#   include <proto/arossupport.h>
#   define DMSG(x...) kprintf(x)
#else
#   define DMSG(x...) /* */
#endif /* DEBUG */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dos/dos.h>
#include <exec/exec.h>
#include <exec/execbase.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/icon.h>
#include <proto/workbench.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <dos/bptr.h>

extern struct ExecBase *SysBase;

typedef struct ScriptArg
{
    struct ScriptArg *next;   /* Next argument				*/
    struct ScriptArg *parent; /* Parent argument			*/
    char             *arg;    /* Either string or			*/
    struct ScriptArg *cmd;    /* ptr to list of arguments		*/
                              /* set one of them to NULL		*/
    long int         intval;  /* If argument is an integer *arg will
				 get NULL				*/
    int	             ignore;  /* Parameters set this to 1 to disappear	*/

} ScriptArg;

/* Error codes used for ( trap ... ) and onerror */
#define NOERROR		0 /* no error occurred	*/
#define USERABORT	1 /* user aborted	*/
#define OUTOFMEMORY	2 /* out of memory	*/
#define	SCRIPTERROR	3 /* error in script	*/
#define DOSERROR	4 /* DOS error		*/
#define	BADPARAMETER	5 /* bad parameter data	*/

#define NUMERRORS	5 /* Number of error codes */

typedef struct InstallerPrefs
{
    char * transcriptfile;
    BPTR transcriptstream;
    int debug, pretend, nopretend, novicelog, noprint;
    int welcome;
    int copyfail, copyflags;
    ScriptArg onerror, *onerrorparent;
    ScriptArg trap[NUMERRORS], *trapparent[NUMERRORS];
    int minusrlevel, defusrlevel;
    int fromcli;
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
    long int varinteger;
};

struct ProcedureList
{
    char * procname;
    ScriptArg * procbody;
    char ** arglist;
    int argnum;
};

struct ParameterList
{
    char ** arg;
    long int intval, intval2;
    int used;
};


/* Special characters */
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

