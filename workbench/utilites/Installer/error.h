#ifndef _ERROR_H
#define _ERROR_H

/* Used for ( trap ... ) to trap errors while executing script */
#define NOERROR		0 /* no error occurred	*/
#define USERABORT	1 /* user aborted	*/
#define OUTOFMEMORY	2 /* out of memory	*/
#define	SCRIPTERROR	3 /* error in script	*/
#define DOSERROR	4 /* DOS error		*/
#define	BADPARAMETER	5 /* bad parameter data	*/

struct TrapList
{
  struct TrapList *next;
  ScriptArg *cmd;
  int code;
};


/* ( onerror ... ) will be executed when a fatal error occurred that was not trapped */

struct OnerrList
{
  struct OnerrList *next;
  ScriptArg *cmd;
};



#endif /* _ERROR_H */

