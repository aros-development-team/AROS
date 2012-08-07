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
#include "scsicmd.h"
#include "endian.h"
#include "bytepack.h"

static BOOL GetGeometry (struct DiskImageUnit *unit, struct DriveGeometry *dg) {
	struct DiskImagePlugin *plugin = unit->Plugin;

	ClearMem(dg, sizeof(*dg));
	dg->dg_SectorSize = 512;
	dg->dg_CylSectors = 1;
	dg->dg_Heads = 1;
	dg->dg_TrackSectors = 1;
   	dg->dg_BufMemType = MEMF_ANY;
   	dg->dg_DeviceType = unit->DeviceType;
   	dg->dg_Flags = unit->Flags;

	if (unit->ImageData && plugin) {
		return Plugin_Geometry(plugin, unit->ImageData, dg) == IOERR_SUCCESS;
	} else {
		return FALSE;
	}
}

static void WriteSenseData (BytePackBuffer *sense, UBYTE sensekey, UBYTE asc, UBYTE ascq) {
	BytePackWrite8(sense, 0x70);
	BytePackWrite8(sense, 0);
	BytePackWrite8(sense, sensekey);
	BytePackWrite32MSB(sense, 0);
	BytePackWrite8(sense, 10);
	BytePackWrite32MSB(sense, 0);
	BytePackWrite8(sense, asc);
	BytePackWrite8(sense, ascq);
	BytePackWrite8(sense, 0);
	BytePackWrite24MSB(sense, 0);
}

