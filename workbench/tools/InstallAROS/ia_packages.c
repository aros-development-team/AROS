/*
    Copyright © 2018-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include "ia_locale.h"
#include "ia_install.h"
#include "ia_packages.h"

extern char *source_Path;

static struct List PACKAGELIST;

#define PACKAGEDIR_BASE "Prefs/Env-Archive/SYS/Packages"

void PACKAGES_InitSupport(void)
{
    D(bug("[InstallAROS] %s()\n", __func__));

    NEWLIST(&PACKAGELIST);
    // TODO: enumerate available packages .. 
}

void PACKAGES_AddCoreSkipPaths(struct List *SkipList)
{
    ULONG skipPathLen = strlen(source_Path) + 100;
    TEXT skipPath[skipPathLen];

    D(bug("[InstallAROS] %s()\n", __func__));

#if (1) // Hard coded for now ...
    strcpy(skipPath, source_Path);
    AddPart(skipPath, "Prefs/Env-Archive/SYS/Packages/Developer", skipPathLen);
    AddSkipListEntry(SkipList, skipPath);
#endif
    // TODO: remove core package dirs .. 
}

void PACKAGES_DoInstall(Class * CLASS, Object * self)
{
    D(bug("[InstallAROS] %s()\n", __func__));
}
