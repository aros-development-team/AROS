/*
 * This file implements the runtime module function, Export.
 */

//#define DEBUG 1
#include <aros/debug.h>

#include "dynmodule_modules.h"

#include <dynmod/dynmodstack.h>

#include <stdlib.h>
#include <string.h>

// module provided symbols...
extern dynmod_export_t DYNMODULE_Exports[];

BOOL dynmodule__InternalMatchSymbol(const char *sym1, const char *sym2)
{
    return (strcmp(sym1, sym2) == 0);
}

int dynmoduleExport(dynmod_sym_t *sym)
{
    D(bug("[DynLink] %s(0x%p)\n", __func__, sym));

    if (sym->SymPtr) {
        dynmod_export_t *symtable = DYNMODULE_Exports;
        while (symtable->ExportAddress) {
            if(dynmodule__InternalMatchSymbol(symtable->ExportName, sym->SymName)) {
                *sym->SymPtr = symtable->ExportAddress;
                return 1;
            }
            symtable++;
        }

        *sym->SymPtr = NULL;
    }
    return 0;
}
