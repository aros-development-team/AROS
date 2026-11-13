#ifndef __DYNMODULE_MODULES_H
#define __DYNMODULE_MODULES_H

/*
 * Definitions for the internal module entries
 */

#include "dynmodule_intern.h"

#define DYNMODULE_MAX       20

typedef struct dynModuleEntry
{
    __dynmoduleinstance_t           *mhandle;
    int                             opencnt;
    char                            pnam[100];
} __dynmoduleentry_t;

extern   __dynmoduleentry_t         dynmoduleslots[];
extern int                          dynmodopncnt;

int dynmodule__InternalFindFreeSlot(int *);
__dynmoduleinstance_t *dynmodule__InternalAllocDynModEntry(struct MsgPort **);
void dynmodule__InternalInitDynModEntry(int, __dynmoduleinstance_t *, const char *);
void dynmodule__InternalDestroyDynModEntry(__dynmoduleinstance_t *, struct MsgPort *);
__dynmoduleinstance_t *dynmodule__InternalOpenPortEntry(const char *, BOOL);
struct MsgPort *dynmodule__InternalBootstrapDynMod(const char *, const char *, int);

#endif
