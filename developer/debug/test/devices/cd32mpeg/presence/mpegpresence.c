/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <libraries/configvars.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/expansion.h>

struct Library *ExpansionBase;
static ULONG checks, failures;

static void check(BOOL okay, CONST_STRPTR text)
{
    checks++;
    if (!okay)
    {
        failures++;
        bug("MPEG PRESENCE FAIL: %s\n", text);
    }
}

int main(int argc, char **argv)
{
    BOOL present, registered;
    BPTR version;
    LONG result;
    char args[] = "cd32mpeg.device\n";

    ExpansionBase = (APTR)OpenLibrary("expansion.library", 0);
    if (!ExpansionBase)
        return 20;
    present = FindConfigDev(NULL, 514, 106) != NULL;
    check((FindResident("cd32mpeg.device") != NULL) == present,
          "resident visibility matches cartridge presence");
    Forbid();
    registered = FindName(&SysBase->DeviceList, "cd32mpeg.device") != NULL;
    Permit();
    check(registered == present, "device registration matches cartridge presence");

    /* Exercise the command used by game startup scripts, including its ROM
     * resident search. Pass a Version executable path on a minimal boot disk.
     */
    version = LoadSeg(argc > 1 ? argv[1] : "C:Version");
    check(version != BNULL, "Version command is available");
    if (version)
    {
        result = RunCommand(version, 16384, args, sizeof(args) - 1);
        check(present ? result == 0 : result >= 5,
              "Version selects MPEG only with a cartridge");
        UnLoadSeg(version);
    }
    CloseLibrary((APTR)ExpansionBase);
    bug("MPEG PRESENCE TEST: cartridge %s, %lu/%lu pass, %lu failures\n",
        present ? "present" : "absent", checks - failures, checks, failures);
    return failures ? 20 : 0;
}
