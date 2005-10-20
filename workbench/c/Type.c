/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Type CLI command
    Lang: English
*/
#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#include <string.h>

static const char version[] = "$VER: Type 42.0 (20.10.2005)\n";

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

LONG hexdumpfile(struct file *in, struct file *out)
{
    UBYTE buf[16];
    UBYTE *b;
    LONG offset=0, n, c, tty;
    LONG retval = RETURN_OK;
    
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
		{
		    break;
		}
		b[n]=c;
	    }
	}
	if(n==16)
	{
	    if(out->cnt>=63)
		putlinequick(out,offset,b);
	    else
		if(putline(out,offset,b,n))
		{
		    retval = RETURN_ERROR;
		    break;
		}
	}else
	{
	    if(n)
		putline(out,offset,b,n);
	    if(out->cur!=out->buf)
		put(out);
	    break;
	}
	if(tty)
	    if(put(out))
	    {
	    	retval = RETURN_ERROR;
		break;
	    }
	offset+=n;
	
	if (CheckSignal(SIGBREAKF_CTRL_C))
	{
	    retval = RETURN_WARN;
	    break;
	}
    }
    
    return retval;
    
}

void putlinenumber(struct file * out, unsigned short line)
{
  int x = 10000;
  BOOL s = FALSE;

  while (x)
  {
    if (line / x || s)
    {
      putc(out, line/x+'0');
      line %= x;
      s = TRUE;
    }
    else
      putc(out, ' ');

    x/=10;
  }  
  
  putc(out, ' ');
}

LONG dumpfile(struct file *in, struct file *out, BOOL showline)
{
    LONG c, lastc = 0;
    unsigned short line = 0;
    LONG retval = RETURN_OK;
    
    if (showline)
      putlinenumber(out, ++line);
    
    if(1/*IsInteractive(out->fd)*/)
	for(;;)
	{
	    c=getc(in);

	    if(c<0)
	    {
	        if (lastc!='\n')
	          putc(out, '\n');

                put(out);
		break;
            }

	    if (lastc==0x0a && showline)
	      putlinenumber(out, ++line);

	    if(putc(out,c)||(c=='\n' && put(out)))
	    {
	        if (c!='\n')
	          putc(out, '\n');

	        put(out);
		retval = RETURN_ERROR;
		break;
	    }
	
	    if ((c == '\n') && CheckSignal(SIGBREAKF_CTRL_C))
	    {
	    	retval = RETURN_WARN;
		break;
	    }
	    lastc = c;
	}
	
    return retval;
}

static LONG processfile(CONST_STRPTR name, struct file *in, struct file *out, IPTR *args, LONG *numfiles)
{
	LONG error = 0;

	in->fd = Open(name, MODE_OLDFILE);
	if (in->fd)
	{
		(*numfiles)++;
		in->cnt = 0;

		if (args[2])
			error = hexdumpfile(in, out);
		else
			error = dumpfile(in, out, args[3]);

		Close(in->fd);
	}
	else
		error = IoErr();

	return error;
}

int __nocommandline;

int main (void)
{
    IPTR args[5]={ 0, 0, 0, 0, 0 };
    struct RDArgs *rda;
    struct file *in, *out;
    STRPTR *names;
    int retcode = RETURN_OK;
    struct AnchorPath apath;
    
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
	apath.ap_BreakBits  = SIGBREAKF_CTRL_C;
	apath.ap_FoundBreak = 0;
	apath.ap_Flags      = 0;
	apath.ap_Strlen     = MAX_PATH_LEN;
	if (args[1])
		out->fd = Open((STRPTR) args[1], MODE_NEWFILE);
	else
		out->fd=Output();
	if (out->fd)
	{
		while(*names!=NULL)
		{
			ULONG numfiles = 0;
			LONG error;

			for (error = MatchFirst(*names, &apath);
			     !error;
			     error = MatchNext(&apath))
			{
				error = processfile(apath.ap_Buf, in, out, args, &numfiles);
				if (error)
					break;
			}
			MatchEnd(&apath);

			if (numfiles == 0 && error == ERROR_NO_MORE_ENTRIES)
			{
				error = -1;
			}

			if (error && error != ERROR_NO_MORE_ENTRIES)
			{
				if (*names && error != ERROR_BREAK)
				{
					Printf("TYPE: can't open %s\n", (LONG) *names);
				}

				if (error != -1)
				{
					PrintFault(error, NULL);
					SetIoErr(error);
				}

				break;
			}
		names++;
		}

		if (args[ARG_TO])
			Close(data->out.fd);

		/* If all files got dumped, return ok, else error.
		*/
		retval = *names ? RETURN_ERROR : RETURN_OK;
	} else
	{
		PrintFault(IoErr(), NULL);
	}
    }else
    {
	PrintFault(ERROR_NO_FREE_STORE,"Type");
	retcode = RETURN_ERROR;
    }

    if(in!=NULL)
	FreeMem(in,sizeof(struct file));
    if(out!=NULL)
	FreeMem(out,sizeof(struct file));
    if(rda!=NULL)
	FreeArgs(rda);
	
    return retcode;
}
