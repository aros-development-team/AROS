/*
** BTStackLoader loads the ENVARC:Sys/bluetooth.prefs file and starts the stack.
** This is AROS specific, as under AmigaOS, the BTStackLoader is the config itself
** and under MorphOS, IPrefs loads the prefs.
**
** It also binds the pluggable controller-firmware loaders: every module in
** DEVS:Bluetooth/FWLoaders/ is opened once, which self-registers its
** BtFirmwareLoader with bluetooth.library (see rom/bluetooth/firmware/). The
** opens are intentionally never closed, so the loaders stay resident and remain
** available to controllers that are plugged in later.
*/

#include <exec/exec.h>
#include <dos/dosextens.h>
#include <dos/exall.h>

#include <proto/bluetooth.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

const char *psd_version = "$VER: BTStackLoader 1.1 (25.08.2026) (ported from the Poseidon shell commands by Chris Hodges)";

#define FWLOADERPATH    "DEVS:Bluetooth/FWLoaders"
#define FWPATHMAX       256

/* Open every firmware-loader module in DEVS:Bluetooth/FWLoaders/ so it binds
   itself to the stack. The directory is optional - a machine with no such
   controllers simply has none. */
static void bLoadFirmwareLoaders(void)
{
    UBYTE buf[1024];
    UBYTE path[FWPATHMAX];
    struct ExAllControl *exall;
    struct ExAllData *exdata;
    BPTR lock;
    ULONG ents, namelen;
    BOOL exready;

    if(!(lock = Lock(FWLOADERPATH, ACCESS_READ))) {
        return;
    }
    if((exall = AllocDosObject(DOS_EXALLCONTROL, NULL))) {
        exall->eac_LastKey = 0;
        exall->eac_MatchString = NULL;
        exall->eac_MatchFunc = NULL;
        do {
            exready = ExAll(lock, (struct ExAllData *) buf, sizeof(buf), ED_NAME, exall);
            exdata = (struct ExAllData *) buf;
            ents = exall->eac_Entries;
            while(ents--) {
                namelen = strlen((char *) exdata->ed_Name);
                if(!(((namelen > 4) && !strcmp((char *) &exdata->ed_Name[namelen-4], ".dbg")) ||
                     ((namelen > 5) && !strcmp((char *) &exdata->ed_Name[namelen-5], ".info")))) {
                    if((sizeof(FWLOADERPATH) + namelen + 1) < FWPATHMAX) {
                        strcpy((char *) path, FWLOADERPATH "/");
                        strcat((char *) path, (char *) exdata->ed_Name);
                        /* opening runs the module's init, which registers it;
                           keep it open for the session (deliberately no close). */
                        OpenLibrary((STRPTR) path, 0);
                    }
                }
                exdata = exdata->ed_Next;
            }
        } while(exready);
        FreeDosObject(DOS_EXALLCONTROL, exall);
    }
    UnLock(lock);
}

int main(void)
{
    struct Library *BluetoothBase;
    int ret = RETURN_FAIL;
    if((BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        bLoadFirmwareLoaders();
        if(btLoadCfgFromDisk(NULL))
        {
            ret = RETURN_OK;
            btParseCfg();
        } else {
            /* First boot with the stack: no prefs yet. Write the defaults
               so that pairings made from now on can be saved into the
               file automatically (the library only auto-saves once a prefs
               file exists, to avoid clobbering one it has not read). */
            if(btSaveCfgToDisk(NULL, FALSE))
            {
                ret = RETURN_WARN;
                PutStr("No bluetooth.prefs found - created one with the defaults.\n");
                btParseCfg();
            } else {
                ret = RETURN_ERROR;
                PutStr("Error loading bluetooth.prefs!\n");
            }
        }
        CloseLibrary(BluetoothBase);
    } else {
        PutStr("Unable to open bluetooth.library\n");
    }
    return(ret);
}
