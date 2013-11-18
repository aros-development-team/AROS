/* $Id$ */
/* $Log: mountf.c $
 * Revision 1.1  1997/03/03  22:04:04  Michiel
 * Initial revision
 *
 * Revision 10.6  1995/02/15  16:43:39  Michiel
 * Release version
 * Using new headers (struct.h & blocks.h)
 *
 * Revision 10.5  1995/01/29  07:34:57  Michiel
 * Other disk specs
 *
 * Revision 10.4  1995/01/18  04:29:34  Michiel
 * Less_buffers
 *
 * Revision 10.3  1995/01/13  15:22:16  Michiel
 * mounts on dh1 now
 * bug fixed, can format while debugging now
 *
 * Revision 10.2  1994/11/15  17:52:30  Michiel
 * Mounts on harddisk now
 *
 * Revision 10.1  1994/10/24  11:16:28  Michiel
 * first RCS revision
 * */

/* Copyright (c) 1994 Doug Walker, Raleigh, NC */
/* All Rights Reserved. */
#define __USE_SYSBASE

#include <dos/dos.h>
#include <dos/filehandler.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

//#include "mount.h"
#include "blocks.h"
#include "struct.h"

/* protos */
int DisMount(struct DeviceList *volume);
struct DeviceNode *Mount(char *name, struct MsgPort *port);

#define V37 (SysBase->LibNode.lib_Version > 36)

static __aligned UBYTE devicename[] = {16,'t','r','a','c','k','d','i','s','k','.','d','e','v','i','c','e',0};

/* Mount a volume with the given name; route all handler
** messages to the given port.
*/
struct DeviceNode *Mount(char *name, struct MsgPort *port)
{
   struct DeviceNode *volume;
   struct DosList *dlist;
   struct FileSysStartupMsg *fssm = AllocVec(sizeof(struct FileSysStartupMsg), MEMF_CLEAR);
   struct DosEnvec  *dosenvec = AllocVec(sizeof(struct DosEnvec), MEMF_CLEAR);

   if(name == NULL || port == NULL) return NULL;

	/* make dosenvec */

	dosenvec->de_TableSize = 17;
	dosenvec->de_SizeBlock = 128;
	dosenvec->de_Surfaces = 2;
	dosenvec->de_SectorPerBlock = 1;
	dosenvec->de_BlocksPerTrack = 11;
	dosenvec->de_Reserved = 2;
	dosenvec->de_LowCyl = 0;
	dosenvec->de_HighCyl = 79;
	dosenvec->de_NumBuffers = 100;
	dosenvec->de_BufMemType = 0;
	dosenvec->de_MaxTransfer = 0xffffff;
	dosenvec->de_Mask = 0x7ffffffe;
	dosenvec->de_DosType = ID_PFS_DISK;
	dosenvec->de_BootBlocks = 2;

	/* make fssm */

	fssm->fssm_Unit = 0;
	fssm->fssm_Device = MKBADDR(devicename);
	fssm->fssm_Environ = MKBADDR(dosenvec);
	fssm->fssm_Flags = 0;

	while(!(dlist = AttemptLockDosList(LDF_DEVICES|LDF_WRITE)))
	{
		/* Can't lock the DOS list.  Wait a second and try again. */
		Delay(50);
	}
	volume = (struct DeviceNode *)FindDosEntry(dlist, name, LDF_DEVICES);
	if(volume) RemDosEntry((struct DosList *)volume);
	UnLockDosList(LDF_DEVICES|LDF_WRITE);

	if(!volume && !(volume = (struct DeviceNode *)MakeDosEntry(name, DLT_DEVICE)))
		return NULL;
   
	volume->dn_Startup = MKBADDR(fssm);
	volume->dn_Lock = NULL;
	volume->dn_GlobalVec = -1;

	/* Now we can own the volume by giving it our msgport */
	volume->dn_Task = port;

	while(!(dlist = AttemptLockDosList(LDF_DEVICES|LDF_WRITE)))
	{
      /* Oops, can't lock DOS list.  Wait 1 second and retry. */
      Delay(50);
	}
	AddDosEntry((struct DosList *)volume);
	UnLockDosList(LDF_DEVICES|LDF_WRITE);

	return volume;
}
