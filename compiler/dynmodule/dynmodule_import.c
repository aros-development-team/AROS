/*
 * This file implements the runtime module function, Import.
 */

//#define DEBUG 1
#include <aros/debug.h>

#include "dynmodule_modules.h"

#include <dynmod/dynmodstack.h>

#include <stdlib.h>
#include <string.h>

// module needed symbols...
extern dynmod_import_t DYNMODULE_Imports[];

int dynmoduleImport()
{
    int cnt = 0;
    dynmod_import_t *symtable = DYNMODULE_Imports;

    D(bug("[DynLink] %s()\n", __func__));

    while (symtable->ImportPtr) {
        void *mhandle;
        void *ptr;

        if (((mhandle = dynmodule__InternalLoadModule(symtable->ImportModule.Name, symtable->ImportModule.Port, 0)) == NULL) ||
            ((ptr = dynmoduleGetProcAddress(mhandle, symtable->ImportName)) == NULL))
            return 0;

        *symtable->ImportPtr = ptr;
        symtable++;
        cnt++;
    }

    D(bug("[DynLink] %s: imported %u symbols\n", __func__, cnt));

    return cnt;
}
