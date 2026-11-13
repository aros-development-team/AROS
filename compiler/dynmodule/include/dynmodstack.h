#ifndef DYNMODULE_DYNMODSTACK_H
#define DYNMODULE_DYNMODSTACK_H

/*
 * Definitions used to handle the correct stack frame format
 * on supported targets/toolchains. These are to define the stack
 * used by the module opener.
 *
 * the glue code will patch in the correct function pointer based
 * the requested stack frame type.
 */

// "Generic" Targets.
#define DYNMOD_STACKB_GCC               0
#define DYNMOD_STACKB_SYSV              1

// M68k Targets
#define DYNMOD_STACKB_EGCS              DYNMOD_STACKB_GCC
#define DYNMOD_STACKB_VBCC              2                       /* vbcc         */
#define DYNMOD_STACKB_STORM             3                       /* StormC       */
#define DYNMOD_STACKB_SAS               4                       /* SAS/C        */

// PPC targets
#define DYNMOD_STACKB_EGCSPPC           DYNMOD_STACKB_SYSV
#define DYNMOD_STACKB_POWEROPEN         5                       /* StormC/vbcc  */

typedef enum
{
    DYNMOD_STACKF_STORM         = (1 << DYNMOD_STACKB_STORM),
    DYNMOD_STACKF_GCC           = (1 << DYNMOD_STACKB_GCC),
    DYNMOD_STACKF_SAS           = (1 << DYNMOD_STACKB_SAS),
    DYNMOD_STACKF_VBCC          = (1 << DYNMOD_STACKB_VBCC),
    DYNMOD_STACKF_POWEROPEN     = (1 << DYNMOD_STACKB_POWEROPEN),
    DYNMOD_STACKF_SYSV          = (1 << DYNMOD_STACKB_SYSV)
} dynmod_stackf_t;

#ifdef __PPC
    #if (defined __STORM__) || (defined __VBCC__)
    # define DYNMOD_STACKF_DEFAULT        DYNMOD_STACKF_POWEROPEN
    #elif (defined __GNUC__)
    # define DYNMOD_STACKF_DEFAULT        DYNMOD_STACKF_SYSV
    #endif
#else
    #if (defined __VBCC__)
    # define DYNMOD_STACKF_DEFAULT        DYNMOD_STACKF_VBCC
    #elif (defined __STORM__)
    # define DYNMOD_STACKF_DEFAULT        DYNMOD_STACKF_STORM
    #elif (defined __SASC__)
    # define DYNMOD_STACKF_DEFAULT        DYNMOD_STACKF_SAS
    #elif (defined __GNUC__)
    # define DYNMOD_STACKF_DEFAULT        DYNMOD_STACKF_GCC
    #endif
#endif

#endif
