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
#include <devices/newstyle.h>

#include <proto/exec.h>

#ifdef DEBUG
#include <aros/debug.h>
#endif
#include <aros/macros.h>

void InitKeyboard(void);
void putc(char);
void putstring(char *);

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
		putstring
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

void putstring(char *string) {

	while (*string)
		Putc(*string++);
}


int checkBoot(struct DeviceNode *dn) {
struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);
struct DosEnvec *de = BADDR(fssm->fssm_Environ);
struct IOExtTD *iotd;
ULONG buf[128];
int retval=0;
struct MsgPort *mp=CreateMsgPort();

	if (mp)
	{
		iotd=(struct IOExtTD *)CreateIORequest(mp,sizeof(struct IOExtTD));
		if (iotd)
		{
			kprintf("Trying to boot from %S\n", AROS_BSTR_ADDR(fssm->fssm_Device));
			if (!OpenDevice(AROS_BSTR_ADDR(fssm->fssm_Device),fssm->fssm_Unit, (struct IORequest *)&iotd->iotd_Req,0))
			{
				if (strncmp(AROS_BSTR_ADDR(fssm->fssm_Device), "ide.device", 10) == 0)
				{
				    UQUAD offset64 = de->de_LowCyl;

				    offset64 *= de->de_Surfaces;
				    offset64 *= de->de_BlocksPerTrack;
				    offset64 *= (de->de_SizeBlock*4);

				    iotd->iotd_Req.io_Command = NSCMD_TD_READ64;
				    iotd->iotd_Req.io_Offset  = offset64 &  0xFFFFFFFF;
				    iotd->iotd_Req.io_Actual  = offset64 >> 32;
				    kprintf("Reading at offset 0x%08x%08x\n",
				        iotd->iotd_Req.io_Actual, iotd->iotd_Req.io_Offset);

				}
				else
				{
				    iotd->iotd_Req.io_Command = CMD_READ;
			  	    iotd->iotd_Req.io_Offset =
				        de->de_LowCyl*de->de_Surfaces*
					de->de_BlocksPerTrack*(de->de_SizeBlock*4);

				    kprintf("Reading at offset 0x%08x\n",
				    iotd->iotd_Req.io_Offset);
				}

				iotd->iotd_Req.io_Length = 512;
				iotd->iotd_Req.io_Data = buf;

				if (!DoIO((struct IORequest *)&iotd->iotd_Req))
				{
					if ((AROS_BE2LONG(buf[0]) & 0xFFFFFF00)==0x444F5300)
						retval=1;
					else
					{
						iotd->iotd_Req.io_Offset =
							((de->de_LowCyl*de->de_Surfaces*
							de->de_BlocksPerTrack)+1)*(de->de_SizeBlock*4);
						if (!DoIO((struct IORequest *)&iotd->iotd_Req))
						{
							if ((AROS_BE2LONG(buf[0]) & 0xFFFFFF00)==0x444F5300)
								retval=1;
						}
					}
				}
				if (strcmp(AROS_BSTR_ADDR(fssm->fssm_Device),"trackdisk.device")==0)
				{
					iotd->iotd_Req.io_Command = TD_MOTOR;
					iotd->iotd_Req.io_Length = 0;
					DoIO((struct IORequest *)&iotd->iotd_Req);
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

