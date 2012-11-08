
#define DETECTDEBUG 0

#ifdef KS13WRAPPER

#define KS13WRAPPER_DEBUG 0

#include <dos/dosextens.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <intuition/intuition.h>

LONG wrapper_stackswap(LONG (*func)(struct ExecBase *), struct ExecBase *);
/* dos */
BOOL MatchPatternNoCase(CONST_STRPTR pat, CONST_STRPTR str);
STRPTR FilePart(CONST_STRPTR path);
STRPTR PathPart(CONST_STRPTR path);
struct DosList *MakeDosEntry(CONST_STRPTR name, LONG type);
LONG RemDosEntry(struct DosList *dlist);
void FreeDosEntry(struct DosList *dlist);
LONG AddDosEntry(struct DosList *list);
BOOL ErrorReport(LONG code, LONG type, IPTR arg1, struct MsgPort *device);
/* exec */
APTR AllocVec(ULONG size, ULONG flags);
void FreeVec(APTR mem);
APTR CreateIORequest(struct MsgPort *ioReplyPort, ULONG size);
void DeleteIORequest(APTR iorequest);
struct MsgPort *CreateMsgPort(void);
void DeleteMsgPort(struct MsgPort *port);
/* utility */
IPTR CallHookPkt(struct Hook *hook, APTR object, APTR paramPacket);
/* intuition */
LONG EasyRequestArgs(struct Window *window, struct EasyStruct *easyStruct, ULONG *IDCMP_ptr, APTR argList);
struct Window *BuildEasyRequestArgs(struct Window *RefWindow, struct EasyStruct *easyStruct, ULONG IDCMP, APTR Args);
LONG SysReqHandler(struct Window *window, ULONG *IDCMPFlagsPtr, BOOL WaitInput);

void DebugPutStr(register const char *buff);
void DebugPutDec(const char *what, ULONG val);
void DebugPutHex(const char *what, IPTR val);
void DebugPutHexVal(ULONG val);

#endif


