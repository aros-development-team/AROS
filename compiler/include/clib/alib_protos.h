#ifndef  CLIB_ALIB_PROTOS_H
#define  CLIB_ALIB_PROTOS_H

/*
**	$VER: alib_protos.h 1.0 (26.10.95)
**
**	C prototypes for things in amiga.lib.
**
*/

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif

/*
    Prototypes
*/
ULONG DoMethodA (Object * obj, Msg message);
ULONG DoMethod (Object * obj, ULONG MethodID, ...);
ULONG DoGadgetMethod (struct Gadget * gad, struct Window * win,
		    struct Requester * req, ULONG MethodID, ...);
ULONG DoSuperMethodA (Class  * cl, Object * obj, Msg message);
ULONG DoSuperMethod (Class * cl, Object * obj, ULONG MethodID, ...);
void SetAttrs (Object * obj, ULONG tag1, ...);
APTR NewObject (struct IClass * classPtr, UBYTE * classID, ULONG tag1, ...);

struct Window * OpenWindowTags (struct NewWindow * newWindow, ULONG tag1, ...);
struct Screen * OpenScreenTags (struct NewScreen * newScreen, ULONG tag1, ...);

#endif /* CLIB_ALIB_PROTOS_H */
