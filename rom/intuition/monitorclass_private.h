/* System-private methods and attributes of monitorclass */

/* Attributes */

#define MA_AROS_PRIVATE		(TAG_USER + 0x00020000)

#define MA_DriverObject		(MA_AROS_PRIVATE + 1)   /* [I..] OOP_Object * Display driver object */

/* Methods */

#define MM_GetCompositionFlags 0x2401 /* Ask display composition flags */

struct msGetCompositionFlags
{
    STACKED ULONG MethodID;
    STACKED ULONG ModeID;
};

Object *DisplayDriverNotify(APTR obj, BOOL add, struct IntuitionBase *IntuitionBase);
