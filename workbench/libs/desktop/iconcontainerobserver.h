
#ifndef ICONCONTAINER_OBSERVER_H
#define ICONCONTAINER_OBSERVER_H

#define ICOA_Directory            TAG_USER+1
#define ICOA_InTree               TAG_USER+2

#define ICOM_AddIcons             TAG_USER+12

struct IconContainerObserverClassData
{
	UBYTE *directory;
	BPTR dirLock;
};

struct icoAddIcon
{
	Msg methodID;
	ULONG wsr_Results;
	struct SingleResult *wsr_ResultsArray;
};

#endif
