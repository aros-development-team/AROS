/* $Id$
 *
 *      dummy.c - unimplemented netdb functions 
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *      Copyright © 2005 - 2011 Pavel Fedin
 */

#include <sys/types.h>
#include <netdb.h>

struct netent  *getnetent(void)
{
    return NULL;
}

void endnetent(void)
{

}

struct servent  *getservent(void)
{
    return NULL;
}

void endservent(void)
{

}
