
#define DETECTDEBUG 0

#ifdef KS13WRAPPER

#define KS13WRAPPER_DEBUG 0

#include <dos/dosextens.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <intuition/intuition.h>

#include "blocks.h"
#include "struct.h"

LONG wrapper_stackswap(LONG (*func)(struct ExecBase *), struct ExecBase *);
void FixStartupPacket(struct DosPacket *pkt, struct globaldata *g);

/* dos */
#define MatchPatternNoCase(pat, str) W_MatchPatternNoCase(pat, str, g)
#define MakeDosEntry(name, type) W_MakeDosEntry(name, type, g)
#define RemDosEntry(dlist) W_RemDosEntry(dlist, g)
#define FreeDosEntry(dlist) W_FreeDosEntry(dlist, g)
#define AddDosEntry(dlist) W_AddDosEntry(dlist, g)
#define ErrorReport(code, type, arg1, device) W_ErrorReport(code, type, arg1, device, g)

BOOL W_MatchPatternNoCase(CONST_STRPTR pat, CONST_STRPTR str, struct globaldata *g);
STRPTR FilePart(CONST_STRPTR path);
STRPTR PathPart(CONST_STRPTR path);
struct DosList *W_MakeDosEntry(CONST_STRPTR name, LONG type, struct globaldata *g);
LONG W_RemDosEntry(struct DosList *dlist, struct globaldata *g);
void W_FreeDosEntry(struct DosList *dlist, struct globaldata *g);
LONG W_AddDosEntry(struct DosList *list, struct globaldata *g);
BOOL W_ErrorReport(LONG code, LONG type, IPTR arg1, struct MsgPort *device, struct globaldata *g);

/* exec */
#define AllocVec(size, flags) W_AllocVec(size, flags, g)
#define FreeVec(mem) W_FreeVec(mem, g)
#define CreateIORequest(ioReplyPort, size) W_CreateIORequest(ioReplyPort, size, g)
#define DeleteIORequest(iorequest) W_DeleteIORequest(iorequest, g)
#define CreateMsgPort() W_CreateMsgPort(g)
#define DeleteMsgPort(port) W_DeleteMsgPort(port, g)

APTR W_AllocVec(ULONG size, ULONG flags, struct globaldata *g);
void W_FreeVec(APTR mem, struct globaldata *g);
APTR W_CreateIORequest(struct MsgPort *ioReplyPort, ULONG size, struct globaldata *g);
void W_DeleteIORequest(APTR iorequest, struct globaldata *g);
struct MsgPort *W_CreateMsgPort(struct globaldata *g);
void W_DeleteMsgPort(struct MsgPort *port, struct globaldata *g);

/* utility */
IPTR CallHookPkt(struct Hook *hook, APTR object, APTR paramPacket);

/* intuition */
#define EasyRequestArgs(window, easyStruct, IDCMP_ptr, argList) W_EasyRequestArgs(window, easyStruct, IDCMP_ptr, argList, g)
#define BuildEasyRequestArgs(RefWindow, easyStruct, IDCMP, Args) W_BuildEasyRequestArgs(RefWindow, easyStruct, IDCMP, Args, g)
#define SysReqHandler(window, IDCMPFlagsPtr, WaitInput) W_SysReqHandler(window, IDCMPFlagsPtr, WaitInput, g)

LONG W_EasyRequestArgs(struct Window *window, struct EasyStruct *easyStruct, ULONG *IDCMP_ptr, APTR argList, struct globaldata *g);
struct Window *W_BuildEasyRequestArgs(struct Window *RefWindow, struct EasyStruct *easyStruct, ULONG IDCMP, APTR Args, struct globaldata *g);
LONG W_SysReqHandler(struct Window *window, ULONG *IDCMPFlagsPtr, BOOL WaitInput, struct globaldata *g);

void DebugPutStr(register const char *buff);
void DebugPutDec(const char *what, ULONG val);
void DebugPutHex(const char *what, IPTR val);
void DebugPutHexVal(ULONG val);

#endif


