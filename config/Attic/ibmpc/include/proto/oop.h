#ifndef PROTO_OOP_H
#define PROTO_OOP_H

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef OOP_BASE_NAME
#define OOP_BASE_NAME OOPBase
#endif

#define NewObjectA(classPtr, classID, tagList) \
	LP3(0x14, APTR, NewObjectA, struct IClass *, classPtr, UBYTE *, classID, struct TagItem *, tagList, \
	, OOP_BASE_NAME)

#define GetID(stringID) \
	LP1(0x18, STRPTR, GetID, STRPTR, stringID, \
	, OOP_BASE_NAME)

#define DoSuperMethodA(class, object, msg) \
	LP3(0x1c, IPTR, DoSuperMethodA, struct IClass *, classPtr, Object *, object, Msg, msg, \
	, OOP_BASE_NAME)

/* More functions missing */

#endif /* PROTO_OOP_H */
