#ifndef PREFSFILE_H
#define PREFSFILE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ENV:Envoy/services.prefs reading and writing
          (re/spec/services-accounts.md §3): FORM PREF, a PRHD chunk, one
          ISVC chunk of 328 bytes per service.
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#define PREFS_PATHSIZE          221     /* device-qualified path field                  */
#define PREFS_SHORTPATHSIZE     35      /* the same path without the device name        */
#define PREFS_NAMESIZE          64

struct PrefsEntry
{
    struct Node pe_Node;
    char        pe_Name[PREFS_NAMESIZE];
    char        pe_Path[PREFS_PATHSIZE];
    BOOL        pe_Active;
};

/* Reads a prefs file into the list (struct PrefsEntry nodes, AllocVec).
 * Returns the number of services, -1 if the file could not be read or is
 * not a services prefs file. */
LONG ReadServicesPrefs(CONST_STRPTR filename, struct List *entries);

/* Writes the list as a prefs file; creates the directory if needed. */
BOOL WriteServicesPrefs(CONST_STRPTR filename, struct List *entries);

struct PrefsEntry *FindPrefsEntry(struct List *entries, CONST_STRPTR name);
struct PrefsEntry *AddPrefsEntry(struct List *entries, CONST_STRPTR name, CONST_STRPTR path, BOOL active);
void FreePrefsEntries(struct List *entries);

#endif /* PREFSFILE_H */
