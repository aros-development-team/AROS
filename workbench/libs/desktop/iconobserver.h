
#ifndef ICONOBSERVERCLASS_H
#define ICONOBSERVERCLASS_H

#include "observer.h"

#define IO_Base TAG_USER+2100

#define IOM_Execute   IO_Base+1
#define IOA_Selected  IO_Base+2
#define IOA_Name      IO_Base+3
#define IOA_Directory IO_Base+4

struct IconObserverClassData
{
	BOOL selected;
	UBYTE *name, *directory;
};

struct __dummyIconObsData__
{
    struct MUI_NotifyData mnd;
	struct ObserverClassData ocd;
    struct IconObserverClassData icd;
};

#define iconObsData(obj) (&(((struct __dummyIconObsData__ *)(obj))->icd))

#define _name(obj)         (iconObsData(obj)->name)
#define _directory(obj)    (iconObsData(obj)->directory)

#endif
