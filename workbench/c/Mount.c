/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Mount CLI command
    Lang: English
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
#include <string.h>

# define   DEBUG 1
# include  <aros/debug.h>

static const char version[] = "$VER: Mount 41.2 (27.10.2001)\n";

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
		*mem = (STRPTR)AllocVec(*size, MEMF_ANY);

		if (*mem != NULL)
		{
		    rest = *size;
		    buf = *mem;

		    for (;;)
		    {
			if (!rest)
			{
			    Close(ml);

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
"HANDLER=FILESYSTEM/A/K,DEVICE/K,UNIT/K/N,BLOCKSIZE/K/N,SURFACES/K/N,"
"BLOCKSPERTRACK/K/N,RESERVED/K/N,INTERLEAVE/K/N,LOWCYL/K/N,HIGHCYL/K/N,"
"BUFFERS/K/N,BUFMEMTYPE/K/N,MAXTRANSFER/K/N,MASK/K/N,BOOTPRI/K/N,"
"DOSTYPE/K/N,BAUD/K/N,CONTROL/K";

static LONG mount(STRPTR name, STRPTR buf, LONG size)
{
    IPTR *args[18]=
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    IPTR            *params;	     /* MakeDosNode() paramPacket */
    struct DosEnvec *vec;

    UBYTE  buffer[1024];
    LONG   error, res;
    STRPTR end = buf + size;
    STRPTR s2;
    struct RDArgs *rd;
    struct RDArgs rda = { { NULL, 0, 0 }, 0, NULL, 0, NULL, RDAF_NOPROMPT };

    params = AllocVec(sizeof(struct DosEnvec) + sizeof(IPTR)*4,
		      MEMF_PUBLIC | MEMF_CLEAR);

    if (params == NULL)
    {
	return ERROR_NO_FREE_STORE;
    }

    vec = (struct DosEnvec *)&params[4];

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
	    rd = ReadArgs((STRPTR)options, (IPTR *)args, &rda);

	    if (rd == NULL)
	    {
		return IoErr();
	    }

	    vec->de_TableSize      = (IPTR)19;
	    vec->de_SizeBlock      = (IPTR)(args[3]  ? *args[3]  : 512)/4;
	    vec->de_SegOrg         = args[3]  ? (IPTR)*args[3]  : (IPTR)512;
	    vec->de_Surfaces       = args[4]  ? (IPTR)*args[4]  : (IPTR)2;
	    vec->de_SectorPerBlock = 1;
	    vec->de_BlocksPerTrack = args[5]  ? (IPTR)*args[5]  : (IPTR)11;
	    vec->de_Reserved       = args[6]  ? (IPTR)*args[6]  : (IPTR)2;
	    vec->de_Interleave     = args[7]  ? (IPTR)*args[7]  : (IPTR)0;
	    vec->de_LowCyl         = args[8]  ? (IPTR)*args[8]  : (IPTR)0;
	    vec->de_HighCyl        = args[9]  ? (IPTR)*args[9]  : (IPTR)79;
	    vec->de_NumBuffers     = args[10] ? (IPTR)*args[10] : (IPTR)20;
	    vec->de_BufMemType     = args[11] ? (IPTR)*args[11] : (IPTR)1;
	    vec->de_MaxTransfer    = args[12] ? (IPTR)*args[12] : (IPTR)~0ul;
	    vec->de_Mask           = args[13] ? (IPTR)*args[13] : (IPTR)~0ul;
	    vec->de_BootPri        = args[14] ? (IPTR)*args[14] : (IPTR)0;
	    vec->de_DosType        = args[15] ? (IPTR)*args[15] : (IPTR)0x444f5301;
	    vec->de_Baud           = args[16] ? (IPTR)*args[16] : (IPTR)9600;
	    vec->de_Control        = args[17] ? (IPTR)*args[17] : (IPTR)"";
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
		    dn->dn_OldName = DuplicateBSTRVolumeName(buffer);

		    if (dn->dn_OldName != NULL)
		    {
			dn->dn_NewName = AROS_BSTR_ADDR(dn->dn_OldName);

			if (AddDosNode(vec->de_BootPri, ADNF_STARTPROC, dn))
			    error = 0;
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

	    FreeArgs(rd);

	    return error;
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


int __nocommandline;

int UtilityBase_version = 0;
int ExpansionBase_version = 0;

int main(void)
{
    STRPTR  args[2] = { NULL, "Devs:Mountlist" };
    STRPTR  mem;
    LONG    size;
    LONG    error = 0;

    struct RDArgs    *rda;

    rda = ReadArgs("DEVICE/M,FROM/K", (ULONG *)args, NULL);

    if (rda != NULL)
    {
	error = readfile(args[1], &mem, &size);

	if (!error)
	{
	    STRPTR *dev = (STRPTR *)args[0];
	    preparefile(mem, size);

	    if (dev) while (*dev)
	    {
		error = mount(*dev++, mem, size);

		if (error)
		{
		    break;
		}
	    }

	    FreeVec(mem);
	}

	FreeArgs(rda);
    } /* if (rda != NULL) */
    else
    {
	error = IoErr();
    }

    if (error)
    {
	PrintFault(error,"Mount");

	return RETURN_FAIL;
    }

    return RETURN_OK;
}
