/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/09/13 17:52:10  digulla
    Use IPTR

    Revision 1.4  1996/09/12 14:49:25  digulla
    More checks
    Better code to specify size of buffer

    Revision 1.3  1996/08/01 17:40:45  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>

/* Don't define symbols before the entry point. */
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern const char dosname[];
static LONG tinymain(void);

__AROS_LH0(LONG,entry,struct ExecBase *,sysbase,,)
{
    __AROS_FUNC_INIT
    LONG error=RETURN_FAIL;

    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary((STRPTR)dosname,39);
    if(DOSBase!=NULL)
    {
	error=tinymain();
	CloseLibrary((struct Library *)DOSBase);
    }
    return error;
    __AROS_FUNC_EXIT
}

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
const char dosname[]="dos.library";

static LONG tinymain(void)
{
    char *args[1]={ 0 };
    struct RDArgs *rda;
    BPTR dir;
    LONG i;
    LONG loop;
    struct ExAllControl *eac;
    struct ExAllData *ead;
    static UBYTE buffer[4096];
    UBYTE flags[9];
    IPTR argv[5];
    LONG error=0, rc=RETURN_OK;

    rda=ReadArgs("DIR",(IPTR *)args,NULL);
    if(rda!=NULL)
    {
	dir=Lock(args[0]!=NULL?args[0]:"",SHARED_LOCK);
	if(dir)
	{
	    LONG files=0, dirs=0, blocks=0;
	    eac=AllocDosObject(DOS_EXALLCONTROL,NULL);
	    if(eac!=NULL)
	    {
		eac->eac_LastKey=0;
		do
		{
		    loop=ExAll(dir,(struct ExAllData *)buffer,sizeof(buffer),ED_COMMENT,eac);
		    if(!loop&&IoErr()!=ERROR_NO_MORE_ENTRIES)
		    {
			error=RETURN_ERROR;
			break;
		    }
		    if(eac->eac_Entries)
		    {
			ead=(struct ExAllData *)buffer;
			do
			{
			    UBYTE date[LEN_DATSTRING];
			    UBYTE time[LEN_DATSTRING];
			    struct DateTime dt;
			    dt.dat_Stamp.ds_Days  =ead->ed_Days;
			    dt.dat_Stamp.ds_Minute=ead->ed_Mins;
			    dt.dat_Stamp.ds_Tick  =ead->ed_Ticks;
			    dt.dat_Format =FORMAT_DOS;
			    dt.dat_Flags  =DTF_SUBST;
			    dt.dat_StrDay =NULL;
			    dt.dat_StrDate=date;
			    dt.dat_StrTime=time;
			    DateToStr(&dt); /* returns 0 if invalid */
			    ead->ed_Prot^=0xf;
			    for(i=0;i<7;i++)
				if(ead->ed_Prot&(64>>i))
				    flags[i]="sparwed"[i];
				else
				    flags[i]='-';

			    flags[i] = 0;

			    argv[0]=(IPTR)ead->ed_Name;
			    if(ead->ed_Type>=0)
			    {
				argv[1]=(IPTR)flags;
				argv[2]=(IPTR)date;
				argv[3]=(IPTR)time;
				if(VPrintf("%-25.s   <Dir> %7.s %s %s\n",argv)<0)
				{
				    error=RETURN_ERROR;
				    loop=0;
				    break;
				}
				dirs++;
			    }else
			    {
				argv[1]=ead->ed_Size;
				argv[2]=(IPTR)flags;
				argv[3]=(IPTR)date;
				argv[4]=(IPTR)time;
				if(VPrintf("%-25.s %7.ld %7.s %s %s\n",argv)<0)
				{
				    error=RETURN_ERROR;
				    loop=0;
				    break;
				}
				blocks+=ead->ed_Size;
				files++;
			    }
			    ead=ead->ed_Next;
			}while(ead!=NULL);
		    }
		}while(loop);
		if(!error)
		{
		    argv[0]=files;
		    argv[1]=dirs;
		    argv[2]=blocks;
		    if(VPrintf("%ld files - %ld directories - %ld bytes used\n",argv)<0)
			error=RETURN_ERROR;
		}
		FreeDosObject(DOS_EXALLCONTROL,eac);
	    }else
	    {
		SetIoErr(ERROR_NO_FREE_STORE);
		error=RETURN_ERROR;
	    }
	    UnLock(dir);
	}
	else
	{
	    PrintFault(IoErr(),"List: Lock failed");
	    rc = RETURN_ERROR;
	}

	FreeArgs(rda);
    }
    else
    {
	error=RETURN_FAIL;
    }

    if (error)
	PrintFault(IoErr(),"List");

    return rc;
}
