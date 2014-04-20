
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <string.h>

#include "ks13wrapper.h"

#ifdef KS13WRAPPER

#if KS13WRAPPER_DEBUG

#include <hardware/intbits.h>

#ifndef D
#define D(x) ;
#endif

#define SERDATR			0x18
#define SERDAT			0x30
#define INTREQ			0x9c
#define SERDATR_TBE		(1 << 13)	/* Tx Buffer Empty */
#define SERDAT_STP8		(1 << 8)
#define SERDAT_DB8(x)		((x) & 0xff)

#endif

/* Must use DOSBase, CDTV extended ROM 2.x includes v37 SysBase replacement */
#define ISKS20 (DOSBase->dl_lib.lib_Version >= 37)

#define MIN_STACKSIZE 8000

AROS_UFP2(void, StackSwap,
    AROS_UFPA(struct StackSwapStruct*, stack, A0),
    AROS_UFPA(struct ExecBase*, SysBase, A6));

#if KS13WRAPPER_DEBUG

static inline void reg_w(ULONG reg, UWORD val)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	*r = val;
}
static inline UWORD reg_r(ULONG reg)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	return *r;
}
static void DebugPutChar(register int chr)
{
	if (chr == '\n')
		DebugPutChar('\r');

	while ((reg_r(SERDATR) & SERDATR_TBE) == 0);
	reg_w(INTREQ, INTF_TBE);

	/* Output a char to the debug UART */
	reg_w(SERDAT, SERDAT_STP8 | SERDAT_DB8(chr));
}
void DebugPutStr(register const char *buff)
{
	for (; *buff != 0; buff++)
		DebugPutChar(*buff);
}
void DebugPutDec(const char *what, ULONG val)
{
	int i, num;
	DebugPutStr(what);
	DebugPutStr(": ");
	if (val == 0) {
	    DebugPutChar('0');
	    DebugPutChar('\n');
	    return;
	}

	for (i = 1000000000; i > 0; i /= 10) {
	    if (val == 0) {
	    	DebugPutChar('0');
	    	continue;
	    }

	    num = val / i;
	    if (num == 0)
	    	continue;

	    DebugPutChar("0123456789"[num]);
	    val -= num * i;
	}
	DebugPutChar('\n');
}
void DebugPutHex(const char *what, ULONG val)
{
	int i;
	DebugPutStr(what);
	DebugPutStr(": ");
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar('\n');
}
void DebugPutHexVal(ULONG val)
{
	int i;
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar(' ');
}

#endif

APTR W_AllocVec(ULONG size, ULONG flags, struct globaldata *g)
{
    ULONG *mem;
    
    if (!size)
    	return NULL;
    size += sizeof(ULONG);
    mem = AllocMem(size, flags);
    if (!mem)
    	return NULL;
    mem[0] = size;
    mem++;
    return mem;
}
void W_FreeVec(APTR mem, struct globaldata *g)
{
    ULONG *p = mem;
    if (!p)
    	return;
    p--;
    FreeMem(p, p[0]);
}

APTR W_CreateIORequest(struct MsgPort *ioReplyPort, ULONG size, struct globaldata *g)
{
    struct IORequest *ret=NULL;
    if(ioReplyPort==NULL)
	return NULL;
    ret=(struct IORequest *)AllocMem(size,MEMF_PUBLIC|MEMF_CLEAR);
    if(ret!=NULL)
    {
	ret->io_Message.mn_ReplyPort=ioReplyPort;
	ret->io_Message.mn_Length=size;
    }
    return ret;
}

void W_DeleteIORequest(APTR iorequest, struct globaldata *g)
{
    if(iorequest != NULL)
	FreeMem(iorequest, ((struct Message *)iorequest)->mn_Length);
}

