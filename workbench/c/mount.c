/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Mount CLI command
    Lang: english
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/rdargs.h>
#include <proto/dos.h>
#include <proto/utility.h>

static const char version[] = "$VER: mount 41.1 (19.2.1997)\n";

static struct UtilityBase *UtilityBase;

LONG readfile(STRPTR name, STRPTR *mem, LONG *size)
{
    BPTR ml;
    ULONG rest,sub;
    STRPTR buf;
    ml=Open(name,MODE_OLDFILE);
    if(ml)
    {
	if(Seek(ml,0,OFFSET_END)!=-1)
	{
	    *size=Seek(ml,0,OFFSET_BEGINNING);
	    if(*size!=-1)
	    {
		*mem=(STRPTR)AllocVec(*size,MEMF_ANY);
		if(*mem!=NULL)
		{
		    rest=*size;
		    buf=*mem;
		    for(;;)
		    {
			if(!rest)
			{
			    Close(ml);
			    return 0;
			}
			sub=Read(ml,buf,rest);
			if(sub==-1)
			    break;
			rest-=sub;
			buf+=sub;
		    }
		    FreeVec(*mem);
		}else
		    SetIoErr(ERROR_NO_FREE_STORE);
	    }
	}
	Close(ml);
    }
    return IoErr();
}

static void preparefile(STRPTR buf, LONG size)
{
    STRPTR end=buf+size;
    while(buf<end)
    {
	/* Convert comments to spaces */
	if(buf+1<end&&*buf=='/'&&buf[1]=='*')
	{
	    *buf++=' ';
	    *buf++=' ';
	    while(buf<end)
	    {
		if(*buf=='*')
		{
		    *buf++=' ';
		    if(buf>=end)
			break;
		    if(*buf=='/')
		    {
			*buf++=' ';
			break;
		    }
		}
		*buf++=' ';
	    }
	    continue;
	}
	/* Skip strings */
	if(*buf=='\"')
	{
	    while(buf<end&&*buf!='\"')
	    {
		if(*buf++=='*'&&buf<end)
		    buf++;
	    }
	    continue;
	}
	/* Convert '\n' and ';' to spaces */
	if(*buf=='\n'||*buf==';')
	{
	    *buf++=' ';
	    continue;
	}
	/* Convert '#' to NUL */
	if(*buf=='#')
	{
	    *buf++=0;
	    continue;
	}
	/* Skip all other characters */
	buf++;
    }
}

static LONG mountdevice(struct IOFileSys *iofs, STRPTR filesys, STRPTR device)
{
    struct DosList *entry;
    LONG error;
    entry=MakeDosEntry(device,DLT_DEVICE);
    if(entry!=NULL)
    {
	if(!OpenDevice(filesys,0,&iofs->IOFS,0))
	{
	    if(AddDosEntry(entry))
	    {
		entry->dol_Unit  =iofs->IOFS.io_Unit;
		entry->dol_Device=iofs->IOFS.io_Device;
		/*
		    Neither close the device nor free the DosEntry.
		    Both will stay in the dos list as long as the
		    device is mounted.
		*/
		return 0;
	    }
	    else
		error=IoErr();
	}
	else
	    error=ERROR_OBJECT_NOT_FOUND;

	FreeDosEntry(entry);
    }
    else
	error=ERROR_NO_FREE_STORE;

    return error;
}

static const UBYTE options[]=
"HANDLER=FILESYSTEM/A/K,DEVICE/K,UNIT/K/N,BLOCKSIZE/K/N,SURFACES/K/N,"
"BLOCKSPERTRACK/K/N,RESERVED/K/N,INTERLEAVE/K/N,LOWCYL/K/N,HIGHCYL/K/N,"
"BUFFERS/K/N,BUFMEMTYPE/K/N,MAXTRANSFER/K/N,MASK/K/N,BOOTPRI/K/N,"
"DOSTYPE/K/N,BAUD/K/N,CONTROL/K";

