/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.9  1996/10/04 14:35:14  digulla
    Search C: without path C: add

    Revision 1.8  1996/09/17 16:43:01  digulla
    Use general startup code

    Revision 1.7  1996/09/17 16:09:34  digulla
    Another failure to make SIGALRM work :(

    Revision 1.6  1996/09/13 17:52:11  digulla
    Use IPTR

    Revision 1.5  1996/09/12 14:47:55  digulla
    More stack

    Revision 1.4  1996/08/30 17:02:03  digulla
    Fixed a bug which caused the shell to exit if the timer sent a signal. This
	fix is a very bad hack :(

    Revision 1.3  1996/08/23 17:05:58  digulla
    Increased the stack.

    Revision 1.2  1996/08/01 17:40:46  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <exec/libraries.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

BPTR cLock;

static void printpath(void)
{
    BPTR dir;
    STRPTR buf;
    ULONG i;
    dir=CurrentDir(0);
    for(i=256;;i+=256)
    {
	buf=AllocVec(i,MEMF_ANY);
	if(buf==NULL)
	    break;
	if(NameFromLock(dir,buf,i))
	{
	    FPuts(Output(),buf);
	    FreeVec(buf);
	    break;
	}
	FreeVec(buf);
    }
    CurrentDir(dir);
}

static void prompt(void)
{
    printpath();
    FPuts(Output(),"> ");
    Flush(Output());
}

struct linebuf
{
    BPTR file;
    UBYTE *buf;
    ULONG size;
    ULONG bend;
    ULONG lend;
    LONG eof;
};

static LONG readline(struct linebuf *lb)
{
    if(lb->bend)
    {
	UBYTE *src=lb->buf+lb->lend, *dst=lb->buf;
	ULONG i=lb->bend-lb->lend;
	if(i)
	    do
		*dst++=*src++;
	    while(--i);
	lb->bend-=lb->lend;
    }
    lb->lend=0;
    for(;;)
    {
	LONG subsize;
	for(;lb->lend<lb->bend;)
	    if(lb->buf[lb->lend++]=='\n')
	    {
		lb->buf[lb->lend-1]='\0';
		return 0;
	    }
	if(lb->eof)
	{
	    FreeVec(lb->buf);
	    lb->buf=NULL;
	    lb->eof=0;
	    return 0;
	}
	if(lb->bend>=lb->size)
	{
	    UBYTE *newbuf;
	    newbuf=AllocVec(lb->size+256,MEMF_ANY);
	    if(newbuf==NULL)
	    {
		FreeVec(lb->buf);
		lb->buf=NULL;
		return ERROR_NO_FREE_STORE;
	    }
	    CopyMem(lb->buf,newbuf,lb->bend);
	    FreeVec(lb->buf);
	    lb->buf=newbuf;
	    lb->size+=256;
	}
	subsize=Read(lb->file,lb->buf+lb->bend,lb->size-lb->bend);
	if(subsize==0)
	{
	    if(lb->bend)
		lb->buf[lb->bend++]='\n';
	    lb->eof=1;
	}else if(subsize<0)
	{
	    /*ada 30.8.96 This is a REALLY BAD hack !! */
	    if (IoErr() != -1 && IoErr() != 0)
	    {
		ULONG error = IoErr();

		VPrintf ("Shell:Read %ld\n", &error);
		PrintFault(IoErr(), "Shell:Read");
		FreeVec(lb->buf);
		lb->buf=NULL;
		return IoErr();
	    }
	}else
	    lb->bend+=subsize;
    }
}

BPTR loadseg(STRPTR name)
{
    BPTR old, *cur, seg;
    STRPTR s;
    seg=LoadSeg(name);
    if(seg)
	return seg;
    s=name;
    while(*s)
    {
	if(*s==':'||*s=='/')
	    return 0;
	s++;
    }
    old=CurrentDir(0);
    cur=(BPTR *)BADDR(Cli()->cli_CommandDir);
    while(cur!=NULL)
    {
	(void)CurrentDir(cur[1]);
	seg=LoadSeg(name);
	if(seg)
	    break;
	cur=(BPTR *)BADDR(cur[0]);
    }
    if (!seg)
    {
	(void)CurrentDir(cLock);
	seg=LoadSeg(name);
    }
    CurrentDir(old);
    return seg;
}

LONG execute(STRPTR com)
{
    STRPTR s1=NULL, s2=NULL;
    STRPTR args, rest, command=NULL, infile=NULL, outfile=NULL;
    STRPTR last;
    BPTR in=0, out=0;
    BPTR seglist, lock;
    struct FileInfoBlock *fib;
    LONG res, size, error=0;
    struct CSource cs;

    last=com;
    while(*last++)
	;
    args=s1=(STRPTR)AllocVec(last-com+1,MEMF_ANY);
    rest=s2=(STRPTR)AllocVec(last-com+1,MEMF_ANY);
    if(args==NULL||rest==NULL)
    {
	error=ERROR_NO_FREE_STORE;
	goto end;
    }

    cs.CS_Buffer=com;
    cs.CS_Length=last-com-1;
    cs.CS_CurChr=0;

    for(;;)
    {
	while(com[cs.CS_CurChr]==' '||com[cs.CS_CurChr]=='\t')
	    *rest++=com[cs.CS_CurChr++];
	if(com[cs.CS_CurChr]==';')
	    break;
	if(command==NULL)
	    command=args;
	else if(com[cs.CS_CurChr]=='<')
	    infile=args+1;
	else if(com[cs.CS_CurChr]=='>')
	    outfile=args+1;
	else
	{
	    size=cs.CS_CurChr;
	    res=ReadItem(args,~0ul/2,&cs);
	    while(size<cs.CS_CurChr)
		*rest++=com[size++];
	    if(res==ITEM_NOTHING||res==ITEM_ERROR)
	    {
		*rest++=0;
		break;
	    }
	    continue;
	}
	res=ReadItem(args,~0ul/2,&cs);
	if(res!=ITEM_QUOTED&&res!=ITEM_UNQUOTED)
	    break;
	while(*args++)
	    ;
    }
    if(command==NULL||!*command)
	goto end;
    if(infile!=NULL)
    {
	in=Open(infile,MODE_OLDFILE);
	if(!in)
	{
	    infile=NULL;
	    error=IoErr();
	    goto end;
	}
	in=SelectInput(in);
    }
    if(outfile!=NULL)
    {
	out=Open(outfile,MODE_NEWFILE);
	if(!out)
	{
	    outfile=NULL;
	    error=IoErr();
	    goto end;
	}
	out=SelectOutput(out);
    }
    seglist=loadseg(command);
    if(seglist)
    {
	last=s2;
	while(*last++)
	    ;
	RunCommand(seglist,100000,s2,last-s2-1);
	UnLoadSeg(seglist);
    }else if(infile==NULL&&outfile==NULL)
    {
	lock=Lock(command,SHARED_LOCK);
	if(lock)
	{
	    fib=AllocDosObject(DOS_FIB,NULL);
	    if(fib!=NULL)
	    {
		if(Examine(lock,fib))
		{
		    if(fib->fib_DirEntryType>0)
			lock=CurrentDir(lock);
		    else
			SetIoErr(error=ERROR_OBJECT_WRONG_TYPE);
		}
		FreeDosObject(DOS_FIB,fib);
	    }
	    UnLock(lock);
	}
	else
	    error = IoErr ();
    }

end:
    if(infile!=NULL)
	Close(SelectInput(in));
    if(outfile!=NULL)
	Close(SelectOutput(out));
    FreeVec(s1);
    FreeVec(s2);
    if(error)
	PrintFault(error,"Couldn't run command");
    Flush(Output());
    return 0;
}

int executefile(STRPTR name)
{
    struct linebuf lb = { 0, NULL, 0, 0, 0, 0 };
    LONG error=0;

    lb.file=Open(name,MODE_OLDFILE);
    if(lb.file)
    {
	for(;;)
	{
	    error=readline(&lb);
	    if(error||lb.buf==NULL)
		break;
	    error = execute(lb.buf);
	    if (error != 0)
		PrintFault (error, "execute:");
	}
	Close(lb.file);
    }else
	error=IoErr();
    return error;
}

int main (int argc, char ** argv)
{
    struct RDArgs *rda;
    STRPTR args[2]={ "S:Shell-Startup", NULL };
    struct linebuf lb = { 0, NULL, 0, 0, 0, 0 };
    LONG error=0;

    lb.file=Input();

    cLock = Lock("C:", SHARED_LOCK);
    if (!cLock)
    {
	PrintFault (IoErr(), "Shell");
	return RETURN_ERROR;
    }

    rda=ReadArgs("FROM,COMMAND/K/F",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	if(args[1])
	    execute((STRPTR)args[1]);
	else
	{
	    ULONG num=((struct Process *)FindTask(NULL))->pr_TaskNum;
	    VPrintf("New Shell process %ld\n",&num);
	    Flush(Output());
	    executefile((STRPTR)args[0]);
	    for(;;)
	    {
		prompt();
		error=readline(&lb);
		if(error||lb.buf==NULL)
		    break;
		execute(lb.buf);
	    }
	    VPrintf("Process %ld ending\n",&num);
	    Flush(Output());
	}
	FreeArgs(rda);
    }

    UnLock (cLock);

    return 0;
}
