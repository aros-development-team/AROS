/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* procedure.h -- Prototypes for all functions related to user-defined procedures */

#ifndef _PROCEDURE_H
#define _PROCEDURE_H

extern struct ProcedureList *find_proc(char *);
extern long int set_procedure(char **, int, ScriptArg *);
extern void free_proclist();
extern void link_function(char *, long int);

#endif /* _PROCEDURE_H */

