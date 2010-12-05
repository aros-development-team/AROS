/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/filesystem.h>

#include <aros/macros.h>
#include <aros/debug.h>

#include "afshandler.h"
#include "cache.h"
#include "error.h"
#include "filehandles1.h"
#include "filehandles2.h"
#include "filehandles3.h"
#include "misc.h"
#include "volumes.h"

#include "baseredef.h"

static VOID startFlushTimer(struct AFSBase *afsbase)
{
struct timerequest *request;

	/* Set up delay for next flush */
	if (afsbase->timer_flags & TIMER_ACTIVE) {
	    afsbase->timer_flags |= TIMER_RESTART;
	} else {
	D(bug("[afs] Starting timer\n"));
	    request = afsbase->timer_request;
	    request->tr_node.io_Command = TR_ADDREQUEST;
	    request->tr_time.tv_secs = 1;
	    request->tr_time.tv_micro = 0;
	    SendIO((struct IORequest *)afsbase->timer_request);
	    afsbase->timer_flags = TIMER_ACTIVE;
	}
}

/*******************************************
 Name  : AFS_work
 Descr.: main loop (get packets and answer (or not))
 Input : proc - our process structure
 Output: -
********************************************/
void AFS_work(struct AFSBase *afsbase) {
struct IOFileSys *iofs;
struct AfsHandle *afshandle;
LONG retval;
LONG error;

	afsbase->port.mp_SigBit = SIGBREAKB_CTRL_F;
	afsbase->port.mp_Flags = PA_SIGNAL;
	afsbase->timer_flags = 0;
	for (;;) {
		while ((iofs=(struct IOFileSys *)GetMsg(&afsbase->port))!=NULL)
		{
			/* Flush dirty blocks on all volumes */
			if (iofs->IOFS.io_Message.mn_Node.ln_Type == NT_REPLYMSG)
			{
			    afsbase->timer_flags &= ~TIMER_ACTIVE;
			    if (afsbase->timer_flags & TIMER_RESTART)
				startFlushTimer(afsbase);
			    else {
				struct Volume *volume, *tail;
				struct BlockCache *blockbuffer;

				D(bug("[afs] Alarm rang.\n"));
				volume = (struct Volume *)afsbase->device_list.lh_Head;
				tail = (struct Volume *)&afsbase->device_list.lh_Tail;
				while(volume != tail)
				{
					if ((volume->dostype == 0x444f5300) && mediumPresent(&volume->ioh))
					{
						/* Check if adding volume node needs to be retried */
						if (volume->volumenode == NULL)
							attemptAddDosVolume(afsbase, volume);

						flushCache(afsbase, volume);
						blockbuffer = getBlock(afsbase, volume, volume->rootblock);
						if ((blockbuffer->flags & BCF_WRITE) != 0)
						{
							writeBlock(afsbase, volume, blockbuffer, -1);
							blockbuffer->flags &= ~BCF_WRITE;
						}
						if (volume->ioh.flags & IOHF_MOTOR_OFF) {
							D(bug("[afs 0x%08lX] turning off motor\n", volume));
							motorOff(afsbase, &volume->ioh);
							volume->ioh.flags &= ~IOHF_MOTOR_OFF;
						}
					}
					volume = (struct Volume *)volume->ln.ln_Succ;
				}
			    }
			}
			else
			{
				DB2(bug("[afs] got command %lu\n",iofs->IOFS.io_Command));
				startFlushTimer(afsbase);
				error=0;
				afshandle = (struct AfsHandle *)iofs->IOFS.io_Unit;
				switch (iofs->IOFS.io_Command)
				{
				case (UWORD)-1 :
					{
						struct Volume *volume;
						volume = initVolume
							(
								afsbase,
								iofs->IOFS.io_Device,
								iofs->io_Union.io_OpenDevice.io_DeviceName,
								iofs->io_Union.io_OpenDevice.io_Unit,
								0,
								(struct DosEnvec *)iofs->io_Union.io_OpenDevice.io_Environ,
								&iofs->io_DosError
							);
						if (volume != NULL)
							iofs->IOFS.io_Unit = (struct Unit *)&volume->ah;
						else
							iofs->IOFS.io_Unit = NULL;
						PutMsg(&afsbase->rport, &iofs->IOFS.io_Message);
					}
					continue;
				case (UWORD)-2 :
					{
						struct Volume *volume;
						volume=((struct AfsHandle *)iofs->IOFS.io_Unit)->volume;
						if (volume->locklist != NULL)
						{
							error = ERROR_OBJECT_IN_USE;
						}
						else
						{
							uninitVolume(afsbase, volume);
							error=0;
						}
						iofs->io_DosError = error;
						PutMsg(&afsbase->rport, &iofs->IOFS.io_Message);
					}
					continue;
				case FSA_SAME_LOCK :
					iofs->io_Union.io_SAME_LOCK.io_Same=sameLock
						(
							iofs->io_Union.io_SAME_LOCK.io_Lock[0],
							iofs->io_Union.io_SAME_LOCK.io_Lock[1]
						);
					break;
				case FSA_IS_FILESYSTEM :
					iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem=TRUE;
					break;
				case FSA_INHIBIT :
					error=inhibit
						(
							afsbase,
							afshandle->volume,
							iofs->io_Union.io_INHIBIT.io_Inhibit
						);
					break;
				case FSA_CLOSE :
					closef(afsbase, afshandle);
					break;
				case FSA_MORE_CACHE :
				{
					struct Volume *volume;
					volume=((struct AfsHandle *)iofs->IOFS.io_Unit)->volume;
					if (iofs->io_Union.io_MORE_CACHE.io_NumBuffers) {
						volume->numbuffers += iofs->io_Union.io_MORE_CACHE.io_NumBuffers;
						if (volume->numbuffers < 1)
							volume->numbuffers = 1;
						flushCache(afsbase, volume);
						Forbid(); /* so that we at least get old number of buffers back */
						freeCache(afsbase, volume->blockcache);
						for (;;) {
							volume->blockcache = initCache(afsbase, volume, volume->numbuffers);
							if (volume->blockcache)
								break;
							volume->numbuffers /= 2;
							if (volume->numbuffers < 1)
								volume->numbuffers = 1;
						}
						Permit();
					}
					iofs->io_Union.io_MORE_CACHE.io_NumBuffers = volume->numbuffers;
					break;
				}
				default:
					if (mediumPresent(&afshandle->volume->ioh))
					{
						switch (iofs->IOFS.io_Command)
						{
						case FSA_OPEN : //locateObject, findupdate, findinput
							iofs->IOFS.io_Unit=(struct Unit *)openf
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_OPEN.io_Filename,
									iofs->io_Union.io_OPEN.io_FileMode,
									&error
								);
							break;
						case FSA_READ :
							iofs->io_Union.io_READ.io_Length=readf
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_READ.io_Buffer,
									iofs->io_Union.io_READ.io_Length,
									&error
								);
							break;
						case FSA_WRITE :
							iofs->io_Union.io_WRITE.io_Length=writef
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_WRITE.io_Buffer,
									iofs->io_Union.io_WRITE.io_Length,
									&error
								);
							break;
						case FSA_SEEK :
							iofs->io_Union.io_SEEK.io_Offset=seek
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_SEEK.io_Offset,
									iofs->io_Union.io_SEEK.io_SeekMode,
									&error
								);
							break;
						case FSA_SET_FILE_SIZE :
							iofs->io_Union.io_SEEK.io_Offset = setFileSize
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_SEEK.io_Offset,
									iofs->io_Union.io_SEEK.io_SeekMode,
									&error
								);
							break;
						case FSA_FILE_MODE :
							D(bug("[afs] set file mode nsy\n"));
							error=ERROR_ACTION_NOT_KNOWN;
							break;
						case FSA_EXAMINE :
							error=examine
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_EXAMINE.io_ead,
									iofs->io_Union.io_EXAMINE.io_Size,
									iofs->io_Union.io_EXAMINE.io_Mode,
									&iofs->io_DirPos
								);
							break;
						/* FIXME: Disabled FSA_EXAMINE_ALL support since it seems to have bugs */
						#if 0
						case FSA_EXAMINE_ALL :
							error=examineAll
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_EXAMINE_ALL.io_ead,
									iofs->io_Union.io_EXAMINE_ALL.io_eac,
									iofs->io_Union.io_EXAMINE_ALL.io_Size,
									iofs->io_Union.io_EXAMINE_ALL.io_Mode
								);
							break;
						#endif 
						case FSA_EXAMINE_NEXT :
							error=examineNext
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_EXAMINE_NEXT.io_fib
								);
							break;
						case FSA_OPEN_FILE :
							iofs->IOFS.io_Unit=(struct Unit *)openfile
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_OPEN_FILE.io_Filename,
									iofs->io_Union.io_OPEN_FILE.io_FileMode,
									iofs->io_Union.io_OPEN_FILE.io_Protection,
									&error
								);
							break;
						case FSA_CREATE_DIR :
							iofs->IOFS.io_Unit=(struct Unit *)createDir
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_CREATE_DIR.io_Filename,
									iofs->io_Union.io_CREATE_DIR.io_Protection,
									&error
								);
							break;
						case FSA_CREATE_HARDLINK :
							D(bug("[afs] create hardlinks nsy\n"));
							iofs->IOFS.io_Unit=0;
							error=ERROR_ACTION_NOT_KNOWN;
							break;
						case FSA_CREATE_SOFTLINK :
							D(bug("[afs] create softlinks nsy\n"));
							iofs->IOFS.io_Unit=0;
							error=ERROR_ACTION_NOT_KNOWN;
							break;
						case FSA_READ_SOFTLINK :
							D(bug("[afs] read softlinks nsy\n"));
							error=ERROR_ACTION_NOT_KNOWN;
							break;
						case FSA_RENAME :
							error=renameObject
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_RENAME.io_Filename,
									iofs->io_Union.io_RENAME.io_NewName
								);
							break;
						case FSA_DELETE_OBJECT :
							error=deleteObject
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_DELETE_OBJECT.io_Filename
								);
							break;
						case FSA_SET_COMMENT :
							error=setComment
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_SET_COMMENT.io_Filename,
									iofs->io_Union.io_SET_COMMENT.io_Comment
								);
							break;
						case FSA_SET_PROTECT :
							error=setProtect
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_SET_PROTECT.io_Filename,
									iofs->io_Union.io_SET_PROTECT.io_Protection
								);
							break;
						case FSA_SET_OWNER :
							D(bug("[afs] set owner nsy\n"));
							error=ERROR_ACTION_NOT_KNOWN;
							break;
						case FSA_SET_DATE :
							error=setDate
								(
									afsbase,
									afshandle,
									iofs->io_Union.io_SET_DATE.io_Filename,
									&iofs->io_Union.io_SET_DATE.io_Date
								);
							break;
