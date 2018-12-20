/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#if AROS_SERIAL_DEBUG
static const UBYTE *const sizes[] = { "LONG", "BYTE", "WORD", "?" };

extern BOOL mmu_valid_check_030(APTR);
extern BOOL mmu_valid_check_040(APTR);
extern BOOL mmu_valid_check_060(APTR);

static BOOL mmuisvalid(UWORD mmutype, APTR p)
{
   UWORD valid = 0;
   switch(mmutype)
   {
       case 0:
       valid = mmu_valid_check_030(p);
       break;
       case 1:
       valid = mmu_valid_check_040(p);
       break;
       case 2:
       valid = mmu_valid_check_060(p);
       break;
    }
    return valid == 0;
}

static void dumpline(UWORD mmutype, const UBYTE *label, ULONG *p)
{
    WORD i;
    DebugPutStr(label);
    for (i = 0; i < 8; i++) {
        if (mmuisvalid(mmutype, &p[i]) && mmuisvalid(mmutype, ((UBYTE*)(&p[i])) + 3))
            DebugPutHexVal(p[i]);
        else
            DebugPutStr("???????? ");
    }
    DebugPutStr("\n");
}

static void dumpstr(UWORD mmutype, const UBYTE *label, const UBYTE *str)
{
    DebugPutStr(label);
    if (str) {
        if (mmuisvalid(mmutype, (APTR)str)) {
            DebugPutStr("\"");
            DebugPutStr(str);
            DebugPutStr("\"");
        } else {
            DebugPutStr("<INVALID>");
        }
    } else {
        DebugPutStr("<NULL>");
    }
}
static void dumpbstr(UWORD mmutype, const UBYTE *label, const UBYTE *str)
{
    DebugPutStr(label);
    if (str) {
        if (mmuisvalid(mmutype, (APTR)str)) {
            UBYTE tmp[258];
            UBYTE i;
            tmp[0] = '\"';
            for (i = 0; i < str[0]; i++) {
                tmp[i + 1] = '?';
                if (mmuisvalid(mmutype, (APTR)&str[i + 1]))
                    tmp[i + 1] = str[i + 1];
            }
            tmp[i + 1] = '\"';
            tmp[i + 2] = 0;
            DebugPutStr(tmp);
        } else {
            DebugPutStr("<INVALID>");
        }    
    } else {
        DebugPutStr("<BNULL>");
    }
}
static void dumpinfo(UWORD mmutype, struct ExecBase *SysBase, ULONG pc)
{
    struct DebugBase *DebugBase;
    struct Task *t;

    if (SysBase == NULL)
        return;
    t = SysBase->ThisTask;
    if (t && mmuisvalid(mmutype, t) && (t->tc_Node.ln_Type == NT_TASK || t->tc_Node.ln_Type == NT_PROCESS)) {
        dumpstr(mmutype, "Name: ", t->tc_Node.ln_Name);
        if (t->tc_Node.ln_Type == NT_PROCESS) {
            struct Process *p = (struct Process*)t;
            if (p->pr_CLI && mmuisvalid(mmutype, BADDR(p->pr_CLI))) {
                struct CommandLineInterface *cli = (struct CommandLineInterface*)BADDR(p->pr_CLI);
                dumpbstr(mmutype, " CLI: ", BADDR(cli->cli_CommandName));
            }
        }
        DebugPutStr("\n");
    }
    /* Better way to find DebugBase in Supervisor mode? */
    DebugBase = (struct DebugBase*)FindName(&SysBase->LibList, "debug.library");
    if (DebugBase) {
        ULONG segNum;
        BPTR segPtr;
        char *modName;
        if (DecodeLocation(pc, DL_SegmentNumber, &segNum,  DL_SegmentPointer, &segPtr, DL_ModuleName, &modName, TAG_DONE)) {
            dumpstr(mmutype, "Module ", modName);
            DebugPutStr(" Hunk ");
            DebugPutHexVal(segNum);
            DebugPutStr("Offset ");
            DebugPutHexVal(pc - (ULONG)BADDR(segPtr));
            DebugPutStr("Start ");
            DebugPutHexVal((ULONG)BADDR(segPtr));
        }
    }
}
#endif

/* WARNING: Running in bus/address error exception.
 * Can't call most system routines!
 */
