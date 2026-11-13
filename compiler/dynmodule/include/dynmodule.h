#ifndef DYNMODULE_DYNMODULE_H
#define DYNMODULE_DYNMODULE_H

/*
 * Public definitions for dynmodule support
 */

#include <dynmod/dynmodstack.h>

typedef struct dynmodModule
{
    const char                  *Name;
    const char                  *Port;
} dynmod_mod_t;

typedef struct dynmodSymbol
{
    dynmod_stackf_t             StackFType;
    const char *                SymName;
    void **                     SymPtr;
} dynmod_sym_t;

typedef struct dynmodImport
{
    void                        **ImportPtr;
    const char                  *ImportName;
    dynmod_mod_t                ImportModule;
} dynmod_import_t;

typedef struct dynmodExport
{
    void                        *ExportAddress;
    const char                  *ExportName;
} dynmod_export_t;

/*
 * Typedefs for standard function vectors, implemented in module.
 */
typedef void *(*dynmoduleFindResourceFn_t)(int, const char *);
typedef void *(*dynmoduleLoadResourceFn_t)(void *);
typedef void  (*dynmoduleFreeResourceFn_t)(void *);

typedef void (*stdexitfunc_t)(void);

#ifdef __cplusplus
extern "C" {
#endif

void    *dynmoduleLoadModule(const char *, const char *);
void    dynmoduleFreeModule(void *);
int     dynmoduleImport(void);
int     dynmoduleExport(dynmod_sym_t *);
void    *dynmoduleGetProcAddress(void *, const char *);
int     dynmoduleRemoveModule(const char *);

/*
 * Prototypes for module provided structures/implementations
 */
extern const char       *DYNMODULE_Name;
extern dynmod_export_t  DYNMODULE_Exports[];
extern dynmod_import_t  DYNMODULE_Imports[];

extern int              DYNMODULE_Init(void *, unsigned long int, stdexitfunc_t);
extern int              DYNMODULE_Setup(void *);
extern void             DYNMODULE_Cleanup(void);
extern void             DYNMODULE_Exit(int);

/*
 * Prototypes for standard implementations exposed by the glue
 */
extern void             *__dynglue_FindResource(int, const char *);
extern void             *__dynglue_LoadResource(void *);
extern void             __dynglue_FreeResource(void *);

#ifdef __cplusplus
}
#endif

#endif
