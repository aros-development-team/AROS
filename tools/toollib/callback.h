#ifndef TOOLLIB_CALLBACK_H
#define TOOLLIB_CALLBACK_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

typedef void * CBD;
typedef int (*CB) (void * obj, const void *data, CBD);

#define CallCB(cb,obj,cmd,data)     ((*(cb))(obj,cmd,data))

#endif /* TOOLLIB_CALLBACK_H */
