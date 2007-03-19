#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <prefs/trackdisk.h>
#include <utility/tagitem.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "trackdiskprefs.h"

struct td_UnitPrefs TDPrefs[TD_NUMUNITS];
struct IORequest TDIO;

void LoadPrefs(void)
{
	BPTR cf;
	struct TagItem PrefsBuf;
	ULONG Unit;

	cf = Open(TRACKDISK_PREFS_NAME, MODE_OLDFILE);
	if (cf) {
		Unit = 0;
		while (FRead(cf, &PrefsBuf, sizeof(PrefsBuf), 1)) {
			if (PrefsBuf.ti_Tag == TDPR_UnitNum)
				Unit = PrefsBuf.ti_Data;
			else {
				if (Unit < TD_NUMUNITS) {
					switch (PrefsBuf.ti_Tag)
					{
					case TDPR_PubFlags:
						TDPrefs[Unit].PubFlags = PrefsBuf.ti_Data;
						break;
					case TDPR_RetryCnt:
						TDPrefs[Unit].RetryCnt = PrefsBuf.ti_Data;
						break;
					}
				}
			}
			if (PrefsBuf.ti_Tag == TAG_DONE)
				break;
		}
		Close(cf);
	}
}

void ReadTDPrefs(void)
{
    int i;
    struct TDU_PublicUnit *tdu;
    
    for (i = 0; i < TD_NUMUNITS; i++) {
	TDPrefs[i].PubFlags = 0;
	TDPrefs[i].RetryCnt = 3;
    }
    LoadPrefs();
    for (i = 0; i < TD_NUMUNITS; i++) {
	if (!OpenDevice("trackdisk.device", i, &TDIO, 0)) {
	    tdu = (struct TDU_PublicUnit *)TDIO.io_Unit;
	    tdu->tdu_PubFlags = TDPrefs[i].PubFlags;
	    tdu->tdu_RetryCnt = TDPrefs[i].RetryCnt;
	    CloseDevice(&TDIO);
	}
    }
}
