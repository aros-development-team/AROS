
#ifndef OBSERVER_H
#define OBSERVER_H

#define OA_InTree                 TAG_USER+101
#define OA_Presentation           TAG_USER+102

struct ObserverClassData
{
	Object *presentation;
};

struct __dummyObserverData__
{
    struct MUI_NotifyData mnd;
    struct ObserverClassData ocd;
};

#define observerData(obj) (&(((struct __dummyObserverData__ *)(obj))->ocd))

#define _presentation(obj)    (observerData(obj)->presentation)


#endif