struct MsgPort *W_CreateMsgPort(struct globaldata *g)
{
    struct MsgPort *ret;
    ret=(struct MsgPort *)AllocMem(sizeof(struct MsgPort),MEMF_PUBLIC|MEMF_CLEAR);
    if(ret!=NULL)
    {
	BYTE sb;
	sb=AllocSignal(-1);
	if (sb != -1)
	{
	    ret->mp_Flags = PA_SIGNAL;
	    ret->mp_Node.ln_Type = NT_MSGPORT;
	    NEWLIST(&ret->mp_MsgList);
	    ret->mp_SigBit=sb;
	    ret->mp_SigTask=SysBase->ThisTask;
	    return ret;
	}
	FreeMem(ret,sizeof(struct MsgPort));
    }
    return NULL;
}

void W_DeleteMsgPort(struct MsgPort *port, struct globaldata *g)
{
    if(port!=NULL)
    {
	FreeSignal(port->mp_SigBit);
	FreeMem(port,sizeof(struct MsgPort));
    }
}

BOOL W_MatchPatternNoCase(CONST_STRPTR pat, CONST_STRPTR str, struct globaldata *g)
{
    if (ISKS20)
	return AROS_LVO_CALL2(BOOL,
		AROS_LCA(CONST_STRPTR, pat, D1),
		AROS_LCA(CONST_STRPTR, str, D2),
		struct DosLibrary*, DOSBase, 162, );
    /* Used by ACTION_EXAMINE_ALL which is 2.0+ only packet */
    return FALSE;
}

STRPTR FilePart(CONST_STRPTR path)
{
    if(path)
    {
	CONST_STRPTR i;
	if (!*path)
	  return (STRPTR)path;
	i = path + strlen (path) -1;
	while ((*i != ':') && (*i != '/') && (i != path))
	    i--;
	if ((*i == ':')) i++;
	if ((*i == '/')) i++;
	return (STRPTR)i;
    }
    return NULL;
}

STRPTR PathPart(CONST_STRPTR path)
{
    const char *ptr;

    while (*path == '/')
    {
	++path;
    }
    ptr = path;
    while (*ptr)
    {
	if (*ptr == '/')
	{
	    path = ptr;
	}
	else if (*ptr == ':')
	{
	    path = ptr + 1;
	}
	ptr++;
    }
    return (STRPTR)path;
}

static BOOL CMPBSTR(BSTR s1, BSTR s2)
{
    UBYTE *ss1 = BADDR(s1);
    UBYTE *ss2 = BADDR(s2);
    return memcmp(ss1, ss2, ss1[0] + 1);
}
static struct DosList *getdoslist(struct globaldata *g)
{
    struct DosInfo *di;
    
    di = BADDR(DOSBase->dl_Root->rn_Info);
    Forbid();
    return (struct DosList *)&di->di_DevInfo;
}
static void freedoslist(struct globaldata *g)
{
    Permit();
}

struct DosList *W_MakeDosEntry(CONST_STRPTR name, LONG type, struct globaldata *g)
{
    ULONG len;
    STRPTR s2;
    struct DosList *dl;

    if (ISKS20)
	return AROS_LVO_CALL2(struct DosList*,
		AROS_LCA(CONST_STRPTR, name, D1),
		AROS_LCA(LONG, type, D2),
		struct DosLibrary*, DOSBase, 116, );

    len = strlen(name);
    dl = (struct DosList *)AllocVec(sizeof(struct DosList),
				    MEMF_PUBLIC | MEMF_CLEAR);

    if (dl != NULL)
    {
	/* Binary compatibility for BCPL string.
	 * First byte is the length then comes the string.
	 * For ease of use a zero is put at the end so it can be used as a
	 * C string
	 */
	s2 = (STRPTR)AllocVec(len+2, MEMF_PUBLIC | MEMF_CLEAR);
	dl->dol_Name = MKBADDR(s2);
	if (s2 != NULL)
	    *s2++ = (UBYTE)(len > 255 ? 255 : len);
	if (s2 != NULL)
	{
	    strcpy(s2, name);
	    dl->dol_Type = type;
	    return dl;
	}
	FreeVec(dl);
    }
    return NULL;
}

