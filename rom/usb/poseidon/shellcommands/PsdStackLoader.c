/*
** PsdStackloader loads the ENVARC:Sys/poseidon.prefs file and starts the stack.
** This is AROS specific, as under AmigaOS, the PsdStackloader is the config itself
** and under MorphOS, IPrefs loads the prefs.
*/

#include <proto/poseidon.h>
#include <proto/exec.h>
#include <proto/dos.h>

const char *psd_version = "$VER: PsdStackloader 4.1 (17.09.26) by Chris Hodges <chrisly@platon42.de>";

int main(void)
{
    struct Library *ps;
    int ret = RETURN_FAIL;
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        if(psdLoadCfgFromDisk(NULL))
        {
            ret = RETURN_OK;
            psdParseCfg();
        } else {
            ret = RETURN_ERROR;
            PutStr("Error loading poseidon.prefs!\n");
            /* Without a config there is nothing to parse, but DOS is available
               now, so the stack still gets to rebind devices that were claimed
               by the boot classes (bootmouse/bootkeyboard) before DOS was up. */
            psdClassScan();
        }
        CloseLibrary(ps);
    } else {
        PutStr("Unable to open poseidon.library\n");
    }
    return(ret);
}
