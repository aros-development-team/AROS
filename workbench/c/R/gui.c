/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/muimaster.h>

#include <stdio.h>

#include "r.h"

void create_gui(struct Req *req)
{
    LONG i;

    printf(" i Name                      A F K M N S\n");
    for (i = 0; i < req->arg_cnt; i++)
    {
        printf
        (
            "%2d %-25s %c %c %c %c %c %c\n",
            i, req->cargs[i].argname,
            req->cargs[i].a_flag ? 'X' : '-',
            req->cargs[i].f_flag ? 'X' : '-',
            req->cargs[i].k_flag ? 'X' : '-',
            req->cargs[i].m_flag ? 'X' : '-',
            req->cargs[i].n_flag ? 'X' : '-',
            req->cargs[i].s_flag ? 'X' : '-'
        );
    }
}
