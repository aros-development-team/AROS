/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* variables.h -- Prototypes to all functions related to variables */
#ifndef _VARIABLES_H
#define _VARIABLES_H

/* Internal function prototypes */
extern void *get_variable(char *);
extern char *get_var_arg(char *);
extern long int get_var_int(char *);
extern void set_variable(char *, char *, long int);
extern void set_preset_variables(int);
#ifdef DEBUG
extern void dump_varlist();
#endif /* DEBUG */
extern void free_varlist();

#endif /* _VARIABLES_H */

