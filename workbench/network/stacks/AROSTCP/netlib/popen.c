/* $Id$
 *
 *      popen.c - Unix comaptible popen() and pclose()
 *
 *      Author: Rick Schaeffer <ricks@isc-br.isc-br.com>
 *		Pavel Fedin <sonic_amiga@rambler.ru>
 */

/****** net.lib/popen *******************************************************
    NAME
        popen, pclose - initiate I/O to/from a process

    SYNOPSIS
        #include <stdio.h>

        FILE *popen(command, type)
        char *command, *type;

        pclose(stream)
        FILE *stream;

    DESCRIPTION
        The arguments to popen are pointers to null-terminated
        strings containing respectively a command line and an
        I/O mode, either "r" for reading or "w" for writing.  It
        creates a pipe between the calling process and the command
        to be executed.  The value returned is a stream pointer that
        can be used (as appropriate) to write to the standard input
        of the command or read from its standard output.

        A stream opened by popen **MUST** be closed by pclose, which
        waits for the associated process to terminate and returns
        the exit status of the command.

        Because stdio files are shared, a type "r" command may be
        used as an input filter, and a type "w" as an output filter.

    DIAGNOSTICS
        Popen returns a null pointer if files or processes cannot be
        created.

        Pclose returns -1 if stream is not associated with a
        `popened' command.

    AUTHOR
        Original version by Rick Schaeffer <ricks@isc-br.isc-br.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/record.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

#include <errno.h>

#include "conf.h"

extern char *mktemp(char *);

struct POmsg {
    struct Message  POm;
    int     rc;
    char        *cmd;
    struct Library  *DOSBase;
    };


struct pstruct {
    FILE    *fptr;
    struct POmsg    childmsg;
    };

#define MAXPIPES    6
static struct pstruct poarray[MAXPIPES];

static int childprocess(void);

FILE *popen(const char *cmd, const char *mode)
{
    static char tempname[] = "pipe:pXXX.XXX";
    char        *pname,redir[20];
    short       i;
    int         pmode;
    struct pstruct  *poptr;
    struct Process  *child;
    struct CommandLineInterface *cli;
    ULONG stacksize;
    struct Process *thistask;
    FILE            *fptr;

    /* First, get pointers to our process and cli structs */
    thistask = (struct Process *) FindTask(NULL);
    cli = Cli();
    poptr = NULL;

    /* now find an open pipe (we currently only allow 6 simultaneously
       open pipes) */
    for (i=0; i<MAXPIPES; i++) {
        if (poarray[i].fptr == NULL) {
            poptr = &poarray[i];
            break;
            }
        }
    if (poptr == NULL) {
        fprintf(stderr,"popen: Unable to find an open pipe\n");
        errno = EMFILE;
        return(NULL);
        }
    if (strcmp(mode,"r") == 0)
        pmode = MODE_NEWFILE;
    else if (strcmp(mode,"w") == 0)
        pmode = MODE_OLDFILE;
    else {
        fprintf(stderr,"popen: Mode must be 'r' or 'w'\n");
        errno = EINVAL;
        return(NULL);
        }

    /* Try to make a guaranteed unique file name for the pipe */
    strcpy(redir,tempname);
    redir[5] = 'a' + i;

    pname = mktemp(redir);            /* set up a pipe: file name */

    /* Now get the child's stack and priority set up */
    if (cli)
        stacksize = cli->cli_DefaultStack << 2;
    else
        stacksize = thistask->pr_StackSize;

    /* Now open our side of the pipe */
    fptr = fopen(pname,mode);
    if (fptr == NULL) {
        fprintf(stderr,"popen: Unable to open pipe file %s\n",pname);
        return(NULL);
        }

    /* create the command.  since the "System" function runs through
       the default shell, we need to tell it not to fail so that we
       ALWAYS get back the exit status.  This wouldn't be necessary
       if the CLI created by the System function inherited the parent's
       FAILAT level.
       The pipe file is passed as redirection to shell, which the 
       SystemTags() will parse. For some reason the default "more"
       could not get it's input properly if redirection was not used.
    */
    poptr->childmsg.cmd = malloc(strlen(cmd) + 15 + 20);
    sprintf(poptr->childmsg.cmd, "failat 9999\n%s %c%s\n",
	    cmd, (pmode == MODE_NEWFILE) ? '>' : '<', pname);

    /* Create a port that we can get the child's exit status through */
    poptr->childmsg.POm.mn_ReplyPort = CreateMsgPort();
    poptr->childmsg.POm.mn_Node.ln_Type = NT_MESSAGE;
    poptr->childmsg.POm.mn_Node.ln_Pri = 0;
    if (poptr->childmsg.POm.mn_ReplyPort == 0) {
        fclose(fptr);
        fprintf(stderr,"popen: Couldn't create message port\n");
        errno = ENOMEM;
        return(NULL);
        }

    /* Now we can start the new process.  NOTE: this is actually going
       to create a process consisting ONLY of the function "childprocess"
       which can be seen below.  childprocess() then runs the command
       passed in the startup message.
    */
    child = CreateNewProcTags(
        NP_Entry,   (Tag) childprocess,
        NP_Input,   Input(),
        NP_Output,  Output(),
        NP_CloseInput,  FALSE,
        NP_CloseOutput, FALSE,
        NP_StackSize,   stacksize,
        NP_Cli,     TRUE,
#ifdef __MORPHOS__
	NP_CodeType, CODETYPE_PPC,
#endif
        TAG_DONE
        );

    poptr->childmsg.DOSBase = (struct Library *)DOSBase;

    /* now pass the child the startup message */
    PutMsg(&child->pr_MsgPort,(struct Message *) &poptr->childmsg);

    return(poptr->fptr = fptr);
}

