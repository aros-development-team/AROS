/* $Id$

	Desc: Boot AROS native
*/

#define DEBUG 1

#include <devices/trackdisk.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <libraries/expansionbase.h>

#include <proto/exec.h>

#ifdef DEBUG
#include <aros/debug.h>
#endif
#include <aros/macros.h>

void InitKeyboard(void);

int boot_entry()
{
	return -1;
}

static const char boot_end;
int AROS_SLIB_ENTRY(init,boot)();

const struct Resident boot_resident =
{
	RTC_MATCHWORD,
	(struct Resident *)&boot_resident,
	(APTR)&boot_end,
	RTF_COLDSTART,
	41,
	NT_PROCESS,
	-50,
	"Boot Strap",
	"AROS Boot Strap 41.0\r\n",
	(APTR)&boot_init
};

AROS_LH2(int, init,
    AROS_LHA(ULONG, dummy, D0),
    AROS_LHA(ULONG, seglist, A0),
    struct ExecBase *, SysBase, 0, boot)
{
	AROS_LIBFUNC_INIT
	struct ExpansionBase *ExpansionBase;
	struct Resident *dosresident;
	struct BootNode *bn;
	char key;
	static char transl[] =
    { ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', ' ', ' ', ' ',
      ' ', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', ' ', ' ', 10,
      ' ', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ' ', ' ', ' ', ' ', 
      ' ', 'Z', 'X', 'C', 'V', 'B', 'N', 'M' };


	InitKeyboard();
	ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
	if (ExpansionBase == NULL)
		Alert(AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_ExpansionLib);

	for (bn=(struct BootNode *)ExpansionBase->MountList.lh_Head;;bn=(struct BootNode *)ExpansionBase->MountList.lh_Head)
	{
		while (bn->bn_Node.ln_Succ)
		{
			if (((struct DosList *)bn->bn_DeviceNode)->dol_Device==0)
			{
				if (checkBoot((struct DeviceNode *)bn->bn_DeviceNode))
				{
					if (bn!=(struct BootNode *)ExpansionBase->MountList.lh_Head)
					{
						Remove(&bn->bn_Node);
						AddHead(&ExpansionBase->MountList, &bn->bn_Node);
					}
					CloseLibrary((struct Library *)ExpansionBase);
					dosresident=FindResident("dos.library");
					if (dosresident)
					{
						InitResident(dosresident, NULL);
						goto start_boot;
					}
					else
						Alert(AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_DOSLib);
				}
			}
			bn = (struct BootNode *)bn->bn_Node.ln_Succ;
		}
		kprintf
			(
					"No bootable disk found.\n"
					"Insert bootable disk in any drive and press Enter.\n"
			);
		do
		{
			key = GetK();
			key = transl[(key == 0x39) ? 1 : key - 1];
			UnGetK();
		} while(key !=10);
	}

start_boot:
	return 0;
	AROS_LIBFUNC_EXIT
}

int checkBoot(struct DeviceNode *dn) {
struct FileSysStartupMsg *fssm= BADDR(dn->dn_Startup);
struct IOExtTD *iotd;
ULONG buf[128];
int retval=0;
struct MsgPort *mp=CreateMsgPort();

	if (mp)
	{
		iotd=(struct IOExtTD *)CreateIORequest(mp,sizeof(struct IOExtTD));
		if (iotd)
		{
			if (!OpenDevice(AROS_BSTR_ADDR(fssm->fssm_Device),fssm->fssm_Unit, (struct IORequest *)&iotd->iotd_Req,0))
			{
				iotd->iotd_Req.io_Command = CMD_READ;
				iotd->iotd_Req.io_Offset = 0;
				iotd->iotd_Req.io_Length = 512;
				iotd->iotd_Req.io_Data = buf;
				if (!DoIO((struct IORequest *)&iotd->iotd_Req))
				{
#warning TODO: check bootblock for bootable code
					if ((AROS_BE2LONG(buf[0]) & 0xFFFFFF00)==0x444F5300)
						retval=1;
				}
				CloseDevice((struct IORequest *)&iotd->iotd_Req);
			}
			DeleteIORequest((struct IORequest *)&iotd->iotd_Req);
		}
		else
		{
			Alert(AT_DeadEnd | AG_NoMemory | AN_BootStrap);
		}
		DeleteMsgPort(mp);
	}
	else
	{
		Alert(AT_DeadEnd | AG_NoMemory | AN_BootStrap);
	}
	return retval;
}
static const char boot_end = 0;

