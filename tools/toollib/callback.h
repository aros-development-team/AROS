#ifndef _CALLBACK_H
#define _CALLBACK_H

typedef void * CBD;
typedef int (*CB) (void * obj, int, CBD);

#define CallCB(cb,obj,cmd,data)     ((*(cb))(obj,cmd,data))

#endif /* _CALLBACK_H */
