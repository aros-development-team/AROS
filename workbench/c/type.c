/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 15:34:04  digulla
    #include <exec/execbase.h> was missing

    Revision 1.2  1996/08/01 17:40:46  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

CALLENTRY /* Before the first symbol */

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

static LONG tinymain(void);

LONG entry(struct ExecBase *sysbase)
{
    LONG error=RETURN_FAIL;
    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(DOSBase!=NULL)
    {
	error=tinymain();
	CloseLibrary((struct Library *)DOSBase);
    }
    return error;
}

#define BUFSIZE 8192

struct file
{
    BPTR fd;
    UBYTE *cur;
    ULONG cnt;
    LONG error;
    UBYTE buf[BUFSIZE];
};

const UBYTE hs[16]="0123456789abcdef";

#define putc(f,c) (*(f)->cur++=(c),--(f)->cnt?0:put(f))
static int put(struct file *f)
{
    LONG size,subsize;
    STRPTR buf;
    size=f->cur-f->buf;
    buf=f->buf;
    while(size)
    {
	subsize=Write(f->fd,buf,size);
	if(subsize<=0)
	{
	    f->error=IoErr();
	    return 1;
	}
	buf+=subsize;
	size-=subsize;
    }
    f->cur=f->buf;
    f->cnt=BUFSIZE;
    return 0;
}

#define getc(f) ((f)->cnt--?*(f)->cur++:get(f))
static int get(struct file *f)
{
    LONG size;
    size=Read(f->fd,f->buf,BUFSIZE);
    if(size<0)
	f->error=IoErr();
    if(size<=0)
	return -1;
    f->cnt=size-1;
    f->cur=f->buf;
    return *f->cur++;
}

static void putlinequick(struct file *f, ULONG offset, UBYTE *buf)
{
    int c, i, k;
    UBYTE *b, *o;
    o=f->cur;
    if(offset>=0x10000)
    {
	if(offset>=0x100000)
	{
	    if(offset>=0x1000000)
	    {
		if(offset>=0x10000000)
		{
		    *o++=hs[(offset>>28)&0xf];
		    f->cnt--;
		}
		*o++=hs[(offset>>24)&0xf];
		f->cnt--;
	    }
	    *o++=hs[(offset>>20)&0xf];
	    f->cnt--;
	}
	*o++=hs[(offset>>16)&0xf];
	f->cnt--;
    }
    *o++=hs[(offset>>12)&0xf];
    *o++=hs[(offset>>8)&0xf];
    *o++=hs[(offset>>4)&0xf];
    *o++=hs[offset&0xf];
    *o++=':';
    *o++=' ';
    b=buf;
    for(i=0;i<4;i++)
    {
	for(k=0;k<4;k++)
	{
	    c=*b++;
	    *o++=hs[c>>4];
	    *o++=hs[c&0xf];
	}
	*o++=' ';
    }
    b=buf;
    for(i=0;i<16;i++)
    {
	c=*b++;
	*o++=(c&0x7f)>=0x20&&c!=0x7f?c:'.';
    }
    *o++='\n';
    f->cur=o;
    f->cnt-=59;
}

static int putline(struct file *f, ULONG offset, UBYTE *buf, ULONG num)
{
    int c, i;
    UBYTE *b;
    if(offset>=0x10000)
    {
	if(offset>=0x10000000&&putc(f,hs[(offset>>28)&0xf]))
	    return 1;
	if(offset>=0x1000000&&putc(f,hs[(offset>>24)&0xf]))
	    return 1;
	if(offset>=0x100000&&putc(f,hs[(offset>>20)&0xf]))
	    return 1;
	if(offset>=0x10000&&putc(f,hs[(offset>>16)&0xf]))
	    return 1;
    }
    if(putc(f,hs[(offset>>12)&0xf]))
	return 1;
    if(putc(f,hs[(offset>>8)&0xf]))
	return 1;
    if(putc(f,hs[(offset>>4)&0xf]))
	return 1;
    if(putc(f,hs[offset&0xf]))
	return 1;
    if(putc(f,':'))
	return 1;
    if(putc(f,' '))
	return 1;
    b=buf;
    for(i=0;i<16;i++)
    {
	if(i<num)
	{
	    c=*b++;
	    if(putc(f,hs[c>>4]))
		return 1;
	    if(putc(f,hs[c&0xf]))
		return 1;
	}else
	{
	    if(putc(f,' '))
		return 1;
	    if(putc(f,' '))
		return 1;
	}
	if((i&3)==3)
	    if(putc(f,' '))
		return 1;
    }
    b=buf;
    for(i=0;i<num;i++)
    {
	c=*b++;
	if(putc(f,(c&0x7f)>=0x20&&c!=0x7f?c:'.'))
	    return 1;
    }
    if(putc(f,'\n'))
	return 1;
    return 0;
}

void hexdumpfile(struct file *in, struct file *out)
{
    UBYTE buf[16];
    UBYTE *b;
    LONG offset=0, n, c, tty;
    tty=IsInteractive(out->fd);
    for(;;)
    {
	if(in->cnt>16)
	{
	    b=in->cur;
	    n=16;
	    in->cur+=16;
	    in->cnt-=16;
	}else
	{
	    b=buf;
	    for(n=0;n<16;n++)
	    {
		c=getc(in);
		if(c<0)
		    break;
		b[n]=c;
	    }
	}
	if(n==16)
	{
	    if(out->cnt>=63)
		putlinequick(out,offset,b);
	    else
		if(putline(out,offset,b,n))
		    return;
	}else
	{
	    if(n)
		putline(out,offset,b,n);
	    if(out->cur!=out->buf)
		put(out);
	    return;
	}
	if(tty)
	    if(put(out))
		return;
	offset+=n;
    }
}

LONG dumpfile(struct file *in, struct file *out)
{
    LONG c;
    if(1/*IsInteractive(out->fd)*/)
	for(;;)
	{
	    c=getc(in);
	    if(c<0)
		return 0;
	    if(putc(out,c)||(c=='\n'&&put(out)))
		return 1;
	}
}

LONG tinymain(void)
{
    ULONG args[5]={ 0, 0, 0, 0, 0 };
    struct RDArgs *rda;
    struct file *in, *out;
    STRPTR *names;

    rda=ReadArgs("FROM/A/M,TO/K,OPT/K,HEX/S,NUMBER/S",args,NULL);
    if(rda==NULL)
    {
	PrintFault(IoErr(),"Type");
	return RETURN_FAIL;
    }
    names=(STRPTR *)args[0];

    in =AllocMem(sizeof(struct file),MEMF_ANY);
    out=AllocMem(sizeof(struct file),MEMF_ANY);

    if(in!=NULL&&out!=NULL)
    {
	out->cur=out->buf;
	out->cnt=BUFSIZE;
	out->fd=Output();
	while(*names!=NULL)
	{
	    in->fd=Open(*names,MODE_OLDFILE);
	    if(in->fd)
	    {
		in->cnt=0;
		if(args[3])
		    hexdumpfile(in,out);
		else
		    dumpfile(in,out);
		Close(in->fd);
	    }else
	    {
		PrintFault(IoErr(),"Type");
		break;
	    }
	    names++;
	}
    }else
	PrintFault(ERROR_NO_FREE_STORE,"Type");

    if(in!=NULL)
	FreeMem(in,sizeof(struct file));
    if(out!=NULL)
	FreeMem(out,sizeof(struct file));
    if(rda!=NULL)
	FreeArgs(rda);
    return 0;
}
