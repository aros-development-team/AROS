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
#ifndef  DOS_DOS_H
#   include <dos/dos.h>
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

BOOL ReadByte	(BPTR fh, UBYTE  * dataptr);
BOOL ReadWord	(BPTR fh, UWORD  * dataptr);
BOOL ReadLong	(BPTR fh, ULONG  * dataptr);
BOOL ReadFloat	(BPTR fh, FLOAT  * dataptr);
BOOL ReadDouble (BPTR fh, DOUBLE * dataptr);
BOOL ReadString (BPTR fh, STRPTR * dataptr);
BOOL ReadStruct (BPTR fh, IPTR * desc, APTR * dataptr);
BOOL WriteByte	 (BPTR fh, UBYTE  data);
BOOL WriteWord	 (BPTR fh, UWORD  data);
BOOL WriteLong	 (BPTR fh, ULONG  data);
BOOL WriteFloat  (BPTR fh, FLOAT  data);
BOOL WriteDouble (BPTR fh, DOUBLE data);
BOOL WriteString (BPTR fh, STRPTR data);
BOOL WriteStruct (BPTR fh, IPTR * desc, APTR data);
void FreeStruct (APTR s, IPTR * desc);

#endif /* CLIB_ALIB_PROTOS_H */
