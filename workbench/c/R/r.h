#ifndef R_H
#define R_H

/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MAX_ARG_CNT (30)
#define MAX_NAME_CNT (30)

struct CArg
{
    TEXT argname[MAX_NAME_CNT];
    BOOL a_flag;
    BOOL f_flag;
    BOOL k_flag;
    BOOL m_flag;
    BOOL n_flag;
    BOOL s_flag;
};

struct Req
{
    struct RDArgs *rda;
    STRPTR filename;
    STRPTR profile;
    BOOL nogui;
    STRPTR arguments;

    TEXT cmd_template[500];

    ULONG arg_cnt;
    struct CArg cargs[MAX_ARG_CNT]; // TODO: dynamic allocation
};



void create_gui(struct Req *req);

#endif // R_H
