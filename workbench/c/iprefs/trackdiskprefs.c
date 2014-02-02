/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <prefs/trackdisk.h>
#include <utility/tagitem.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "global.h"
#include "trackdiskprefs.h"

struct td_UnitPrefs TDPrefs[TD_NUMUNITS];
struct IORequest TDIO;

void LoadPrefs(void)
{
	BPTR cf;
	ULONG PrefsBuf[2];
	ULONG Unit;

	cf = Open(TRACKDISK_PREFS_NAME, MODE_OLDFILE);
	if (cf) {
		Unit = 0;
		while (FRead(cf, &PrefsBuf, sizeof(PrefsBuf), 1)) {

			if (PrefsBuf[0] == TDPR_UnitNum)
				Unit = PrefsBuf[1];
			else {
				if (Unit < TD_NUMUNITS) {
					switch (PrefsBuf[0])
					{
					case TDPR_PubFlags:
						TDPrefs[Unit].PubFlags = PrefsBuf[1];
						break;
					case TDPR_RetryCnt:
						TDPrefs[Unit].RetryCnt = PrefsBuf[1];
						break;
					}
				}
			}
			if (PrefsBuf[0] == TAG_DONE)
				break;
		}
		Close(cf);
	}
}

void ReadTDPrefs(void)
{
    int i;
    struct TDU_PublicUnit *tdu;

    if (FindResident(TD_NAME)) {
        for (i = 0; i < TD_NUMUNITS; i++) {
            TDPrefs[i].PubFlags = 0;
            TDPrefs[i].RetryCnt = 3;
        }
        LoadPrefs();
        for (i = 0; i < TD_NUMUNITS; i++) {
            if (!OpenDevice(TD_NAME, i, &TDIO, 0)) {
                tdu = (struct TDU_PublicUnit *)TDIO.io_Unit;
                tdu->tdu_PubFlags = TDPrefs[i].PubFlags;
                tdu->tdu_RetryCnt = TDPrefs[i].RetryCnt;
                CloseDevice(&TDIO);
            }
        }
    }
}
