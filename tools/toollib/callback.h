#ifndef TOOLLIB_CALLBACK_H
#define TOOLLIB_CALLBACK_H

typedef void * CBD;
typedef int (*CB) (void * obj, int, CBD);

#define CallCB(cb,obj,cmd,data)     ((*(cb))(obj,cmd,data))

#endif /* TOOLLIB_CALLBACK_H */
