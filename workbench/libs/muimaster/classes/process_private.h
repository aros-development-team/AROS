#ifndef _PROCESS_PRIVATE_H_
#define _PROCESS_PRIVATE_H_

/*** Instance data **********************************************************/
struct Process_DATA
{
    Object *self;
    BOOL autolaunch;
    STRPTR name;
    LONG priority;
    struct IClass *sourceclass;
    Object *sourceobject;
    ULONG stacksize;
    struct Process *task;
    ULONG kill;
};

#endif /* _PROCESS_PRIVATE_H_ */
