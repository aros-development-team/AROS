/*
** BTStackLoader loads the ENVARC:Sys/bluetooth.prefs file and starts the stack.
** This is AROS specific, as under AmigaOS, the BTStackLoader is the config itself
** and under MorphOS, IPrefs loads the prefs.
*/

#include <proto/bluetooth.h>
#include <proto/exec.h>
#include <proto/dos.h>

const char *psd_version = "$VER: BTStackLoader 1.0 (18.08.2026) (ported from the Poseidon shell commands by Chris Hodges)";

int main(void)
{
    struct Library *BluetoothBase;
    int ret = RETURN_FAIL;
    if((BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        if(btLoadCfgFromDisk(NULL))
        {
            ret = RETURN_OK;
            btParseCfg();
        } else {
            ret = RETURN_ERROR;
            PutStr("Error loading bluetooth.prefs!\n");
        }
        CloseLibrary(BluetoothBase);
    } else {
        PutStr("Unable to open bluetooth.library\n");
    }
    return(ret);
}
