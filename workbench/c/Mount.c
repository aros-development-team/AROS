/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Mount CLI command.
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/filehandler.h>
#include <dos/rdargs.h>
#include <dos/bptr.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/expansion.h>
#include <libraries/expansion.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <string.h>

# define   DEBUG 1
# include  <aros/debug.h>

static const char version[] = "$VER: Mount 41.2 (27.10.2001)\n";

const char *SearchTable[]=
{
	"",
	"DEVS:DOSDrivers/",
	"SYS:Storage/DOSDrivers/",
	NULL
};

extern struct Library     *ExpansionBase;

LONG readfile(STRPTR name, STRPTR *mem, LONG *size)
{
    BPTR ml;
    ULONG rest,sub;
    STRPTR buf;

    ml = Open(name, MODE_OLDFILE);

    if (ml != NULL)
    {
	if (Seek(ml, 0, OFFSET_END) != -1)
	{
	    *size = Seek(ml, 0, OFFSET_BEGINNING);

	    if (*size != -1)
	    {
		*mem = (STRPTR)AllocVec(*size+1, MEMF_ANY);

		if (*mem != NULL)
		{
		    rest = *size;
		    buf = *mem;

		    for (;;)
		    {
			if (!rest)
			{
			    Close(ml);
			    *buf = '\0';
			    return 0;
			}

			sub = Read(ml, buf, rest);

			if (sub == -1)
			{
			    break;
			}

			rest -= sub;
			buf += sub;
		    }

		    FreeVec(*mem);
		}
		else
		{
		    SetIoErr(ERROR_NO_FREE_STORE);
		}
	    }
	}

	Close(ml);
    }

    return IoErr();
}


static void preparefile(STRPTR buf, LONG size)
{
    STRPTR end = buf + size;

    while (buf < end)
    {
	/* Convert comments to spaces */
	if (buf + 1 < end && *buf == '/' && buf[1] == '*')
	{
	    *buf++ = ' ';
	    *buf++ = ' ';

	    while (buf < end)
	    {
		if (*buf == '*')
		{
		    *buf++ = ' ';

		    if (buf >= end)
		    {
			break;
		    }

		    if (*buf == '/')
		    {
			*buf++ = ' ';
			break;
		    }
		}
		
		*buf++=' ';
	    }

	    continue;
	}
	
	/* Skip strings */
	if (*buf == '\"')
	{
	    while (buf < end && *buf != '\"')
	    {
		if (*buf++ == '*' && buf < end)
		{
		    buf++;
		}
	    }

	    continue;
	}

	/* Convert '\n' and ';' to spaces */
	if (*buf == '\n' || *buf == ';')
	{
	    *buf++ = ' ';
	    continue;
	}

	/* Convert '#' to NUL */
	if (*buf == '#')
	{
	    *buf++ = 0;
	    continue;
	}

	/* Skip all other characters */
	buf++;
    }
}


BSTR DuplicateBSTRVolumeName(STRPTR name)
{
    int    i;			     /* Loop variable */
    int    length = strlen(name);
    STRPTR copy;

    if (name[length - 1] == ':')
    {
	length--;
    }

    copy = AllocVec(length + 2, MEMF_PUBLIC | MEMF_CLEAR);

    if (copy == NULL)
    {
	return NULL;
    }

    for (i = 0; i < length; i++)
    {
	AROS_BSTR_putchar(copy, i, name[i]);
    }

    AROS_BSTR_setstrlen(copy, length);

    return MKBADDR(copy);
}




static const UBYTE options[]=
"HANDLER=FILESYSTEM/A/K,DEVICE/K,UNIT/K/N,BLOCKSIZE/K/N,SURFACES/K/N,SECTORPERBLOCK/K/N,"
"BLOCKSPERTRACK/K/N,RESERVED/K/N,INTERLEAVE/K/N,LOWCYL/K/N,HIGHCYL/K/N,"
"BUFFERS/K/N,BUFMEMTYPE/K/N,MAXTRANSFER/K/N,MASK/K/N,BOOTPRI/K/N,"
"DOSTYPE/K/N,BAUD/K/N,CONTROL/K";