LONG DoSCSICmd (struct IOStdReq *io, struct SCSICmd *scsi) {
	struct DiskImageUnit *unit = (struct DiskImageUnit *)io->io_Unit;
	APTR image = unit->ImageData;
	struct DiskImagePlugin *plugin = unit->Plugin;
	const UBYTE *cmd;
	ULONG cmd_len;
	BytePackBuffer data;
	BytePackBuffer sense;
	UBYTE status;
	
	if (io->io_Length < sizeof(struct SCSICmd)) {
		return IOERR_BADLENGTH;
	}
	io->io_Actual = sizeof(struct SCSICmd);
	
	cmd = scsi->scsi_Command;
	cmd_len = scsi->scsi_CmdLength;
	BytePackInit(&data, scsi->scsi_Data, scsi->scsi_Length);
	if (scsi->scsi_Flags & SCSIF_AUTOSENSE) {
		BytePackInit(&sense, scsi->scsi_SenseData, scsi->scsi_SenseLength);
	} else {
		BytePackInit(&sense, NULL, 0);
	}

	status = SCSI_Good;
	scsi->scsi_CmdActual = 1;
	switch (cmd[0]) {
		case SCSICMD_TEST_UNIT_READY:
			scsi->scsi_CmdActual = 6;
			if (cmd_len >= 6 && cmd[1] == 0 && cmd[2] == 0 && cmd[3] == 0 &&
				cmd[4] == 0 && cmd[5] == 0)
			{
				if (image && plugin) {
					status = SCSI_Good;
				} else {
					status = SCSI_CheckCondition;
					WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x3a, 0x00);
				}
			} else {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x24, 0x00);
			}
			break;
		
		case SCSICMD_INQUIRY:
			scsi->scsi_CmdActual = 6;
			if (cmd_len >= 6 && (cmd[1] & 0xfe) == 0 && cmd[3] == 0 && cmd[5] == 0) {
				if (cmd[1] & 1) {
					status = SCSI_Good;
					BytePackWrite8(&data, unit->DeviceType);
					BytePackWrite8(&data, 0x01);
				} else if (cmd[2] == 0) {
					status = SCSI_Good;
					BytePackWrite8(&data, unit->DeviceType);
					BytePackWrite8(&data, (unit->Flags & DGF_REMOVABLE) ? 0x80 : 0x00);
					BytePackWrite8(&data, 5);
					BytePackWrite8(&data, 0x02);
					BytePackWrite8(&data, 31);
					BytePackWrite24MSB(&data, 0);
					BytePackWriteText(&data, "a500.org", 8);
					BytePackWriteText(&data, "diskimage.device", 16);
					BytePackWriteText(&data, "DI52", 4);
				} else {
					status = SCSI_CheckCondition;
					WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x24, 0x00);
				}
			} else {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x24, 0x00);
			}
			break;
			
		case SCSICMD_READ_CAPACITY:
			scsi->scsi_CmdActual = 10;
			if (unit->DeviceType == DG_CDROM) {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x20, 0x00);
				break;
			}
			if (cmd_len >= 10 && cmd[1] == 0 && cmd[2] == 0 && cmd[3] == 0 &&
				cmd[4] == 0 && cmd[5] == 0 && cmd[6] == 0 && cmd[7] == 0 &&
				cmd[8] == 0 && cmd[9] == 0)
			{
				struct DriveGeometry dg;
				if (GetGeometry(unit, &dg)) {
					status = SCSI_Good;
					BytePackWrite32MSB(&data, dg.dg_TotalSectors-1);
					BytePackWrite32MSB(&data, dg.dg_SectorSize);
				} else {
					status = SCSI_CheckCondition;
					WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x3a, 0x00);
				}
			} else {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x24, 0x00);
			}
			break;
			
		case SCSICMD_READ_TOC:
			scsi->scsi_CmdActual = 10;
			if (unit->DeviceType != DG_CDROM) {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x20, 0x00);
				break;
			}
			if (cmd_len >= 10 && (cmd[1] & 0xfd) == 0 && cmd[2] == 0 &&
				cmd[3] == 0 && cmd[4] == 0 && cmd[5] == 0 && cmd[9] == 0)
			{
				if (image && plugin) {
					struct CDTrack *track = NULL;
					ULONG num_tracks = 0;
					struct CDTrack data_track;
					if (plugin->plugin_GetCDTracks) {
						Plugin_GetCDTracks(plugin, image, &track, &num_tracks);
					} else {
						struct DriveGeometry dg;
						if (GetGeometry(unit, &dg)) {
							ULONG total_sectors;
							total_sectors = ((UQUAD)dg.dg_TotalSectors * (UQUAD)dg.dg_SectorSize) >> 11;
							data_track.next = NULL;
							data_track.track_num = 1;
							data_track.audio = FALSE;
							data_track.sector_size = 2048;
							data_track.sync_size = 0;
							data_track.offset = 0;
							data_track.length = total_sectors << 11;
							data_track.sectors = total_sectors;
							track = &data_track;
							num_tracks = 1;
						}
					}
					if (track && num_tracks) {
						ULONG addr = 0;
						UBYTE type = 0x14;
						status = SCSI_Good;
						if (cmd[6]) {
							while (track && track->track_num != cmd[6]) {
								type = track->audio ? 0x10 : 0x14;
								addr += track->sectors;
								track = track->next;
								num_tracks--;
							}
						}
						BytePackWrite16MSB(&data, 2 + ((num_tracks + 1) * 8));
						BytePackWrite8(&data, 1);
						BytePackWrite8(&data, num_tracks);
						while (track) {
							type = track->audio ? 0x10 : 0x14;
							BytePackWrite8(&data, 0);
							BytePackWrite8(&data, type);
							BytePackWrite8(&data, track->track_num);
							BytePackWrite8(&data, 0);
							if (cmd[1] & 2) {
								uint8 m, s, f;
								ADDR2MSF(addr, m, s, f);
								BytePackWrite8(&data, 0);
								BytePackWrite8(&data, m);
								BytePackWrite8(&data, s);
								BytePackWrite8(&data, f);
							} else {
								BytePackWrite32MSB(&data, addr);
							}
							addr += track->sectors;
							track = track->next;
						}
						BytePackWrite8(&data, 0);
						BytePackWrite8(&data, type);
						BytePackWrite8(&data, 0xaa);
						BytePackWrite8(&data, 0);
						if (cmd[1] & 2) {
							uint8 m, s, f;
							ADDR2MSF(addr, m, s, f);
							BytePackWrite8(&data, 0);
							BytePackWrite8(&data, m);
							BytePackWrite8(&data, s);
							BytePackWrite8(&data, f);
						} else {
							BytePackWrite32MSB(&data, addr);
						}
					} else {
						status = SCSI_CheckCondition;
						WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x3a, 0x00);
					}
				} else {
					status = SCSI_CheckCondition;
					WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x3a, 0x00);
				}
			} else {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x24, 0x00);
			}
			break;
		
		case SCSICMD_READ_CD:
			scsi->scsi_CmdActual = 12;
			if (unit->DeviceType != DG_CDROM) {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x20, 0x00);
				break;
			}
			if (cmd_len >= 12 && (cmd[1] == 0x00 || cmd[1] == 0x04) &&
				cmd[9] == 0x10 && cmd[10] == 0 && cmd[11] == 0)
			{
				if (image && plugin) {
					if (plugin->plugin_ReadCDDA) {
						ULONG addr, frames;
						addr = rbe32(&cmd[2]);
						frames = rbe32(&cmd[6]) >> 8;
						status = SCSI_Good;
						if (frames > 0) {
							data.current = data.start + Plugin_ReadCDDA(plugin, image,
								data.start, addr, frames);
						}
					} else {
						status = SCSI_CheckCondition;
						WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x20, 0x00);
					}
				} else {
					status = SCSI_CheckCondition;
					WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x3a, 0x00);
				}
			} else {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x24, 0x00);
			}
			break;
			
		case SCSICMD_READ_CD_MSF:
			scsi->scsi_CmdActual = 12;
			if (unit->DeviceType != DG_CDROM) {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x20, 0x00);
				break;
			}
			if (cmd_len >= 12 && (cmd[1] == 0x00 || cmd[1] == 0x04) &&
				cmd[2] == 0 && cmd[9] == 0x10 && cmd[10] == 0 && cmd[11] == 0)
			{
				if (image && plugin) {
					if (plugin->plugin_ReadCDDA) {
						ULONG startaddr, endaddr;
						startaddr = MSF2ADDR(cmd[3], cmd[4], cmd[5]);
						endaddr = MSF2ADDR(cmd[6], cmd[7], cmd[8]);
						status = SCSI_Good;
						if (startaddr >= (2*75) && endaddr > startaddr) {
							ULONG addr = startaddr - (2*75);
							ULONG frames = endaddr - startaddr;
							data.current = data.start + Plugin_ReadCDDA(plugin, image,
								data.start, addr, frames);
						}
					} else {
						status = SCSI_CheckCondition;
						WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x20, 0x00);
					}
				} else {
					status = SCSI_CheckCondition;
					WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x3a, 0x00);
				}
			} else {
				status = SCSI_CheckCondition;
				WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x24, 0x00);
			}
			break;
			
		default:
			status = SCSI_CheckCondition;
			WriteSenseData(&sense, SENSEKEY_IllegalRequest, 0x20, 0x00);
			break;
	}
	
	scsi->scsi_Status = status;
	scsi->scsi_Actual = BytePackBytesWritten(&data);
	scsi->scsi_SenseActual = BytePackBytesWritten(&sense);
	
	if (status == SCSI_Good) {
		return IOERR_SUCCESS;
	} else {
		return HFERR_BadStatus;
	}
}
