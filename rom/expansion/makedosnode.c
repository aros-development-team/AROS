/*
    Copyright (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: MakeDosNode() - Create a DOS DeviceNode structure.
    Lang: english
*/
#include "expansion_intern.h"
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <string.h>
/*****************************************************************************

    NAME */
#include <dos/filehandler.h>
#include <proto/expansion.h>

	AROS_LH1(struct DeviceNode *, MakeDosNode,

/*  SYNOPSIS */
	AROS_LHA(APTR, parmPacket, A0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 24, Expansion)

/*  FUNCTION
	MakeDosNode() will create a DeviceNode structure suitable for
	passing to dos.library which contains all the information about
	a device stored in the parmPacket array. This will allow you to
	enter a DOS device into the system from the information contained
	in a DosEnvec structure (such as in a RigidDiskBlock PartitionBlock
	structure).

	MakeDosNode() will allocate the memory that it needs to construct
	the DeviceNode, the strings and a FileSysStartupMsg that is passed
	to the filesystem handler on startup.

	You can use AddBootNode() to add a node to the system.

    INPUTS
	parmPacket  -   a longword array containing the device parameters
			required to initialize the structures. This is a
			variable length structure. See also the DosEnvec
			structure in dos/filehandler.h

	    longword    description
	    --------    -----------
	    0           Exec string with dos handler name (eg ffs.handler)
	    1           Exec string with exec device name (eg fdsk.device)
	    2           unit number of device to open
	    3           flags (for OpenDevice())
	    4           length of the remaining data
	    5-n         environment data - consists of:

	    5           Size of standard device block in 32 bit longwords
	    6           not used; 0
	    7           # of heads - drive specific
	    8           # of sectors per block - not used; 0
	    9           # of blocks per track - drive specific
	    10          # of reserved blocks at the start of the partition
	    11          # of reserved blocks at the end of the partition
	    12          device interleave
	    13          starting cylinder of partition
	    14          end cylinder of partition
	    15          initial number of buffers
	    16          type of memory for buffers (CHIP, FAST,...)
	    17          max number of bytes to transfer at one time
	    18          address mask allowable for DMA transfers
	    19          boot priority for autobootable devices
	    20          standard DOS filesystem ID (eg 'DOS\1')
	    21          baud rate for serial handler
	    22          control word for handler/filesystem
	    23          number of boot blocks on this partition

    RESULT
	deviceNode  -   An initialized DeviceNode structure, or NULL if
			the required memory could not be allocated. The
			caller will have to modify this structure before
			passing it to AddBootNode().

    NOTES
	There are a number of fields of the DeviceNode structure that this
	function cannot initialize due to a lack of information. You
	should fill these in yourself.

    EXAMPLE

    BUGS

    SEE ALSO
	AddBootNode(), AddDosNode(), dos/MakeDosEntry()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExpansionBase *,ExpansionBase)

    if(parmPacket)
    {
	struct DeviceNode *dn;
	struct FileSysStartupMsg *fssm;
	struct DevEnvec *de;
	STRPTR s1, s2 = 0;
	int strLen1, strLen2;

	/* This is the environment structure */
	de = (struct DevEnvec *)((ULONG *)parmPacket + 4);

	dn = AllocMem(sizeof(struct DeviceNode), MEMF_CLEAR|MEMF_PUBLIC);
	if( dn == NULL )
	    return NULL;

	fssm = AllocMem(sizeof(struct FileSysStartupMsg), MEMF_CLEAR|MEMF_PUBLIC);
	if( fssm == NULL )
	{
	    FreeMem(de, sizeof(struct DeviceNode));
	    return NULL;
	}

	/* To help prevent fragmentation I will allocate both strings in the
	   same block of memory.

	   Add for each string, 1 for null-termination
				1 for BSTR size
	   For the first string 3 for longword align
	*/
	strLen1 = strlen( (STRPTR)((ULONG *)parmPacket)[0] ) + 5;
	strLen2 = strLen1 + 2 + strlen( (STRPTR)((ULONG *)parmPacket)[1] );

	s1 = AllocVec( strLen2, MEMF_CLEAR|MEMF_PUBLIC );
	if( s1 == NULL )
	{
	    FreeMem( de, sizeof(struct DeviceNode));
	    FreeMem( fssm, sizeof(struct FileSysStartupMsg));
	    return NULL;
	}

	/* We have no more allocations */
	s2 = (STRPTR)(((ULONG)s1 + strLen1) & ~3);

	CopyMem( (STRPTR)((ULONG *)parmPacket)[0], &s1[1], strLen1 - 5);
	CopyMem( (STRPTR)((ULONG *)parmPacket)[1], &s2[1], strLen2 - 2);

	/* The NULL termination is there from MEMF_CLEAR */
	*s1 = (strLen1 - 5) > 255 ? 255 : (strLen1 - 5);
	*s2 = (strLen2 - 2) > 255 ? 255 : (strLen2 - 2);

	/* Strings are done, now the FileSysStartupMsg */
	fssm->fssm_Unit = ((ULONG *)parmPacket)[2];
	fssm->fssm_Device = MKBADDR(s1);
	fssm->fssm_Environ = MKBADDR(de);
	fssm->fssm_Flags = ((ULONG *)parmPacket)[3];

	/* FSSM is done, now the DeviceNode */
	/* Most of this we cannot set up, leave it to the user */
	dn->dn_Handler = MKBADDR(s2);
	dn->dn_Startup = MKBADDR(fssm);

	return dn;
    }
    return NULL;

    AROS_LIBFUNC_EXIT
} /* MakeDosNode */
