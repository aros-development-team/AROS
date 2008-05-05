#ifndef DOS_VAR_H
#define DOS_VAR_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

/* This structure is used by ScanVars() function to pass information about
   local and/or global variables to specified hook function. Note that this
   structure is READ-ONLY and its content is valid only during ScanVars()
   hook function call. Don't try to use a pointer to this structure outside
   ScanVars() hook function.
*/
struct ScanVarsMsg 
{
    ULONG sv_SVMSize;  /* Size of ScanVarsMsg structure */
    ULONG sv_Flags;    /* The flags parameter given to ScanVars() */
    STRPTR sv_GDir;    /* Directory patch for global variables or empty string
                          "\0" for local variables */
    STRPTR sv_Name;    /* Name of the variable */
    STRPTR sv_Var;     /* Pointer to the contents of the variable */
    ULONG sv_VarLen;   /* Size of the variable */
};

/* lv_Node.ln_Type */
#define LV_VAR   0 /* This is a variable. */
#define LV_ALIAS 1 /* This is an alias. */
/* This flag may be or'ed into lv_Node.ln_Type. It means that dos.library
   should ignore this entry. */
#define LVB_IGNORE 7
#define LVF_IGNORE (1L<<LVB_IGNORE)

/* The following flags are used as flags for the dos variable functions. 
    GVB_BINARY_VAR and GVB_DONT_NULL_TERM are also saved in lv_Flags.	
*/
  /* The variable is not to be used locally. */
#define GVB_GLOBAL_ONLY    8
  /* The variable is not to be used globally. */
#define GVB_LOCAL_ONLY     9
  /* The variable is a binary variable. lv_Value points to binary data. */
#define GVB_BINARY_VAR     10
  /* lv_Value is not null-terminated. This is only allowed, if GVB_BINARY_VAR
     is also set. */
#define GVB_DONT_NULL_TERM 11
  /* This flag tells dos to save the variable to ENVARC: too. */
#define GVB_SAVE_VAR       12

#define GVF_GLOBAL_ONLY    (1L<<GVB_GLOBAL_ONLY)
#define GVF_LOCAL_ONLY     (1L<<GVB_LOCAL_ONLY)
#define GVF_BINARY_VAR     (1L<<GVB_BINARY_VAR)
#define GVF_DONT_NULL_TERM (1L<<GVB_DONT_NULL_TERM)
#define GVF_SAVE_VAR       (1L<<GVB_SAVE_VAR)

#endif /* DOS_VAR_H */