LONG W_RemDosEntry(struct DosList *dlist, struct globaldata *g)
{
    struct DosList *dl;

    if (ISKS20)
	return AROS_LVO_CALL1(LONG,
		AROS_LCA(struct DosList*, dlist, D1),
		struct DosLibrary*, DOSBase, 112, );

    if(dlist == NULL)
	return 0;

    dl = getdoslist(g);

    while(dl->dol_Next)
    {
        struct DosList *dl2 = BADDR(dl->dol_Next);

        if(dl2 == dlist)
	{
	    dl->dol_Next = dlist->dol_Next;
	    break;
	}

	dl = dl2;
    }

    freedoslist(g);

    return 1;
}

void W_FreeDosEntry(struct DosList *dlist, struct globaldata *g)
{
    if (ISKS20) {
	AROS_LVO_CALL1(void,
		AROS_LCA(struct DosList*, dlist, D1),
		struct DosLibrary*, DOSBase, 117, );
	return;
    }
    if (dlist == NULL)
    	return;
    FreeVec(BADDR(dlist->dol_Name));
    FreeVec(dlist);
}

LONG W_AddDosEntry(struct DosList *dlist, struct globaldata *g)
{
    LONG success = DOSTRUE;
    struct DosList *dl;

    if (ISKS20)
	return AROS_LVO_CALL1(LONG,
		AROS_LCA(struct DosList*, dlist, D1),
		struct DosLibrary*, DOSBase, 113, );

    if (dlist == NULL)
    	return success;

    D(bug("[AddDosEntry] Adding '%b' type %d from addr %x Task '%s'\n",
        dlist->dol_Name, dlist->dol_Type, dlist,
        FindTask(NULL)->tc_Node.ln_Name));

    dl = getdoslist(g);

    if(dlist->dol_Type != DLT_VOLUME)
    {
	while(TRUE)
	{
	    dl = BADDR(dl->dol_Next);

	    if(dl == NULL)
		break;

	    if(dl->dol_Type != DLT_VOLUME && !CMPBSTR(dl->dol_Name, dlist->dol_Name))
	    {
		D(bug("[AddDosEntry] Name clash for %08lx->dol_Name: %b and %08lx->dol_Name %b\n", dl, dl->dol_Name, dlist, dlist->dol_Name));
		success = DOSFALSE;
		break;
	    }
	}
    }

    if(success)
    {
        struct DosInfo *dinf = BADDR(DOSBase->dl_Root->rn_Info);

	dlist->dol_Next = dinf->di_DevInfo;
	dinf->di_DevInfo = MKBADDR(dlist);
    }

    freedoslist(g);

    return success;    
}

IPTR CallHookPkt(struct Hook *hook, APTR object, APTR paramPacket)
{
    return CALLHOOKPKT(hook, object, paramPacket);
}

BOOL W_ErrorReport(LONG code, LONG type, IPTR arg1, struct MsgPort *device, struct globaldata *g)
{
    if (ISKS20)
	return AROS_LVO_CALL4(BOOL,
		AROS_LCA(LONG, code, D1),
		AROS_LCA(LONG, type, D2),
		AROS_LCA(IPTR, arg1, D3),
		AROS_LCA(struct MsgPort*, device, D4),
		struct DosLibrary*, DOSBase, 80, );

    return FALSE;
}

LONG W_EasyRequestArgs(struct Window *window, struct EasyStruct *easyStruct, ULONG *IDCMP_ptr, APTR argList, struct globaldata *g)
{
    if (ISKS20)
	return AROS_LVO_CALL4(LONG,
		AROS_LCA(struct Window*, window, A0),
		AROS_LCA(struct EasyStruct*, easyStruct, A1),
		AROS_LCA(ULONG*, IDCMP_ptr, A2),
		AROS_LCA(APTR, argList, A3),
		APTR, IntuitionBase, 98, );
    return 1;
}

struct Window *W_BuildEasyRequestArgs(struct Window *RefWindow, struct EasyStruct *easyStruct, ULONG IDCMP, APTR Args, struct globaldata *g)
{
    if (ISKS20)
	return AROS_LVO_CALL4(struct Window*,
		AROS_LCA(struct Window*, RefWindow, A0),
		AROS_LCA(struct EasyStruct*, easyStruct, A1),
		AROS_LCA(ULONG, IDCMP, D0),
		AROS_LCA(APTR, Args, A3),
		APTR, IntuitionBase, 99, );
    return NULL;
}    