LONG mount (STRPTR name, struct RDArgs *rda)
{
    IPTR *args[19]=
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    IPTR *params;	     /* MakeDosNode() paramPacket */
    struct DosEnvec *vec;
    struct RDArgs *rd;
    LONG error;

    params = AllocVec(sizeof(struct DosEnvec) + sizeof(IPTR)*4,
		      MEMF_PUBLIC | MEMF_CLEAR);

    if (params == NULL)
    {
	return ERROR_NO_FREE_STORE;
    }

    vec = (struct DosEnvec *)&params[4];

    rd = ReadArgs((STRPTR)options, (IPTR *)args, rda);

    if (rd == NULL)
    {
	FreeVec(params);
	return IoErr();
    }

    vec->de_TableSize      = (IPTR)19;
    vec->de_SizeBlock      = (IPTR)(args[3]  ? *args[3]  : 512)/4;
    vec->de_SecOrg         = args[3]  ? (IPTR)*args[3]  : (IPTR)512;
    vec->de_Surfaces       = args[4]  ? (IPTR)*args[4]  : (IPTR)2;
    vec->de_SectorPerBlock = args[5]  ? (IPTR)*args[5]  : (IPTR)1;
    vec->de_BlocksPerTrack = args[6]  ? (IPTR)*args[6]  : (IPTR)11;
    vec->de_Reserved       = args[7]  ? (IPTR)*args[7]  : (IPTR)2;
    vec->de_Interleave     = args[8]  ? (IPTR)*args[8]  : (IPTR)0;
    vec->de_LowCyl         = args[9]  ? (IPTR)*args[9]  : (IPTR)0;
    vec->de_HighCyl        = args[10] ? (IPTR)*args[10]  : (IPTR)79;
    vec->de_NumBuffers     = args[11] ? (IPTR)*args[11] : (IPTR)20;
    vec->de_BufMemType     = args[12] ? (IPTR)*args[12] : (IPTR)1;
    vec->de_MaxTransfer    = args[13] ? (IPTR)*args[13] : (IPTR)~0ul;
    vec->de_Mask           = args[14] ? (IPTR)*args[14] : (IPTR)~0ul;
    vec->de_BootPri        = args[15] ? (IPTR)*args[15] : (IPTR)0;
    vec->de_DosType        = args[16] ? (IPTR)*args[16] : (IPTR)0x444f5301;
    vec->de_Baud           = args[17] ? (IPTR)*args[17] : (IPTR)9600;
    vec->de_Control        = args[18] ? (IPTR)*args[18] : (IPTR)"";
    vec->de_BootBlocks     = 0;

    params[0] = (IPTR)args[0];
    params[1] = (IPTR)args[1];
    params[2] = (IPTR)args[2] ? *args[2] : 0;
    params[3] = 0;

    /* NOTE: The 'params' may not be freed! */

    {
	struct DeviceNode *dn = MakeDosNode(params);

	if (dn != NULL)
	{
	    /* Use the name found in the mountlist */
	    dn->dn_OldName = DuplicateBSTRVolumeName(name);

	    if (dn->dn_OldName != NULL)
	    {
		dn->dn_NewName = AROS_BSTR_ADDR(dn->dn_OldName);

		if (AddDosNode(vec->de_BootPri, ADNF_STARTPROC, dn))
		{
		    FreeArgs(rd);
		    return 0;
		}
		else
		    error = ERROR_INVALID_RESIDENT_LIBRARY;
	    }
	    else
	    {
		error = ERROR_NO_FREE_STORE;
	    }
	}
	else
	{
	    error = ERROR_NO_FREE_STORE;
	}
    }
    FreeVec(params);
    FreeArgs(rd);

    return error;
}

static LONG parsemountlist(STRPTR name, STRPTR buf, LONG size)
{
    UBYTE  buffer[1024];
    LONG res;
    STRPTR end = buf + size;
    STRPTR s2;
    struct RDArgs rda = { { NULL, 0, 0 }, 0, NULL, 0, NULL, RDAF_NOPROMPT };

    rda.RDA_Source.CS_Buffer = buf;
    rda.RDA_Source.CS_Length = end - buf;
    rda.RDA_Source.CS_CurChr = 0;

    while (rda.RDA_Source.CS_CurChr < rda.RDA_Source.CS_Length)
    {
	res = ReadItem(buffer, 1024, &rda.RDA_Source);

	if (res == ITEM_ERROR)
	{
	    return IoErr();
	}

	if (res == ITEM_NOTHING &&
	    rda.RDA_Source.CS_CurChr == rda.RDA_Source.CS_Length)
	{
	    return 0;
	}

	if (res != ITEM_QUOTED && res != ITEM_UNQUOTED)
	{
	    return 1;
	}

	s2 = buffer;

	while (*s2)
	{
	    s2++;
	}

	if (s2 == (STRPTR)buffer || s2[-1] != ':')
	{
	    return 1;
	}

	*--s2 = 0;

	if (!Strnicmp(name, buffer, s2 - (STRPTR)buffer) &&
	   (!name[s2 - (STRPTR)buffer] || (name[s2 - (STRPTR)buffer] == ':' ||
				   !name[s2 - (STRPTR)buffer + 1])))
	{
		res = mount (buffer, &rda);

		return res;
	}
	while (rda.RDA_Source.CS_CurChr < rda.RDA_Source.CS_Length)
	{
	    if (!rda.RDA_Source.CS_Buffer[rda.RDA_Source.CS_CurChr++])
	    {
		break;
	    }
	}
    }

    return 1;
}