void bushandler(struct busframe *bf)
{
#if AROS_SERIAL_DEBUG
	struct ExecBase *SysBase = (struct ExecBase*)(bf->vbr[1]);
	UBYTE *mf = bf->mmuframe;
	ULONG fa = 0;
	ULONG sw = 0;
	ULONG data;
	ULONG pc;
	UWORD size = 0;
	UWORD fc = 0;
	UWORD sr;
	BOOL write = FALSE;
	BOOL inst = FALSE;
	BOOL hasdata = FALSE;
	UWORD mmutype = bf->type;
	char buf[16];
	BOOL addrerror = (bf->type & 0x10) != 0;
	
	DebugPutStr(addrerror ? "Address Error!\n" : "Bus Error!\n");

	if (mmutype == 0 || mmutype == 0x10) {
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
	} else if (mmutype == 1) {
		// 68040
		fa = ((ULONG*)(mf + 20))[0];
		sw = ((UWORD*)(mf + 12))[0];
		size = (sw >> 5) & 3;
		write = (sw & 0x100) == 0;
		fc = sw & 7;
		if (write && (mf[15] & 0x80)) {
			data = ((ULONG*)(mf + 28))[0];
			hasdata = TRUE;
		}
	} else if (mmutype == 2) {
		// 68060
		fa = ((ULONG*)(mf + 8))[0];
		sw = ((ULONG*)(mf + 12))[0];
		size = (sw >> 21) & 3;
		write = (sw & 0x800000) != 0;
		fc = (sw >> 16) & 7;
	} else if (mmutype == 0x11 || mmutype == 0x12) {
		// 68040
		fa = ((ULONG*)(mf + 8))[0];
		sw = 0;
		size = 0;
		fc = (((UWORD*)mf)[0] & 0x2000) ? 6 : 2;
	}
	pc = ((ULONG*)(mf + 2))[0];
	inst = (fc & 3) == 2;
	sr = ((UWORD*)mf)[0];

	if (!mmuisvalid(mmutype, SysBase) || !mmuisvalid(mmutype, SysBase + 1)) {
		SysBase = NULL;
		DebugPutStr("INVALID SYSBASE!\n");
        }

	DebugPutStr(sizes[size]);
	DebugPutStr("-");
	DebugPutStr(write ? "WRITE to  " : "READ from ");
	DebugPutHexVal(fa);
	if (inst)
		DebugPutStr("(INST)");
	else
		DebugPutStr("      ");
	DebugPutStr("   data: ");
	if (hasdata)
		DebugPutHexVal(data);
	else
		DebugPutStr("-------- ");
	DebugPutStr("   PC: ");
	DebugPutHexVal(pc);
	DebugPutStr("\n");

	DebugPutStr("USP:  ");
	DebugPutHexVal((ULONG)bf->usp);
	DebugPutStr("SR: ");
	DebugPutHexVal(sr);
	DebugPutStr("     SW: ");
	DebugPutHexVal(sw);
	buf[0] = '(';
	buf[1] = (sr & 0x2000) ? 'S' : 'U';
	buf[2] = ((sr >> 8) & 7) + '0';
	buf[3] = ')';
	buf[4] = '(';
	buf[5] = SysBase ? (SysBase->TDNestCnt >= 0 ? 'F' : '-') : '?';
	buf[6] = ')';
	buf[7] = '(';
	buf[8] = SysBase ? (SysBase->IDNestCnt >= 0 ? 'D' : '-') : '?';
	buf[9] = ')';
	buf[10] = 0;
	DebugPutStr(buf);
	DebugPutStr("    TCB: ");
	DebugPutHexVal(SysBase ? 0xffffffff : (ULONG)SysBase->ThisTask);
	DebugPutStr("\n");

    dumpline(mmutype, "Data: ", &bf->regs[0]);
    dumpline(mmutype, "Addr: ", &bf->regs[8]);

    dumpline(mmutype, "Stck: ", bf->usp);
    dumpline(mmutype, "Stck: ", bf->usp + 8);

    dumpline(mmutype, "PC-8: ", (ULONG*)(pc - 8 * 4));
    dumpline(mmutype, "PC *: ", (ULONG*)pc);

	dumpinfo(mmutype, SysBase, pc);

	DebugPutStr("\n");
#endif 
}
