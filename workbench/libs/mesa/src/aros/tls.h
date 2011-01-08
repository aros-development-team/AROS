/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef TLS_H
#define TLS_H

#include <exec/types.h>
#include <aros/symbolsets.h>

struct TaskLocalStorage;

struct TaskLocalStorage * CreateTLS();
VOID InsertIntoTLS(struct TaskLocalStorage * tls, APTR ptr);
APTR GetFromTLS(struct TaskLocalStorage * tls);
VOID ClearFromTLS(struct TaskLocalStorage * tls);
VOID DestroyTLS(struct TaskLocalStorage * tls);

#define DECLARE_STATIC_TLS(tls)                 \
static struct TaskLocalStorage * tls = NULL;    \
static void auto_crate_##tls()                  \
{                                               \
    tls = CreateTLS();                          \
}                                               \
static void auto_destroy_##tls()                \
{                                               \
    DestroyTLS(tls);                            \
}                                               \
ADD2INIT(auto_crate_##tls, 5);                  \
ADD2EXIT(auto_destroy_##tls, 5);

#endif /* TLS_H */