/* morecache */
						case FSA_FORMAT :
							error=format
								(
									afsbase,
									afshandle->volume,
									iofs->io_Union.io_FORMAT.io_VolumeName,
									iofs->io_Union.io_FORMAT.io_DosType
								);
							break;
						case FSA_RELABEL :
							iofs->io_Union.io_RELABEL.io_Result=relabel
								(
									afsbase,
									afshandle->volume,
									iofs->io_Union.io_RELABEL.io_NewName,
									&error
								);
							break;
						case FSA_DISK_INFO :
							error=getDiskInfo
								(afshandle->volume, iofs->io_Union.io_INFO.io_Info);
							break;
						default :
							D(bug("[afs] unknown fsa %d\n", iofs->IOFS.io_Command));
							retval=DOSFALSE;
							error=ERROR_ACTION_NOT_KNOWN;
						}
					}
					else
					{
						switch (iofs->IOFS.io_Command)
						{
						case FSA_OPEN : /* locateObject, findupdate, findinput */
						case FSA_READ :
						case FSA_WRITE :
						case FSA_SEEK :
						case FSA_SET_FILE_SIZE :
						case FSA_FILE_MODE :
						case FSA_EXAMINE :
						/* FIXME: Disabled FSA_EXAMINE_ALL support */
                                        	#if 0
	                                        case FSA_EXAMINE_ALL :
						#endif
                	                        case FSA_EXAMINE_NEXT :
						case FSA_OPEN_FILE :
						case FSA_CREATE_DIR :
						case FSA_CREATE_HARDLINK :
						case FSA_CREATE_SOFTLINK :
						case FSA_READ_SOFTLINK :
						case FSA_RENAME :
						case FSA_DELETE_OBJECT :
						case FSA_SET_COMMENT :
						case FSA_SET_PROTECT :
						case FSA_SET_OWNER :
						case FSA_SET_DATE :
						case FSA_FORMAT :
						case FSA_RELABEL :
						case FSA_DISK_INFO :
							retval= DOSFALSE;
							error = ERROR_NO_DISK;
							break;
						default :
							D(bug("[afs] unknown fsa %d\n", iofs->IOFS.io_Command));
							retval= DOSFALSE;
							error = ERROR_ACTION_NOT_KNOWN;
						}
					}
				}
				D(checkCache(afsbase, afshandle->volume));
				iofs->io_DosError = error;
				ReplyMsg(&iofs->IOFS.io_Message);
			}
		}
		checkDeviceFlags(afsbase);
		Wait(1<<afsbase->port.mp_SigBit);
	}
}

