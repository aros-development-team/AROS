/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

struct LayersBase
{
    struct Library   lb_LibNode;

    struct Library * lb_GfxBase;
    struct ExecBase *lb_SysBase;
};
