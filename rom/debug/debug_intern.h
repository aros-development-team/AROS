/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/config.h>
#include <dos/bptr.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <libraries/debug.h>

typedef struct
{
    char *s_name;               /* Symbol name */
    void *s_lowest;             /* Start address */
    void *s_highest;            /* End address */
} dbg_sym_t;

struct segment;

typedef struct
{
    struct MinNode  m_node;         /* For linking into a list */
    BPTR            m_seg;          /* First segment pointer */
    char            *m_shstr;       /* Section headers table */
    char            *m_str;         /* Symbol names table */
    struct segment  **m_segments;   /* Sorted array of segments */
    unsigned int    m_segcnt;       /* Count of segments */
#if AROS_MODULES_DEBUG
    char            *m_seggdbhlp;   /* Pre-built string for add-symbol-file */
#endif
    dbg_sym_t       *m_symbols;     /* Array of associated symbols */
    unsigned long   m_symcnt;       /* Number of symbols in the array */
    void            *m_lowest;      /* Lowest address of all segments */
    void            *m_highest;     /* Highest address of all segments */
    void            *m_gaplowest;   /* Lowest address of biggest gap */
    void            *m_gaphighest;  /* Highest address of biggest gap */
    char            m_name[1];      /* Module name, variable length */
} module_t;

struct segment
{
    struct MinNode  s_node;     /* For linking into the list */
    BPTR            s_seg;      /* DOS segment pointer */
    void *          s_lowest;   /* Start address */
    void *          s_highest;  /* End address */
    module_t *      s_mod;      /* Module descriptor */
    char *          s_name;     /* Segment name */
    unsigned int    s_num;      /* Segment number */
};

struct DebugBase
{
    struct Library          db_Lib;
    struct MinList          db_Modules;
    struct SignalSemaphore  db_ModSem;
    APTR                    db_KernelBase;
};

#define DBGBASE(x) ((struct DebugBase *)x)
#define KernelBase  DBGBASE(DebugBase)->db_KernelBase

void RegisterModule_ELF(const char *name, BPTR segList, struct elfheader *eh, struct sheader *sections,
        struct Library *DebugBase);
