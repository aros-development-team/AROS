/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: The shell program.
    Lang: English
*/
#include <exec/memory.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <ctype.h>
#include <stdio.h>
#include <aros/debug.h>

struct Library *UtilityBase;

static const char version[] = "$VER: shell 41.4 (18.12.1999)\n";

BPTR cLock;
struct CommandLineInterface *cli;



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


static void setpath(BPTR lock)
{
    BPTR    dir;
    STRPTR  buf;
    ULONG   i;

    if(lock == NULL)
	dir = CurrentDir(0);
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

static void printCliNum(void)
{
    IPTR   args[2] = { (IPTR)(PROCESS(FindTask(NULL))->pr_TaskNum), NULL };
    
    VFPrintf(Output(), "%ld", (IPTR *)&args);
}


static void printResult(void)
{
    IPTR args[2] = { (IPTR)(Cli()->cli_ReturnCode), NULL };
    
    VFPrintf(Output(), "%ld", (IPTR *)&args);
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
		printCliNum();
		break;
	    case 'R':
		printResult();
		break;
	    case 'S':
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


struct linebuf
{
    BPTR   file;
    UBYTE *buf;
    ULONG  size;
    ULONG  bend;
    ULONG  lend;
    LONG   eof;
};


static LONG readline(struct linebuf *lb)
{
    if(lb->bend)
    {
	UBYTE *src = lb->buf + lb->lend, *dst = lb->buf;
	ULONG  i   = lb->bend - lb->lend;

	if(i)
	{
	    do
		*dst++ = *src++;
	    while(--i);
	}

	lb->bend -= lb->lend;
    }

    lb->lend = 0;

    for(;;)
    {
	LONG subsize;

	for(;lb->lend < lb->bend;)
	    if(lb->buf[lb->lend++] == '\n')
	    {
		lb->buf[lb->lend-1] = '\0';
		return 0;
	    }

	if(lb->eof)
	{
	    FreeVec(lb->buf);
	    lb->buf = NULL;
	    lb->eof = 0;
	    return 0;
	}

	if(lb->bend >= lb->size)
	{
	    UBYTE *newbuf = AllocVec(lb->size + 256, MEMF_ANY);

	    if(newbuf == NULL)
	    {
		FreeVec(lb->buf);
		lb->buf = NULL;
		return ERROR_NO_FREE_STORE;
	    }

	    CopyMem(lb->buf, newbuf, lb->bend);
	    FreeVec(lb->buf);
	    lb->buf = newbuf;
	    lb->size += 256;
	}

	subsize = Read(lb->file, lb->buf + lb->bend, lb->size - lb->bend);

	if(subsize == 0)
	{
	    if(lb->bend)
		lb->buf[lb->bend++] = '\n';
	    lb->eof=1;
	} else if(subsize < 0)
	{
	    /*ada 30.8.96 This is a REALLY BAD hack !! */
	    if(IoErr() != -1 && IoErr() != 0)
	    {
		ULONG error = IoErr();

		VPrintf("Shell:Read %ld\n", &error);
		PrintFault(IoErr(), "Shell:Read");
		FreeVec(lb->buf);
		lb->buf = NULL;
		return IoErr();
	    }
	}
	else
	    lb->bend += subsize;
    }
}


BPTR loadseg(STRPTR name)
{
    BPTR old, *cur, seg;
    STRPTR s;

    seg = LoadSeg(name);

    if(seg)
	return seg;

    s = name;

    while(*s)
    {
	if(*s == ':' || *s == '/')
	    return 0;

	s++;
    }

    old = CurrentDir(0);
    cur = (BPTR *)BADDR(cli->cli_CommandDir);

    while(cur != NULL)
    {
	CurrentDir(cur[1]);
	seg = LoadSeg(name);

	if(seg)
	    break;

	cur = (BPTR *)BADDR(cur[0]);
    }

    if (!seg)
    {
	CurrentDir(cLock);
	seg = LoadSeg(name);
    }

    CurrentDir(old);
    return seg;
}


LONG execute(STRPTR com)
{
    BOOL   ended = FALSE;
    STRPTR s1 = NULL, s2 = NULL;
    STRPTR args, rest, command = NULL, infile = NULL, outfile = NULL;
    STRPTR appfile = NULL;
    STRPTR last;
    BPTR   in = 0, out = 0;
    BPTR   seglist, lock;
    struct FileInfoBlock *fib;
    LONG res, size, error = 0;
    struct CSource cs;

    last = com;

    while(*last++);

    args = s1 = (STRPTR)AllocVec(last - com + 1, MEMF_ANY);
    rest = s2 = (STRPTR)AllocVec(last - com + 1, MEMF_ANY | MEMF_CLEAR);

    if(args == NULL || rest == NULL)
    {
	error = ERROR_NO_FREE_STORE;
	goto end;
    }

    cs.CS_Buffer = com;
    cs.CS_Length = last - com - 1;
    cs.CS_CurChr = 0;

    for(;;)
    {
	while(com[cs.CS_CurChr] == ' ' || com[cs.CS_CurChr] == '\t')
	    *rest++ = com[cs.CS_CurChr++];

	/* Skip comments  */
	if(com[cs.CS_CurChr] == ';')
	    break;

	if(command == NULL)
	    command = args;
	else if(com[cs.CS_CurChr] == '<')
	{
	    cs.CS_CurChr++; /* Skip redirection character */
	    infile = args;
	}
	else if(com[cs.CS_CurChr] == '>')
	{
	    cs.CS_CurChr++; /* Skip redirection character */
	    
	    if(com[cs.CS_CurChr] == '>')
	    {
		cs.CS_CurChr++; /* Skip redirection character */
		appfile = args;
	    }
	    else
		outfile = args;
	}
	else
	{
	    size = cs.CS_CurChr;
	    res = ReadItem(args, ~0ul/2, &cs);

	    while(size < cs.CS_CurChr)
		*rest++ = com[size++];

	    if(res == ITEM_NOTHING || res == ITEM_ERROR)
	    {
		*rest++ = '\n';
		break;
	    }
	    continue;
	}

	res = ReadItem(args, ~0ul/2, &cs);

	if(res != ITEM_QUOTED && res != ITEM_UNQUOTED)
	    break;

	while(*args++);
    }

    if((!command) || (!*command))
	goto end;

    if(!Stricmp(command, "ENDCLI"))
    {
        ended = TRUE;
        goto end;
    }

    if(infile != NULL)
    {
	in = Open(infile, MODE_OLDFILE);

	if(!in)
	{
	    infile = NULL;
	    error = IoErr();
	    goto end;
	}

	cli->cli_CurrentInput = in;
	in = SelectInput(in);
    }

    if(outfile != NULL)
    {
	out = Open(outfile, MODE_NEWFILE);
	if(!out)
	{
	    outfile = NULL;
	    error = IoErr();
	    goto end;
	}

	cli->cli_CurrentOutput = out;
	out = SelectOutput(out);
    }

    if(out == NULL && appfile != NULL)
    {
	out = Open(appfile, MODE_OLDFILE);

	if(!out)
	{
	    outfile = NULL;
	    error = IoErr();
	    goto end;
	}

	cli->cli_CurrentOutput = out;
	Seek(out, 0, OFFSET_END);
	out = SelectOutput(out);
    }

    seglist = loadseg(command);

    if(seglist)
    {
	last = s2;

	while(*last++);

	SetProgramName(command);
	cli->cli_Module = seglist;
	cli->cli_ReturnCode = RunCommand(seglist, 100000, s2, last - s2 - 1);
	AROS_BSTR_setstrlen(cli->cli_CommandName, 0);
	AROS_BSTR_putchar(cli->cli_CommandName, 0, '\0');
	seglist = cli->cli_Module;
	UnLoadSeg(seglist);
	cli->cli_Result2 = IoErr();
    } else if(infile == NULL && outfile == NULL)
    {
	lock = Lock(command, SHARED_LOCK);

	if(lock)
	{
	    fib = AllocDosObject(DOS_FIB, NULL);

	    if(fib != NULL)
	    {
		if(Examine(lock, fib))
		{
		    if(fib->fib_DirEntryType > 0)
		    {
			setpath(lock);
			lock = CurrentDir(lock);
		    }
		    else
			SetIoErr(error = ERROR_OBJECT_WRONG_TYPE);
		}

		FreeDosObject(DOS_FIB, fib);
	    }

	    UnLock(lock);
	}
	else
	    error = IoErr ();
    }

end:
    if(in)
	Close(SelectInput(in));

    if(out)
	Close(SelectOutput(out));

    FreeVec(s1);
    FreeVec(s2);

    if(error)
    {
	cli->cli_Result2 = error;
	PrintFault(error, "Couldn't run command");
    }

    Flush(Output());

    if(ended)
        return -1;

    return 0;
}


int executefile(STRPTR name)
{
    struct linebuf lb = { 0, NULL, 0, 0, 0, 0 };
    LONG           error = 0;

    lb.file = Open(name, MODE_OLDFILE);

    if(lb.file)
    {
	cli->cli_CurrentInput = lb.file;

	for(;;)
	{
	    error = readline(&lb);

	    if(error || lb.buf == NULL)
		break;

	    error = execute(lb.buf);

	    if(error != 0)
		PrintFault(error, "execute:");
	}

	Close(lb.file);
    }
    else
	error=IoErr();

    return error;
}


int main (int argc, char ** argv)
{
    struct RDArgs *rda;
    STRPTR         args[2] = { "S:Shell-Startup", NULL };
    struct linebuf lb = { 0, NULL, 0, 0, 0, 0 };
    LONG           error = RETURN_OK;

    UtilityBase = OpenLibrary("utility.library", 39);

    if(UtilityBase)
    {
        lb.file = Input();

        cLock = Lock("C:", SHARED_LOCK);

        if(cLock)
        {
            cli = Cli();
            cli->cli_StandardInput  = cli->cli_CurrentInput  = Input();
            cli->cli_StandardOutput = cli->cli_CurrentOutput = Output();
            setpath(NULL);

            rda = ReadArgs("FROM,COMMAND/K/F", (IPTR *)args, NULL);

            if(rda != NULL)
            {
                if(args[1])
		{
		    cli->cli_Interactive = DOSFALSE;
		    cli->cli_Background  = DOSTRUE;
                    execute((STRPTR)args[1]);
		}
                else
                {
                    ULONG num = ((struct Process *)FindTask(NULL))->pr_TaskNum;

                    VPrintf("New Shell process %ld\n", &num);
                    Flush(Output());
                    cli->cli_Interactive = DOSTRUE;
		    cli->cli_Background  = DOSFALSE;
                    executefile((STRPTR)args[0]);

                    while(error == 0)
                    {
                        printPrompt();
                        error = readline(&lb);

                        if(error || lb.buf == NULL)
                            break;

                        error = execute(lb.buf);
                    }

                    if(error == -1)
                        error = 0;

                    VPrintf("Process %ld ending\n", &num);
                    Flush(Output());
                }

                FreeArgs(rda);
            }
	    else
            {
                PrintFault(IoErr(), "Shell");
                error = RETURN_FAIL;
            }

            UnLock(cLock);
        }
	else
        {
            PrintFault (IoErr(), "Shell");
            error = RETURN_FAIL;
        }

        CloseLibrary(UtilityBase);
    }
    else
    {
        VPrintf("Could not open utility.library\n", NULL);
        SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);
        error = RETURN_FAIL;
    }
    
    return error;
}
