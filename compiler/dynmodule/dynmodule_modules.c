/*
 * Suppport functions and structures for the internal module entries
 */

//#define DEBUG 1
#include <aros/debug.h>

#include "dynmodule_modules.h"

#include <dos/dos.h>
#include <dos/dostags.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <proto/exec.h>
#include <proto/dos.h>

struct dynmodulesupport {
    int                 dms_opencnt;
    int                 dms_flags;
    __dynmoduleentry_t  *dms_modules;
};

int                dynmodopncnt = 0;
__dynmoduleentry_t dynmoduleslots[DYNMODULE_MAX];

static const char *dynmodule_output_str = "CON:0/0/800/600/%s/AUTO/CLOSE/WAIT";
static const char *dynmodule_cmd_template = "\"%s\" \"%s\"";

void dynmodule__InternalInitDynModEntry(int slotid, __dynmoduleinstance_t *dynmod, const char *port)
{
    dynmodopncnt++;
    dynmoduleslots[slotid].mhandle = dynmod;
    dynmoduleslots[slotid].opencnt = 1;
    strcpy((char *)dynmoduleslots[slotid].pnam, port);
}

int dynmodule__InternalFindFreeSlot(int *slotid)
{
    int slotcur;
    for (slotcur = 0; slotcur < DYNMODULE_MAX; slotcur++)
        if(!dynmoduleslots[slotcur].mhandle)
            break;
    if (slotcur == DYNMODULE_MAX)
        return 0;
    *slotid = slotcur;
    return 1;
}

__dynmoduleinstance_t *dynmodule__InternalAllocDynModEntry(struct MsgPort **dmifport)
{
    __dynmoduleinstance_t *dynmod;

    if ((!dmifport) || ((dynmod = malloc(sizeof(__dynmoduleinstance_t))) == NULL))
        return NULL;

    D(bug("[DynLink] %s: entry @ 0x%p\n", __func__, dynmod));

    if (!(*dmifport = CreateMsgPort())) {
        free(dynmod);
        return NULL;
    }
    return dynmod;
}

__dynmoduleinstance_t *dynmodule__InternalOpenPortEntry(const char *port, BOOL registeropen)
{
    int slotcur;
    for (slotcur = 0; slotcur < DYNMODULE_MAX; slotcur++) {
        if(dynmoduleslots[slotcur].mhandle) {
            if(strcmp((const char *)dynmoduleslots[slotcur].pnam, port) == 0) {
                if(registeropen)
                    dynmoduleslots[slotcur].opencnt++;
                return dynmoduleslots[slotcur].mhandle;
            }
        }
    }
    return NULL;
}

struct MsgPort *dynmodule__InternalBootstrapDynMod(const char *modname, const char *port, int timeout)
{
    char *outputfmt;
    char *commandline;
    BPTR input = BNULL, output = BNULL;

    if ((outputfmt = AllocVec(strlen(port) + strlen(dynmodule_output_str) - 1, MEMF_PUBLIC)) != NULL) {
        sprintf(outputfmt, dynmodule_output_str, port);
        output = Open(outputfmt, MODE_NEWFILE);
    }

    commandline = AllocVec(strlen(modname) + strlen(port) + 6, MEMF_PUBLIC);
    if (commandline) {
        struct TagItem cmdTags[] = {
            { NP_StackSize, AROS_STACKSIZE  },
            { SYS_Input,    (IPTR)input     },
            { SYS_Output,   (IPTR)output    },
            { SYS_Asynch,   TRUE            },
            { TAG_DONE,     0               }
        };
        struct MsgPort *dmifport = NULL;
#if !defined(__AMIGA__)
#define TICKSPERSEC 50
#else
        int TICKSPERSEC;
        if ((SysBase->AttnFlags & AFB_NTSC) != 0)
            TICKSPERSEC = 60;
        else
            TICKSPERSEC = 50;
#endif
        int iter, itermax, to = TICKSPERSEC/2;
        if (timeout > 1)
            itermax = (timeout * TICKSPERSEC) / to;
        else
            itermax = 1;
        sprintf(commandline, dynmodule_cmd_template, modname, port);

        D(
            bug("[DynLink] %s: bootstrapping '%s'\n", __func__, commandline);
            bug("[DynLink] %s:       input @ 0x%p\n", __func__, commandline, input);
            bug("[DynLink] %s:      output @ 0x%p\n", __func__, commandline, output);
        )
        SystemTagList(commandline, cmdTags);

        D(bug("[DynLink] %s: waiting %usecs for module to load ...\n", __func__, ((itermax * to)/TICKSPERSEC)));
        for (iter = 0; iter < itermax; iter++) {
            if ((dmifport = FindPort(port)) != NULL)
                break;
            Delay(to);
        }
        FreeVec(commandline);
        D(bug("[DynLink] %s: returning 0x%p\n", __func__, dmifport));
        return dmifport;
    }
    return NULL;
}

void dynmodule__InternalDestroyDynModEntry(__dynmoduleinstance_t *dynmod, struct MsgPort *dmifport)
{
    if (dmifport)
        DeleteMsgPort(dmifport);
    free(dynmod);    
}

