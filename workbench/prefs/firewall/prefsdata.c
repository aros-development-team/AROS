/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Firewall Preferences -- orchestration and rule application.
    Delegates filter and NAT rule I/O to filter.c and nat.c respectively.
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#include "prefsdata.h"

/*
 * Load existing rules from the given directory.
 */
void
InitFirewallPrefs(STRPTR path)
{
    ReadFilterConf(path);
    ReadNATConf(path);
}

/*
 * Apply the current rules by running ipf and ipnat commands.
 */
BOOL
ApplyFirewallRules(void)
{
    char cmd[512];

    /* Flush existing filter rules and load new ones */
    SystemTags("ipf -Fa", SYS_Asynch, FALSE, SYS_Output, BNULL, TAG_DONE);

    if (numFilterRules > 0)
    {
        snprintf(cmd, sizeof(cmd), "ipf -f %s/%s", PREFS_PATH_ENV, IPF_CONF_NAME);
        SystemTags(cmd, SYS_Asynch, FALSE, SYS_Output, BNULL, TAG_DONE);
    }

    /* Flush existing NAT rules and load new ones */
    SystemTags("ipnat -CF", SYS_Asynch, FALSE, SYS_Output, BNULL, TAG_DONE);

    if (numNatRules > 0)
    {
        snprintf(cmd, sizeof(cmd), "ipnat -f %s/%s", PREFS_PATH_ENV, IPNAT_CONF_NAME);
        SystemTags(cmd, SYS_Asynch, FALSE, SYS_Output, BNULL, TAG_DONE);
    }

    return TRUE;
}

/*
 * Save: write to ENVARC (persistent) and ENV (runtime), then apply.
 */
BOOL
SaveFirewallPrefs(void)
{
    BOOL ok = TRUE;

    ok = ok && WriteFilterConf(PREFS_PATH_ENVARC);
    ok = ok && WriteNATConf(PREFS_PATH_ENVARC);
    ok = ok && WriteFilterConf(PREFS_PATH_ENV);
    ok = ok && WriteNATConf(PREFS_PATH_ENV);

    if (ok)
        ApplyFirewallRules();

    return ok;
}

/*
 * Use: write to ENV (runtime) only, then apply.
 */
BOOL
UseFirewallPrefs(void)
{
    BOOL ok = TRUE;

    ok = ok && WriteFilterConf(PREFS_PATH_ENV);
    ok = ok && WriteNATConf(PREFS_PATH_ENV);

    if (ok)
        ApplyFirewallRules();

    return ok;
}

/*
 * Handle USE/SAVE command-line arguments (no GUI).
 */
void
Prefs_HandleArgs(BOOL use, BOOL save)
{
    if (save)
        SaveFirewallPrefs();
    else if (use)
        UseFirewallPrefs();
}
