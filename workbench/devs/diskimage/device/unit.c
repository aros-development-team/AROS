/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include "diskimage_device.h"
#include <libraries/iffparse.h>
#include <proto/expat.h>
#include <SDI_stdarg.h>

struct ChangeInt {
	struct MinNode ci_Node;
	struct Interrupt *ci_Interrupt;
};

static void ReadUnitPrefs (struct DiskImageUnit *unit);
static void WriteUnitPrefs (struct DiskImageUnit *unit, BOOL envarc);
static inline struct IOExtTD *GetIOMsg (struct DiskImageUnit *unit);
static void Cleanup (struct DiskImageUnit *unit);
static LONG TDGeometry (struct DiskImageUnit *unit, struct IOStdReq *io);
static LONG TDRead (struct DiskImageUnit *unit, struct IOStdReq *io);
static LONG TDWrite (struct DiskImageUnit *unit, struct IOStdReq *io);
static void InsertDisk (struct DiskImageUnit *unit, BPTR dir, CONST_STRPTR filename,
	struct DiskImagePlugin *plugin, STRPTR fullpath, ULONG fullpath_size);
static void RemoveDisk (struct DiskImageUnit *unit);
static void DiskChange (struct DiskImageUnit *unit);

#ifdef __AROS__
AROS_UFH3(LONG, UnitProcEntry,
	AROS_UFHA(STRPTR, argstr, A0),
	AROS_UFHA(ULONG, arglen, D0),
	AROS_UFHA(struct Library *, SysBase, A6)
)
{
	AROS_USERFUNC_INIT
#else
int UnitProcEntry (void) {
#endif
	struct Process *proc;
	struct DiskImageMsg *msg;
	struct DiskImageUnit *unit;
	struct DeathMessage *dm;

	dbug(("UnitProcEntry()\n"));

	proc = (struct Process *)FindTask(NULL);
	WaitPort(&proc->pr_MsgPort);
	msg = (struct DiskImageMsg *)GetMsg(&proc->pr_MsgPort);
	if (!msg->dim_Unit || msg->dim_Command != DICMD_STARTUP) {
		return RETURN_FAIL;
	}

	unit = msg->dim_Unit;
	dm = unit->DeathMsg;

	dm->dm_ReturnCode = UnitProcMain(unit);
	dm->dm_Result2 = IoErr();

	Forbid();
	ReplyMsg(&dm->dm_Msg);
	return dm->dm_ReturnCode;
#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

int UnitProcMain (struct DiskImageUnit *unit) {
	struct DiskImageBase *libBase = unit->LibBase;
	struct Library *SysBase = libBase->SysBase;
	struct Library *UtilityBase = libBase->UtilityBase;
	struct IOExtTD *iotd;
	struct DiskImageMsg *msg;
	const struct TagItem *ti, *tstate;
	LONG err;
	ULONG sigmask;
	CONST_STRPTR filename, plugin_name;
	struct ChangeInt *handler;
	BOOL disk_change = FALSE;

	dbug(("UnitProcMain()\n"));

	unit->IOPort = CreateMsgPort();
	unit->MsgPort = CreateMsgPort();
	if (!unit->IOPort || !unit->MsgPort) {
		Cleanup(unit);
		return RETURN_FAIL;
	}
	
	unit->Prefs = AllocPrefsDictionary();
	if (!unit->Prefs) {
		Cleanup(unit);
		return RETURN_FAIL;
	}

	dbug(("replying to start msg\n"));
	ReplyMsg(&unit->DiskImageMsg->dim_Msg);

#ifdef __AROS__
	ObtainSemaphore(libBase->PluginSemaphore);
	if (IsListEmpty(libBase->Plugins)) {
		InitLocaleInfo(SysBase, &libBase->LocaleInfo, "diskimagedevice.catalog");
		LoadPlugins(libBase);
	}
	ReleaseSemaphore(libBase->PluginSemaphore);
#endif

	ReadUnitPrefs(unit);
	unit->DeviceType = DictGetIntegerForKey(unit->Prefs, "DeviceType", DG_DIRECT_ACCESS);
	unit->Flags = DictGetIntegerForKey(unit->Prefs, "Flags", DGF_REMOVABLE);
	filename = DictGetStringForKey(unit->Prefs, "DiskImageFile", NULL);
	plugin_name = DictGetStringForKey(unit->Prefs, "Plugin", NULL);
	if (filename) {
		struct DiskImagePlugin *plugin = NULL;
		ObtainSemaphoreShared(libBase->PluginSemaphore);
		if (plugin_name) {
			plugin = (struct DiskImagePlugin *)FindName(libBase->Plugins, plugin_name);
		}
		if (!plugin_name || plugin) {
			APTR window;
			window = SetProcWindow((APTR)-1);
			InsertDisk(unit, ZERO, filename, plugin, NULL, 0);
			SetProcWindow(window);
		}
		ReleaseSemaphore(libBase->PluginSemaphore);
	}

	sigmask = (1UL << unit->IOPort->mp_SigBit)|(1UL << unit->MsgPort->mp_SigBit);
	dbug(("entering main loop\n"));
	for (;;) {
		Wait(sigmask);

		while ((msg = (struct DiskImageMsg *)GetMsg(unit->MsgPort))) {
			switch (msg->dim_Command) {
				case DICMD_DIE:
					Cleanup(unit);
					return RETURN_OK;

				case DICMD_TAGLIST:
					if ((tstate = msg->dim_Tags)) {
						BPTR curr_dir = ZERO;
						struct DiskImagePlugin *plugin = NULL;

						unit->Error = NO_ERROR;
						unit->ErrorString = NULL;

						ObtainSemaphoreShared(libBase->PluginSemaphore);
						while (!unit->Error && (ti = NextTagItem(&tstate))) {
							switch (ti->ti_Tag) {
								case DITAG_Error:
									unit->ErrorPtr = (LONG *)ti->ti_Data;
									if (unit->ErrorPtr) {
										*unit->ErrorPtr = NO_ERROR;
									}
									break;

								case DITAG_ErrorString:
									unit->ErrorString = (STRPTR)ti->ti_Data;
									if (unit->ErrorString && unit->ErrorStringLength) {
										unit->ErrorString[0] = 0;
									}
									break;

								case DITAG_ErrorStringLength:
									unit->ErrorStringLength = ti->ti_Data;
									if (unit->ErrorString && unit->ErrorStringLength) {
										unit->ErrorString[0] = 0;
									}
									break;

								case DITAG_Screen:
									unit->Screen = (struct Screen *)ti->ti_Data;
									break;

								case DITAG_Password:
									unit->Password = (CONST_STRPTR)ti->ti_Data;
									break;

								case DITAG_CurrentDir:
									curr_dir = (BPTR)ti->ti_Data;
									break;

								case DITAG_Plugin:
									if (!ti->ti_Data) break;
									plugin = (void *)FindName(libBase->Plugins, (STRPTR)ti->ti_Data);
									if (!plugin) {
										SetDiskImageError(NULL, unit, ERROR_OBJECT_NOT_FOUND, 0);
										break;
									}
									break;

								case DITAG_Filename: {
									APTR image_data = unit->ImageData;
									TEXT fullpath[512];
									RemoveDisk(unit);
									filename = (CONST_STRPTR)ti->ti_Data;
									if (filename) {
										InsertDisk(unit, curr_dir, filename, plugin, fullpath, sizeof(fullpath));
									}
									if (image_data || unit->ImageData) {
										disk_change = TRUE;
										if (unit->ImageData) {
											DictSetObjectForKey(unit->Prefs,
												AllocPrefsString(fullpath),
												"DiskImageFile");
										} else {
											DictRemoveObjForKey(unit->Prefs,
												"DiskImageFile");
										}
										if (unit->ImageData && plugin) {
											DictSetObjectForKey(unit->Prefs,
												AllocPrefsString(plugin->Node.ln_Name),
												"Plugin");
										} else {
											DictRemoveObjForKey(unit->Prefs,
												"Plugin");
										}
										WriteUnitPrefs(unit, TRUE);
									}
									break;
								}

								case DITAG_WriteProtect:
									unit->WriteProtect = ti->ti_Data ? TRUE : FALSE;
									break;

								case DITAG_GetImageName:
									if (!unit->Name) {
										*(STRPTR *)ti->ti_Data = NULL;
										break;
									}
									*(STRPTR *)ti->ti_Data = ASPrintf("%s", unit->Name);
									if (!ti->ti_Data) {
										SetDiskImageError(NULL, unit, ERROR_NO_FREE_STORE, 0);
										break;
									}
									break;

								case DITAG_GetWriteProtect:
									*(BOOL *)ti->ti_Data = unit->WriteProtect;
									break;

								case DITAG_DiskImageType:
									if (unit->ImageData)
										*(ULONG *)ti->ti_Data = DITYPE_RAW;
									else
										*(ULONG *)ti->ti_Data = DITYPE_NONE;
									break;

								case DITAG_SetDeviceType:
									if (unit->DeviceType != ti->ti_Data) {
										unit->DeviceType = ti->ti_Data;
										DictSetObjectForKey(unit->Prefs,
											AllocPrefsInteger(unit->DeviceType),
											"DeviceType");
										WriteUnitPrefs(unit, TRUE);
									}
									break;

								case DITAG_GetDeviceType:
									*(UBYTE *)ti->ti_Data = unit->DeviceType;
									break;

								case DITAG_SetFlags:
									if (unit->Flags != ti->ti_Data) {
										unit->Flags = ti->ti_Data;
										DictSetObjectForKey(unit->Prefs,
											AllocPrefsInteger(unit->Flags),
											"Flags");
										WriteUnitPrefs(unit, TRUE);
									}
									break;

								case DITAG_GetFlags:
									*(UBYTE *)ti->ti_Data = unit->Flags;
									break;
							} /* switch */
						} /* while */
						ReleaseSemaphore(libBase->PluginSemaphore);

						unit->Screen = NULL;
						unit->Password = NULL;
					} /* if */
					break;
			} /* switch */
			ReplyMsg(&msg->dim_Msg);
		}
		if (disk_change) {
			DiskChange(unit);
			disk_change = FALSE;
		}

		while ((iotd = GetIOMsg(unit))) {
			switch (iotd->iotd_Req.io_Command) {
				case ETD_READ:
					if (iotd->iotd_Count < unit->ChangeCnt) {
						err = TDERR_DiskChanged;
						break;
					}
				case CMD_READ:
					iotd->iotd_Req.io_Actual = 0;
					err = TDRead(unit, &iotd->iotd_Req);
					break;

				case ETD_WRITE:
				case ETD_FORMAT:
					if (iotd->iotd_Count < unit->ChangeCnt) {
						err = TDERR_DiskChanged;
						break;
					}
				case CMD_WRITE:
				case TD_FORMAT:
					iotd->iotd_Req.io_Actual = 0;
					err = TDWrite(unit, &iotd->iotd_Req);
					break;

				case NSCMD_ETD_READ64:
					if (iotd->iotd_Count < unit->ChangeCnt) {
						err = TDERR_DiskChanged;
						break;
					}
				case NSCMD_TD_READ64:
				case TD_READ64:
					err = TDRead(unit, &iotd->iotd_Req);
					break;

				case NSCMD_ETD_WRITE64:
				case NSCMD_ETD_FORMAT64:
					if (iotd->iotd_Count < unit->ChangeCnt) {
						err = TDERR_DiskChanged;
						break;
					}
				case NSCMD_TD_WRITE64:
				case NSCMD_TD_FORMAT64:
				case TD_WRITE64:
				case TD_FORMAT64:
					err = TDWrite(unit, &iotd->iotd_Req);
					break;

				case TD_CHANGENUM:
					err = IOERR_SUCCESS;
					iotd->iotd_Req.io_Actual = unit->ChangeCnt;
					break;

				case TD_CHANGESTATE:
					err = IOERR_SUCCESS;
					iotd->iotd_Req.io_Actual = unit->ImageData ? 0 : 1;
					break;

				case TD_PROTSTATUS:
					err = IOERR_SUCCESS;
					iotd->iotd_Req.io_Actual = unit->WriteProtect;
					break;

				case TD_GETGEOMETRY:
					err = TDGeometry(unit, &iotd->iotd_Req);
					break;

				case TD_EJECT:
					err = IOERR_SUCCESS;
					if (unit->ImageData) {
						RemoveDisk(unit);
						DiskChange(unit);
					}
					break;

				case TD_REMOVE:
					err = IOERR_SUCCESS;
					unit->ObsoleteChangeInt = (struct Interrupt *)iotd->iotd_Req.io_Data;
					break;

				case TD_ADDCHANGEINT:
					handler = AllocMem(sizeof(*handler), MEMF_CLEAR);
					if (handler) {
						iotd->iotd_Req.io_Error = 0;
						handler->ci_Interrupt = (struct Interrupt *)iotd->iotd_Req.io_Data;
						AddTail(unit->ChangeInts, (struct Node *)&handler->ci_Node);
					} else {
						iotd->iotd_Req.io_Error = TDERR_NoMem;
					}
					iotd = NULL;
					break;

				case TD_REMCHANGEINT:
					err = IOERR_SUCCESS;
					handler = (struct ChangeInt *)unit->ChangeInts->lh_Head;
					while (handler->ci_Node.mln_Succ) {
						if (handler->ci_Interrupt == (struct Interrupt *)iotd->iotd_Req.io_Data) {
							Remove((struct Node *)&handler->ci_Node);
							FreeMem(handler, sizeof(*handler));
							break;
						}
						handler = (struct ChangeInt *)handler->ci_Node.mln_Succ;
					}
					break;

				case HD_SCSICMD:
					err = DoSCSICmd(&iotd->iotd_Req, (struct SCSICmd *)iotd->iotd_Req.io_Data);
					break;

				default:
					err = IOERR_NOCMD;
					break;
			}

			if (iotd) {
				iotd->iotd_Req.io_Error = err;
				ReplyMsg(&iotd->iotd_Req.io_Message);
			}
		}
	}
}

static void ReadUnitPrefs (struct DiskImageUnit *unit) {
	TEXT filename[64];
	SNPrintf(filename, sizeof(filename), "ENV:DiskImage/unit_%ld.xml", unit->UnitNum);
	if (!ReadPrefs(unit->Prefs, filename)) {
		SNPrintf(filename, sizeof(filename), "ENVARC:DiskImage/unit_%ld.xml", unit->UnitNum);
		ReadPrefs(unit->Prefs, filename);
	}
}

static void WriteUnitPrefs (struct DiskImageUnit *unit, BOOL envarc) {
	TEXT filename[64];
	if (envarc) {
		UnLock(CreateDir("ENVARC:DiskImage"));
		SNPrintf(filename, sizeof(filename), "ENVARC:DiskImage/unit_%ld.xml", unit->UnitNum);
		WritePrefs(unit->Prefs, filename);
	}
	UnLock(CreateDir("ENV:DiskImage"));
	SNPrintf(filename, sizeof(filename), "ENV:DiskImage/unit_%ld.xml", unit->UnitNum);
	WritePrefs(unit->Prefs, filename);
}

static inline struct IOExtTD *GetIOMsg (struct DiskImageUnit *unit) {
	struct Library *SysBase = unit->LibBase->SysBase;
	struct IOExtTD *iotd;
	ObtainSemaphore(unit->IOSemaphore);
	iotd = (struct IOExtTD *)GetMsg(unit->IOPort);
	ReleaseSemaphore(unit->IOSemaphore);
	return iotd;
}

static void Cleanup (struct DiskImageUnit *unit) {
	struct Library *SysBase = unit->LibBase->SysBase;
	RemoveDisk(unit);
	FreePrefsObject(unit->Prefs);
	DeleteMsgPort(unit->IOPort);
	DeleteMsgPort(unit->MsgPort);
	unit->IOPort = unit->MsgPort = NULL;
}

LONG DOS2IOErr (APTR Self, LONG error) {
	LONG ret;
	switch (error) {
		case ERROR_SEEK_ERROR:
			ret = TDERR_SeekError;
			break;
		case ERROR_DISK_WRITE_PROTECTED:
		case ERROR_WRITE_PROTECTED:
			ret = TDERR_WriteProt;
			break;
		case ERROR_NO_DISK:
			ret = TDERR_DiskChanged;
			break;
		case ERROR_NO_FREE_STORE:
			ret = TDERR_NoMem;
			break;
		default:
			ret = TDERR_NotSpecified;
			break;
	}
	return ret;
}

static LONG TDGeometry (struct DiskImageUnit *unit, struct IOStdReq *io) {
	struct DiskImagePlugin *plugin = unit->Plugin;
	struct DriveGeometry *dg = (struct DriveGeometry *)io->io_Data;
	LONG error;

	io->io_Actual = 0;
	if (io->io_Length < sizeof(struct DriveGeometry)) {
		return IOERR_BADLENGTH;
	}

	memset(dg, 0, io->io_Length);
	dg->dg_SectorSize = 512;
	dg->dg_CylSectors = 1;
	dg->dg_Heads = 1;
	dg->dg_TrackSectors = 1;
   	dg->dg_BufMemType = MEMF_ANY;
   	dg->dg_DeviceType = unit->DeviceType;
   	dg->dg_Flags = unit->Flags;

	if (!unit->ImageData || !plugin) {
		io->io_Actual = sizeof(struct DriveGeometry);
		return IOERR_SUCCESS;
	}

	error = Plugin_Geometry(plugin, unit->ImageData, dg);
	if (error == IOERR_SUCCESS) {
		io->io_Actual = sizeof(struct DriveGeometry);
	}
	return error;
}

static LONG TDRead (struct DiskImageUnit *unit, struct IOStdReq *io) {
	struct DiskImagePlugin *plugin = unit->Plugin;
	if (!unit->ImageData || !plugin) {
		return TDERR_DiskChanged;
	}
	return Plugin_Read(plugin, unit->ImageData, io);
}

static LONG TDWrite (struct DiskImageUnit *unit, struct IOStdReq *io) {
	struct DiskImagePlugin *plugin = unit->Plugin;
	if (!unit->ImageData || !plugin) {
		return TDERR_DiskChanged;
	}
	if (unit->WriteProtect || !plugin->plugin_Write) {
		return TDERR_WriteProt;
	}
	return Plugin_Write(plugin, unit->ImageData, io);
}

static void InsertDisk (struct DiskImageUnit *unit, BPTR dir, CONST_STRPTR filename,
	struct DiskImagePlugin *plugin, STRPTR fullpath, ULONG fullpath_size)
{
	struct Library *DOSBase = unit->LibBase->DOSBase;
	BPTR curr_dir, file;
	curr_dir = CurrentDir(dir);
	unit->Name = ASPrintf("%s", FilePart(filename));
	file = Open(filename, MODE_OLDFILE);
	if (unit->Name && file) {
		if (fullpath && fullpath_size) {
			NameFromFH(file, fullpath, fullpath_size);
		}
		if (plugin) {
			unit->Plugin = plugin;
			unit->ImageData = Plugin_OpenImage(plugin, unit, file, filename);
		} else {
			unit->ImageData = OpenImage(NULL, unit, file, filename);
		}
	} else {
		const LONG error = !file ? IoErr() : ERROR_NO_FREE_STORE;
		SetDiskImageError(NULL, unit, error, 0);
	}
	if (!unit->Plugin) {
		Close(file);
	}
	if (!unit->ImageData) {
		RemoveDisk(unit);
	}
	CurrentDir(curr_dir);
}

static void RemoveDisk (struct DiskImageUnit *unit) {
	struct Library *SysBase = unit->LibBase->SysBase;

	if (unit->Plugin && unit->ImageData) {
		Plugin_CloseImage(unit->Plugin, unit->ImageData);
	}
	unit->ImageData =
	unit->Plugin = NULL;

	RemoveTempFile(NULL, unit);

	FreeVec(unit->Name);
	unit->Name = NULL;
}

static void DiskChange (struct DiskImageUnit *unit) {
	struct DiskImageBase *libBase = unit->LibBase;
	struct Library *SysBase = libBase->SysBase;
	struct Library *UtilityBase = libBase->UtilityBase;
	struct ChangeInt *handler;
	struct Hook *hook;

	unit->ChangeCnt++;

	if (unit->ObsoleteChangeInt) {
		Cause(unit->ObsoleteChangeInt);
	}

	handler = (struct ChangeInt *)unit->ChangeInts->lh_Head;
	while (handler->ci_Node.mln_Succ) {
		Cause(handler->ci_Interrupt);
		handler = (struct ChangeInt *)handler->ci_Node.mln_Succ;
	}

	ObtainSemaphoreShared(libBase->DiskChangeSemaphore);
	hook = (struct Hook *)libBase->DiskChangeHooks->lh_Head;
	while (hook->h_MinNode.mln_Succ) {
		CallHookPkt(hook, &unit->UnitNum, hook->h_Data);
		hook = (struct Hook *)hook->h_MinNode.mln_Succ;
	}
	ReleaseSemaphore(libBase->DiskChangeSemaphore);
}

void SetDiskImageErrorA (APTR Self, struct DiskImageUnit *unit, LONG error,
	LONG error_string, CONST_APTR error_args)
{
	struct DiskImageBase *libBase = unit->LibBase;
	if (error != NO_ERROR) {
		unit->Error = error;
		if (unit->ErrorPtr) {
			*unit->ErrorPtr = error;
		}
	}
	if (unit->ErrorString && unit->ErrorStringLength) {
		if (error_string != NO_ERROR_STRING) {
			VSNPrintf(unit->ErrorString, unit->ErrorStringLength,
				GetString(&libBase->LocaleInfo, error_string), error_args);
		} else if (error != NO_ERROR) {
			struct Library *DOSBase = libBase->DOSBase;
			Fault(error, NULL, unit->ErrorString, unit->ErrorStringLength);
		}
	}
}

VARARGS68K void SetDiskImageError (APTR Self, struct DiskImageUnit *unit, LONG error,
	LONG error_string, ...)
{
	VA_LIST args;
	VA_START(args, error_string);
	SetDiskImageErrorA(Self, unit, error, error_string, VA_ARG(args, CONST_APTR));
	VA_END(args);
}
