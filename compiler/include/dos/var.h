#ifndef DOS_VAR_H
#define DOS_VAR_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Environment variable handling.
    Lang: english
*/
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif


/* This structure describes a local variable. The list is normally held in
   Process->pr_LocalVars. See <dos/dosextens.h> for more information about
   the list. Note that this structure is READ-ONLY! Allocate it with SetVar().
*/
struct LocalVar {
    struct Node   lv_Node;  /* Standard node structure as defined in
                               <exec/nodes.h>. See also below. */
    UWORD         lv_Flags;
    UBYTE       * lv_Value; /* The contents of the variable. */
    ULONG         lv_Len;   /* The length of the contents. */
};

/* lv_Node.ln_Type */
#define LV_VAR   0 /* This is a variable. */
#define LV_ALIAS 1 /* This is an alias. */
/* This flag may be or'ed into lv_Node.ln_Type. It means that dos.library
   should ignore this entry. */
#define LVB_IGNORE 7
#define LVF_IGNORE (1L<<LVB_IGNORE)

/* The following flags may be or'ed into lv_Node.ln_Type, but may also be
   used as flags for the dos variable functions. */
#define GVB_GLOBAL_ONLY    8
  /* The variable is not to be used locally. */
#define GVB_LOCAL_ONLY     9
  /* The variable is not to be used globally. */
#define GVB_BINARY_VAR     10
  /* The variable is a binary variable. lv_Value points to binary data. */
#define GVB_DONT_NULL_TERM 11
  /* lv_Value is not null-terminated. This is only allowed, if GVB_BINARY_VAR
     is also set. */
#define GVB_SAVE_VAR       12
  /* This flag tells dos to save the variable to ENVARC: too. */

#define GVF_GLOBAL_ONLY    (1L<<GVB_GLOBAL_ONLY)
#define GVF_LOCAL_ONLY     (1L<<GVB_LOCAL_ONLY)
#define GVF_BINARY_VAR     (1L<<GVB_BINARY_VAR)
#define GVF_DONT_NULL_TERM (1L<<GVB_DONT_NULL_TERM)
#define GVF_SAVE_VAR       (1L<<GVB_SAVE_VAR)

#endif /* DOS_VAR_H */
