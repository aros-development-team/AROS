/*
    Copyright 2010-2011, The AROS Development Team. All rights reserved.
    $Id: tls.h 56280 2019-04-17 18:42:57Z NicJA $
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
static LONG auto_create_##tls()                  \
{                                               \
    tls = CreateTLS();                          \
    if (tls)                                    \
        return 1;                               \
    else                                        \
        return 0;                               \
}                                               \
                                                \
static VOID auto_destroy_##tls()                \
{                                               \
    if (tls)                                    \
        DestroyTLS(tls);                        \
}                                               \
ADD2INIT(auto_create_##tls, 5);                  \
ADD2EXIT(auto_destroy_##tls, 5);

#endif /* TLS_H */
