#ifndef _R_H
#define _R_H

/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <intuition/classusr.h>


struct CArg
{
    TEXT *argname;
    Object *object;
    BOOL a_flag;
    BOOL f_flag;
    BOOL k_flag;
    BOOL m_flag;
    BOOL n_flag;
    BOOL s_flag;
    BOOL t_flag;
};

struct Req
{
    struct RDArgs *rda;
    STRPTR filename;
    STRPTR profile;
    BOOL nogui;
    STRPTR arguments;

    TEXT *cmd_template; // template string from outfile

    ULONG arg_cnt;
    struct CArg *cargs;

    BOOL do_execute; // TRUE if Execute button was clicked
};


extern APTR poolmem;


BOOL create_gui(struct Req *req);
BOOL handle_gui(struct Req *req);
void cleanup_gui(void);
BOOL get_gui_bool(struct CArg *carg);
CONST_STRPTR get_gui_string(struct CArg *carg);

#define VERSION "$VER: R 1.1 (04.06.2012)"

#endif // R_H
