/* System-private methods and attributes of monitorclass */

/* Attributes */

#define MA_AROS_PRIVATE		(TAG_USER + 0x00020000)

#define MA_MonitorHandle	(MA_AROS_PRIVATE + 1)   /* [I..] struct MonitorHandle * graphics.library monitor handle */
#define MA_PointerVisible	(MA_AROS_PRIVATE + 2)	/* [.S.] BOOL                   Mouse pointer is visible        */
#define MA_MonitorID            (MA_AROS_PRIVATE + 3)   /* [.G.] ULONG                  Monitor ID                      */

/* Methods */

#define MM_GetCompositionFlags 0x2401 /* Ask display composition flags			 */
#define MM_SetPointerPos       0x2402 /* Set mouse pointer position			 */
#define MM_CheckID	       0x2403 /* Check if the given mode ID matches this monitor */
#define MM_SetPointerShape     0x2404 /* Set mouse pointer shape			 */
#define MM_SetScreenGamma      0x2405 /* Set screen-specific gamma correction table      */
#define MM_FindBest3dDepth     0x2406 /* Find best depth with 3D support                 */
#define MM_Calc3dCapability    0x2407 /* Calculate 3D capability index                   */

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

struct msSetPointerShape
{
    STACKED ULONG MethodID;
    STACKED struct SharedPointer *pointer;
};

struct msSetScreenGamma
{
    STACKED ULONG MethodID;
    STACKED struct GammaControl *gamma;
    STACKED ULONG force;
};

struct msFindBest3dDepth
{
    STACKED ULONG MethodID;
    STACKED ULONG depth;
};

static inline ULONG FindBest3dDepth(ULONG depth, void *obj, struct IntuitionBase *IntuitionBase)
{
    return DoMethod(obj, MM_FindBest3dDepth, depth);
}

Object *DisplayDriverNotify(APTR obj, BOOL add, struct IntuitionBase *IntuitionBase);
