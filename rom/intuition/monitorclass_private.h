/* System-private methods and attributes of monitorclass */

/* Attributes */

#define MA_AROS_PRIVATE		(TAG_USER + 0x00020000)

#define MA_DriverObject		(MA_AROS_PRIVATE + 1)   /* [I..] OOP_Object * Display driver object    */
#define MA_PointerVisible	(MA_AROS_PRIVATE + 2)	/* [.S.] BOOL         Mouse pointer is visible */

/* Methods */

#define MM_GetCompositionFlags 0x2401 /* Ask display composition flags */
#define MM_SetPointerPos       0x2402 /* Set mouse pointer position    */

struct msGetCompositionFlags
{
    STACKED ULONG MethodID;
    STACKED ULONG ModeID;
};

struct msSetPointerPos
{
    STACKED ULONG MethodID;
    STACKED ULONG x;
    STACKED ULONG y;
};

Object *DisplayDriverNotify(APTR obj, BOOL add, struct IntuitionBase *IntuitionBase);
