#ifndef DOS_VAR_H
#define DOS_VAR_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Environment Variables
    Lang: english
*/

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

/* In the Process structure, there is a minimal list of these
 * structures in pr_LocalVars.
 *
 * Use SetVar() to add new variable/aliases to the list. Never allocate these
 * structures yourself and link them into the pr_LocalVars list.
 */
struct LocalVar {
    struct Node   lv_Node;
    UWORD         lv_Flags;
    UBYTE       * lv_Value;
    ULONG         lv_Len;
};

/* These are defined for use in lv_Node.ln_Type.
 */

/* Specifiy a variable (local/global) or an shell alias.
 *
 * Due to bugs in dos.library, global variables can only use
 * the LV_VAR type.
 */
#define LV_VAR              0
#define LV_ALIAS            1

/* When using the DOS variable functions, any variable/alias with
 * this type will be ignored.
 */
#define LVB_IGNORE          7
#define LVF_IGNORE          (1L << LVB_IGNORE)

#define GVB_GLOBAL_ONLY     8
#define GVF_GLOBAL_ONLY     (1L << GVB_GLOBAL_ONLY)
#define GVB_LOCAL_ONLY      9
#define GVF_LOCAL_ONLY      (1L << GVB_LOCAL_ONLY)
#define GVB_BINARY_VAR      10
#define GVF_BINARY_VAR      (1L << GVB_BINARY_VAR)
#define GVB_DONT_NULL_TERM  11
#define GVF_DONT_NULL_TERM  (1L << GVB_DONT_NULL_TERM)

/* From V39+, this allows the variable to be
 * saved into the ENVARC: directory.
 */
#define GVB_SAVE_VAR        12
#define GVF_SAVE_VAR        (1L << GVB_SAVE_VAR)

#endif /* DOS_VAR_H */