LONG W_SysReqHandler(struct Window *window, ULONG *IDCMPFlagsPtr, BOOL WaitInput, struct globaldata *g)
{
    if (ISKS20)
	return AROS_LVO_CALL3(LONG,
		AROS_LCA(struct Window*, window, A0),
		AROS_LCA(ULONG*, IDCMPFlagsPtr, A1),
		AROS_LCA(BOOL, WaitInput, D0),
		APTR, IntuitionBase, 100, );
    return 0;
}

#undef SysBase
#undef DOSBase

void FixStartupPacket(struct DosPacket *pkt, struct globaldata *g)
{
	if (pkt->dp_Arg3)
		return;
	/* Fix 1.3 bad boot time ACTION_STARTUP: dp_Arg3 = NULL, dp_Arg1 = uninitialized data */
	struct DosList *dl = getdoslist(g);
	while(dl->dol_Next)
	{
		dl = BADDR(dl->dol_Next);
		if (dl->dol_Type == DLT_DEVICE && dl->dol_misc.dol_handler.dol_Startup == pkt->dp_Arg2) {
			/* This is safe, Startup Packet is allocated from BCPL stack
			 * that goes away after DOS gets message reply.
			 */
			pkt->dp_Arg1 = dl->dol_Name;
			pkt->dp_Arg3 = MKBADDR(dl);
			break;
		}
	}
	freedoslist(g);
}

LONG wrapper_stackswap(LONG (*func)(struct ExecBase *), struct ExecBase *SysBase)
{
	struct DosLibrary *DOSBase;
    ULONG stacksize;
    APTR stackptr;
    struct Task *tc;
    struct StackSwapStruct *stack;
    LONG ret;

    tc = FindTask(NULL);
    stacksize = (UBYTE *)tc->tc_SPUpper - (UBYTE *)tc->tc_SPLower;
    if (stacksize >= MIN_STACKSIZE) {
        ret = func(SysBase);
        return ret;
    }

    stack = AllocMem(sizeof(struct StackSwapStruct) + MIN_STACKSIZE, MEMF_CLEAR | MEMF_PUBLIC);
    if (!stack) {
        Alert(AT_DeadEnd | AG_NoMemory);
        return RETURN_FAIL;
    }
    stackptr = stack + 1;
    stack->stk_Lower = stackptr;
    stack->stk_Upper = (APTR)((IPTR)stack->stk_Lower + MIN_STACKSIZE);
    stack->stk_Pointer = (APTR)stack->stk_Upper;

	DOSBase = (struct DosLibrary *)OpenLibrary ("dos.library", MIN_LIB_VERSION);
    if (ISKS20) {
        AROS_LVO_CALL1(void,
            AROS_LCA(struct StackSwapStruct*, stack, A0),
            struct ExecBase*, SysBase, 122, );
        ret = func(SysBase);
        AROS_LVO_CALL1(void,
            AROS_LCA(struct StackSwapStruct*, stack, A0),
            struct ExecBase*, SysBase, 122, );
    } else {
        AROS_UFC2(void, StackSwap,
            AROS_UFCA(struct StackSwapStruct*, stack, A0),
            AROS_UFCA(struct ExecBase*, SysBase, A6));
        ret = func(SysBase);
        AROS_UFC2(void, StackSwap,
            AROS_UFCA(struct StackSwapStruct*, stack, A0),
            AROS_UFCA(struct ExecBase*, SysBase, A6));
    }
    FreeMem(stack, sizeof(struct StackSwapStruct) + MIN_STACKSIZE);
    CloseLibrary((struct Library*)DOSBase);

    return ret;
}

#endif

size_t strcspn (const char * str, const char * reject)
{
    size_t n = 0; /* Must set this to zero */

    while (*str && !strchr (reject, *str))
    {
	str ++;
	n ++;
    }

    return n;
}