static LONG mount(STRPTR name, STRPTR buf, LONG size, struct IOFileSys *iofs)
{
    IPTR *args[18]=
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    IPTR vec[20];
    UBYTE buffer[1024];
    LONG error, res;
    STRPTR end=buf+size, s2;
    struct RDArgs *rd;
    struct RDArgs rda=
    { { NULL, 0, 0 }, 0, NULL, 0, NULL, RDAF_NOPROMPT };
    rda.RDA_Source.CS_Buffer=buf;
    rda.RDA_Source.CS_Length=end-buf;
    rda.RDA_Source.CS_CurChr=0;
    while(rda.RDA_Source.CS_CurChr<rda.RDA_Source.CS_Length)
    {
	res=ReadItem(buffer,1024,&rda.RDA_Source);
	if(res==ITEM_ERROR)
	    return IoErr();
	if(res==ITEM_NOTHING&&rda.RDA_Source.CS_CurChr==rda.RDA_Source.CS_Length)
	    return 0;
	if(res!=ITEM_QUOTED&&res!=ITEM_UNQUOTED)
	    return 1;
	s2=buffer;
	while(*s2)
	    s2++;
	if(s2==buffer||s2[-1]!=':')
	    return 1;
	*--s2=0;
	if(!Strnicmp(name,buffer,s2-buffer)&&
	   (!name[s2-buffer]||(name[s2-buffer]==':'||!name[s2-buffer+1])))
	{
	    rd=ReadArgs((STRPTR)options,(LONG *)args,&rda);
	    if(rd==NULL)
		return IoErr();
	    vec[DE_TABLESIZE]	=19;
	    vec[DE_SIZEBLOCK]	=(args[3]?*args[3]:512)/4;
	    vec[DE_BLOCKSIZE]	=args[3]?*args[3]:512;
	    vec[DE_NUMHEADS]	=args[4]?*args[4]:2;
	    vec[DE_BLKSPERTRACK]=args[5]?*args[5]:11;
	    vec[DE_RESERVEDBLKS]=args[6]?*args[6]:2;
	    vec[DE_INTERLEAVE]	=args[7]?*args[7]:0;
	    vec[DE_LOWCYL]	=args[8]?*args[8]:0;
	    vec[DE_HIGHCYL]	=args[9]?*args[9]:79;
	    vec[DE_NUMBUFFERS]	=args[10]?*args[10]:20;
	    vec[DE_BUFMEMTYPE]	=args[11]?*args[11]:1;
	    vec[DE_MAXTRANSFER] =args[12]?*args[12]:~0ul;
	    vec[DE_MASK]	=args[13]?*args[13]:~0ul;
	    vec[DE_BOOTPRI]	=args[14]?*args[14]:0;
	    vec[DE_DOSTYPE]	=args[15]?*args[15]:0x444f5301;
	    vec[DE_BAUD]	=args[16]?*args[16]:9600;
	    vec[DE_CONTROL]	=args[17]?*args[17]:(IPTR)"";
	    vec[DE_BOOTBLOCKS]	=0;
	    iofs->io_Union.io_OpenDevice.io_DeviceName=args[1]?(STRPTR)args[1]:(STRPTR)"trackdisk.device";
	    iofs->io_Union.io_OpenDevice.io_Unit      =(ULONG)args[2]?*args[2]:0;
	    iofs->io_Union.io_OpenDevice.io_Environ   =vec;
	    error=mountdevice(iofs,(STRPTR)args[0],buffer);
	    FreeArgs(rd);
	    return error;
	}
	while(rda.RDA_Source.CS_CurChr<rda.RDA_Source.CS_Length)
	    if(!rda.RDA_Source.CS_Buffer[rda.RDA_Source.CS_CurChr++])
		break;
    }
    return 1;
}

int main (int argc, char ** argv)
{
    STRPTR args[2]={ NULL, "Devs:Mountlist" };
    struct RDArgs *rda;
    struct IOFileSys *iofs;
    STRPTR mem;
    LONG size;
    struct Process *me=(struct Process *)FindTask(NULL);
    LONG error=0;

    UtilityBase=(struct UtilityBase *)OpenLibrary("utility.library",0);
    if(UtilityBase!=NULL)
    {
	rda=ReadArgs("DEVICE/M,FROM/K",(ULONG *)args,NULL);
	if(rda!=NULL)
	{
	    error=readfile(args[1],&mem,&size);
	    if(!error)
	    {
		preparefile(mem,size);
		iofs=(struct IOFileSys *)CreateIORequest(&me->pr_MsgPort,sizeof(struct IOFileSys));
		if(iofs!=NULL)
		{
		    STRPTR *dev=(STRPTR *)args[0];
		    while(*dev)
		    {
			error=mount(*dev++,mem,size,iofs);
			if(error)
			    break;
		    }
		    DeleteIORequest(&iofs->IOFS);
		}else
		    error=ERROR_NO_FREE_STORE;
		FreeVec(mem);
	    }
	    FreeArgs(rda);
	}else
	    error=IoErr();
        CloseLibrary((struct Library *)UtilityBase);
    }else
	error=ERROR_OBJECT_NOT_FOUND;
    if(error)
    {
	PrintFault(error,"Mount");
	return RETURN_FAIL;
    }
    return RETURN_OK;
}
