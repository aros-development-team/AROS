#ifndef _INSTALLER_H
#define _INSTALLER_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ScriptArg
{
  struct  ScriptArg *next;	/* Next argument				*/
  struct  ScriptArg *parent;	/* Parent argument				*/
  char  * arg;			/* Either string or				*/
  struct  ScriptArg *cmd;	/* ptr to list of arguments			*/
				/* Set one to NULL				*/
  char  * value;		/* Holds the return value, if cmd is evaluated	*/
  int     intval;		/* if argument is an integer *arg will get NULL	*/

} ScriptArg;


#define SEMICOLON	0x3B
#define LINEFEED	0x0A
#define BACKSLASH	0x5C
#define SQUOTE		0x27
#define DQUOTE		0x22
#define LBRACK		0x28
#define RBRACK		0x29
#define DOLLAR		0x24
#define PERCENT		0x25

#define MAXARGSIZE	1024


#endif /* _INSTALLER_H */