FILE *popenl(const char *arg0, ...)
{
    va_list ap;
    char argbuf[512], *mode;

    strcpy(argbuf, arg0);
    va_start(ap, arg0);
    while(1)
    {
        char *s = va_arg(ap, char *);

        if(s == NULL)
        {
        strcat(argbuf, "\n");
        break;
        } /* if */

        strcat(argbuf, " ");

        if(strchr(s, ' '))
        {
        strcat(argbuf, "\"");
        strcat(argbuf, s);
        strcat(argbuf, "\"");
        }
        else
        {
        strcat(argbuf, s);
        } /* if */
    }
    mode = va_arg(ap, char *);
    va_end(ap);

    return(popen(argbuf, mode));

} /* popenl */

int pclose(FILE *fptr)
{
    short       i;

    /* Figure out which pipe we used for this file */
    for (i=0; i<MAXPIPES; i++)
        if (poarray[i].fptr == fptr)
            break;
    if (i >= MAXPIPES) {
        fprintf(stderr,"popen: DISASTER...couldn't find file pointer in pclose\n");
        exit(1);
        }

    /* close the file */
    fclose(fptr);

    /* now wait for the exit status */
    WaitPort(poarray[i].childmsg.POm.mn_ReplyPort);
    poarray[i].fptr = NULL;

    /* clean things up */
    DeletePort(poarray[i].childmsg.POm.mn_ReplyPort);
    free(poarray[i].childmsg.cmd);
    return(poarray[i].childmsg.rc);
}

/* SAS/C autoinitialization for cleanup! */
void STDARGS DESTRUCTOR _STD_4000_popen(void)
{
    short i;

    /* Close all the open pipes! */
    for(i=0; i<MAXPIPES; i++)
    {
        if(poarray[i].fptr)
        {
            pclose(poarray[i].fptr);
        } /* if */
    } /* for */

} /* _STD_4000_popen */

char *mktemp(char * template)
{
    register char *cp;
    register unsigned long val;
    BPTR lock;

    cp = template;
    cp += strlen(cp);
    for (val = (unsigned long) FindTask(0L) ; ; )
        if (*--cp == 'X') {
            *cp = val%10 + '0';
            val /= 10;
        } else if (*cp != '.')
            break;

    if (*++cp != 0) {
        *cp = 'A';
        while (lock = Lock(template, ACCESS_READ))
        {
	    UnLock(lock);
            if (*cp == 'Z') {
                *template = 0;
                break;
            }
            ++*cp;
        }
    } else {
        if (lock = Lock(template, ACCESS_READ)) {
	    UnLock(lock);
            *template = 0;
	}
    }
    return template;
}

/* WATCH OUT! This only works without __saveds because of the special
   SAS/C 6.1 tricks I use! Check the output with omd! */
static int INTERRUPT childprocess(void)
{
    struct ExecBase *SysBase = *((struct ExecBase **)4);
    struct Library *DOSBase;
    struct Process  *me;
    struct POmsg    *startupmsg;
    int             i = RETURN_FAIL;

    /* find our process structure */
    me = (struct Process *) FindTask(NULL);

    /* Wait for the parent to kick us off */
    WaitPort(&me->pr_MsgPort);

    /* Get the command to execute */
    startupmsg = (struct POmsg *) GetMsg(&me->pr_MsgPort);

    DOSBase = startupmsg->DOSBase;

    if(DOSBase)
    {
        /* Now run the command.  stdin and stdout are already set up */
        i = SystemTags(startupmsg->cmd,
               SYS_UserShell, 1,
               TAG_DONE);
    } /* if */

    if(i > 0)
    {
        /* UNIX compatibility ... */
        i <<= 8;
    } /* if */

    startupmsg->rc = i;
    /* pass the exit code back to the parent */
    ReplyMsg((struct Message *) startupmsg);
    return(0);
}
