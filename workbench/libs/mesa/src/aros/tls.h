/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef TLS_H
#define TLS_H

#include <exec/types.h>

struct TaskLocalStorage;

struct TaskLocalStorage * CreateTLS();
VOID InsertIntoTLS(struct TaskLocalStorage * tls, APTR ptr);
APTR GetFromTLS(struct TaskLocalStorage * tls);
VOID ClearFromTLS(struct TaskLocalStorage * tls);
VOID DestroyTLS(struct TaskLocalStorage * tls);

#endif /* TLS_H */