static LONG parsemountfile(STRPTR name, STRPTR buf, LONG size)
{
    STRPTR end = buf + size;
    struct RDArgs rda = { { NULL, 0, 0 }, 0, NULL, 0, NULL, RDAF_NOPROMPT };

    rda.RDA_Source.CS_Buffer = buf;
    rda.RDA_Source.CS_Length = end - buf;
    rda.RDA_Source.CS_CurChr = 0;

    return mount (name, &rda);
}

int __nocommandline;

int UtilityBase_version = 0;
int ExpansionBase_version = 0;

int main(void)
{
  extern struct WBStartup *WBenchMsg;
  STRPTR args[2] = { NULL, NULL };
  STRPTR mem, mem2;
  LONG   size, size2;
  LONG   error = 0;
  LONG   error2;
  int    i;
  char   name[512];
  struct RDArgs    *rda;

  if (WBenchMsg)
  {
      if (WBenchMsg->sm_NumArgs >= 2)
      {
            for (i = 1; i < WBenchMsg->sm_NumArgs; i++)
            {
                BPTR olddir;

                olddir = CurrentDir(WBenchMsg->sm_ArgList[i].wa_Lock);
                error=readfile(WBenchMsg->sm_ArgList[i].wa_Name, &mem, &size);
                CurrentDir(olddir);
		if (!error)
		{
			preparefile(mem, size);
			error = parsemountfile(WBenchMsg->sm_ArgList[i].wa_Name, mem, size);
			FreeVec(mem);
		}
		if (error != ERROR_OBJECT_NOT_FOUND)
			break;
            }
      }
  }
  else
  {
    rda = ReadArgs("DEVICE/M,FROM/K", (ULONG *)args, NULL);

    if (rda != NULL)
    {
	if (args[1])
	{
		error = readfile(args[1], &mem, &size);

		if (!error)
		{
		    STRPTR *dev = (STRPTR *)args[0];
		    preparefile(mem, size);

		    if (dev) while (*dev)
		    {
			error = parsemountlist(*dev++, mem, size);

			if (error)
			{
			    break;
			}
		    }
		    FreeVec(mem);
		}
	}
	else
	{
		STRPTR *dev = (STRPTR *)args[0];
		error2 = readfile("DEVS:Mountlist", &mem, &size);
		if ((!error2) || (error2 == ERROR_OBJECT_NOT_FOUND))
		{
			if (!error2)
				preparefile(mem, size);
			if (dev) while (*dev)
			{
		                for (i=0; SearchTable[i]; i++)
        		        {
					strcpy(name,SearchTable[i]);
					strcat(name, *dev);
					name[strlen(name)-1] = '\0';
					error = readfile(name, &mem2, &size2);
					if (!error)
					{
						preparefile(mem2, size2);
						error = parsemountfile(*dev, mem2, size2);
						FreeVec(mem2);
					}
					if (error != ERROR_OBJECT_NOT_FOUND)
						break;
				}
				if ((error == ERROR_OBJECT_NOT_FOUND) && (!error2))
					error = parsemountlist(*dev, mem, size);
				if (error)
					break;
				*dev++;
			}
			if (!error2)
				FreeVec(mem);
		}
	}
	FreeArgs(rda);
    } /* if (rda != NULL) */
    else
    {
	error = IoErr();
    }
  }
  if (error)
  {
    PrintFault(error,"Mount");
    return RETURN_FAIL;
  }
  return RETURN_OK;
}
