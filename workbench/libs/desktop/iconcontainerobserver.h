
#ifndef ICONCONTAINER_OBSERVER_H
#define ICONCONTAINER_OBSERVER_H

#define ICOA_Directory            TAG_USER+1
#define ICOA_Presentation         TAG_USER+2

struct IconContainerObserverClassData
{
	Object *presentation;
	UBYTE *directory;
	BPTR dirLock;
};

#endif
