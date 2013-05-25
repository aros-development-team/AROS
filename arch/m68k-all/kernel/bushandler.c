
#include <aros/debug.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <libraries/debug.h>
#include <proto/exec.h>
#include <proto/debug.h>

void DebugPutStr(register const char *buff);
void DebugPutHex(const char *what, ULONG val);
void DebugPutDec(const char *what, ULONG val);
void DebugPutHexVal(ULONG val);

struct busframe
{
	APTR mmuframe;
	ULONG type;
	ULONG *usp;
	ULONG *vbr;
	ULONG regs[16];
};

static const UBYTE *sizes[] = { "LONG", "BYTE", "WORD", "?" };

static void dumpline(const UBYTE *label, ULONG *p)
{
	int i;
	DebugPutStr(label);
	for (i = 0; i < 8; i++) {
		DebugPutHexVal(p[i]);
		DebugPutStr(" ");
	}
	DebugPutStr("\n");
}

static void dumpstr(const UBYTE *label, const UBYTE *str)
{
	DebugPutStr(label);
	if (str) {
		DebugPutStr("\"");
		DebugPutStr(str);
		DebugPutStr("\"");
	} else {
		DebugPutStr("<NULL>");
	}
}
static void dumpbstr(const UBYTE *label, const UBYTE *str)
{
	DebugPutStr(label);
	if (str) {
		UBYTE tmp[258];
		UBYTE i;
		tmp[0] = '\"';
		for (i = 0; i < str[0]; i++)
			tmp[i + 1] = str[i + 1];
		tmp[i + 1] = '\"';
		tmp[i + 2] = 0;
		DebugPutStr(tmp);
	} else {
		DebugPutStr("<BNULL>");
	}
}
static void dumpinfo(struct ExecBase *SysBase, ULONG pc)
{
	struct DebugBase *DebugBase;
	struct Task *t;
	ULONG segNum;
	BPTR segPtr;

    t = SysBase->ThisTask;
    if (t) {
        dumpstr("Name: ", t->tc_Node.ln_Name);
        if (t->tc_Node.ln_Type == NT_PROCESS) {
            struct Process *p = (struct Process*)t;
            if (p->pr_CLI) {
                struct CommandLineInterface *cli = (struct CommandLineInterface*)BADDR(p->pr_CLI);
                DebugPutStr(" ");
                dumpbstr("CLI: ", BADDR(cli->cli_CommandName));
            }
        }
    }
    /* Better way to find DebugBase in Supervisor mode? */
    DebugBase = (struct DebugBase*)FindName(&SysBase->LibList, "debug.library");
    if (DebugBase) {
        if (DecodeLocation(pc, DL_SegmentNumber, &segNum,  DL_SegmentPointer, &segPtr, TAG_DONE)) {
            DebugPutStr(" Hunk ");
            DebugPutHexVal(segNum);
            DebugPutStr("Offset ");
            DebugPutHexVal(pc - (ULONG)BADDR(segPtr));
        }
    }
    DebugPutStr("\n");
}

/* WARNING: Running in bus error exception.
 * Can't call most system routines!
 */
void bushandler(struct busframe *bf)
{
	struct ExecBase *SysBase = (struct ExecBase*)(bf->vbr[1]);
	UBYTE *mf = bf->mmuframe;
	ULONG fa;
	ULONG sw;
	ULONG data;
	ULONG pc;
	UWORD size = 0;
	UWORD fc;
	BOOL write = FALSE;
	BOOL inst = FALSE;
	BOOL hasdata = FALSE;
	
	DebugPutStr("Bus Error!\n");

	if (bf->type == 0) {
		// 68030
		fa = ((ULONG*)(mf + 16))[0];
		sw = ((UWORD*)(mf + 10))[0];
		size = (sw >> 4) & 3;
		write = (sw & 0x40) == 0;
		fc = sw & 7;
		if (write) {
			data = ((ULONG*)(mf + 24))[0];
			hasdata = TRUE;
		}
	} else if (bf->type == 1) {
		// 68040
		fa = ((ULONG*)(mf + 20))[0];
		sw = ((UWORD*)(mf + 12))[0];
		size = (sw >> 5) & 3;
		write = (sw & 0x100) == 0;
		fc = sw & 7;
	} else {
		// 68060
		fa = ((ULONG*)(mf + 8))[0];
		sw = ((ULONG*)(mf + 12))[0];
		size = (sw >> 21) & 3;
		write = (sw & 0x800000) != 0;
		fc = (sw >> 16) & 7;
	}
	pc = ((ULONG*)(mf + 2))[0];
	inst = (fc & 3) == 2;

	DebugPutStr(sizes[size]);
	DebugPutStr("-");
	DebugPutStr(write ? "WRITE to   " : "READ from  ");
	DebugPutHexVal(fa);
	if (inst)
		DebugPutStr("(INST)");
	else
		DebugPutStr("      ");
	DebugPutStr("     data: ");
	if (hasdata)
		DebugPutHexVal(data);
	else
		DebugPutStr("-------- ");
	DebugPutStr(" PC: ");
	DebugPutHexVal(pc);
	DebugPutStr("\n");

	DebugPutStr("USP:  ");
	DebugPutHexVal((ULONG)bf->usp);
	DebugPutStr(" SR: ");
	DebugPutHexVal(((UWORD*)mf)[0]);
	DebugPutStr("       SW: ");
	DebugPutHexVal(sw);
	DebugPutStr("       TCB: ");
	DebugPutHexVal((ULONG)SysBase->ThisTask);
	DebugPutStr("\n");

    dumpline("Data: ", &bf->regs[0]);
    dumpline("Addr: ", &bf->regs[8]);

    dumpline("Stck: ", bf->usp);
    dumpline("Stck: ", bf->usp + 8);

    dumpline("PC-8: ", (ULONG*)(pc - 8 * 4));
    dumpline("PC *: ", (ULONG*)pc);

	dumpinfo(SysBase, pc);

	DebugPutStr("\n");
}
