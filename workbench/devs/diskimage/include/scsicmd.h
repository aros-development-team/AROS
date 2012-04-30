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

#ifndef SCSICMD_H
#define SCSICMD_H

#ifndef DEVICES_SCSIDISK_H
#include <devices/scsidisk.h>
#endif

/* command codes */
#define SCSICMD_TEST_UNIT_READY				0x00
#define SCSICMD_INQUIRY						0x12
#define SCSICMD_READ_CAPACITY				0x25
#define SCSICMD_READ_TOC					0x43
#define SCSICMD_READ_CD						0xbe
#define SCSICMD_READ_CD_MSF					0xb9

/* status codes */
#define SCSI_Good							0x00
#define SCSI_CheckCondition					0x02
#define SCSI_ConditionMet					0x04
#define SCSI_Busy							0x08
#define SCSI_Intermediate					0x10
#define SCSI_Intermediate_ConditionMet		0x14
#define SCSI_ReservationConflict			0x18
#define SCSI_CommandTerminated				0x22
#define SCSI_TaskSetFull					0x28
#define SCSI_ACAActive						0x30

/* sense key codes */
#define SENSEKEY_NoSense					0x0
#define SENSEKEY_RecoveredError				0x1
#define SENSEKEY_NotReady					0x2
#define SENSEKEY_MediumError				0x3
#define SENSEKEY_HardwareError				0x4
#define SENSEKEY_IllegalRequest				0x5
#define SENSEKEY_UnitAttention				0x6
#define SENSEKEY_DataProtect				0x7
#define SENSEKEY_BlankCheck					0x8
#define SENSEKEY_VendorSpecific				0x9
#define SENSEKEY_CopyAborted				0xA
#define SENSEKEY_AbortedCommand				0xB
#define SENSEKEY_VolumeOverflow				0xD
#define SENSEKEY_Miscompare					0xE

#define SAMPLESPERFRAME (2352UL / 4UL)

#define ADDR2MSF(x,m,s,f) do { \
	m = ((x) / 75UL) / 60UL; \
	s = ((x) / 75UL) % 60UL; \
	f = (x) % 75UL; \
	} while (0)
#define MSF2ADDR(m,s,f) \
	(((ULONG)(m) * (75UL*60UL)) + \
	((ULONG)(s) * 75UL) + \
	(ULONG)(f))

#define ADDR2HMSF(x,h,m,s,f) do { \
	h = (((x) / 75UL) / 60UL) / 60UL; \
	m = (((x) / 75UL) / 60UL) % 60UL; \
	s = ((x) / 75UL) % 60UL; \
	f = (x) % 75UL; \
	} while (0)
#define HMSF2ADDR(h,m,s,f) \
	(((ULONG)(h) * (75UL*60UL*60UL)) + \
	((ULONG)(m) * (75UL*60UL)) + \
	((ULONG)(s) * 75UL) + \
	(ULONG)(f))

#define MS2FRAMES(ms) (((ULONG)(ms) * 75UL) / 1000UL)
#define FRAMES2MS(fr) (((ULONG)(ms) * 1000UL) / 75UL)
#define MS2MSF(ms) do { \
	minute = ((ULONG)(ms) / 1000UL) / 60UL; \
	second = ((ULONG)(ms) / 1000UL) % 60UL; \
	frame = MS2FRAMES((ULONG)(ms) % 1000UL); \
	} while (0)
#define MSF2MS(m,s,f) (((ULONG)(m) * (60UL * 1000UL)) \
	((ULONG)(s) * 1000UL) + FRAMES2MS(f))

#endif
