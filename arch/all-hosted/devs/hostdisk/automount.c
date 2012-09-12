#include <aros/debug.h>
#include <dos/filehandler.h>
#include <exec/rawfmt.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include LC_LIBDEFS_FILE

extern const char GM_UNIQUENAME(LibName)[];

static int Automount(struct HostDiskBase *hdskBase)
{
    struct ExpansionBase *ExpansionBase = NULL;
    char *unit;
    struct DriveGeometry dg;
    ULONG u, len;

    if (hdskBase->segList)
    {
        D(bug("hostdisk: Loaded from disk, skipping automount.\n"));
        return TRUE;
    }

    len = strlen(hdskBase->DiskDevice) + 5;
    unit = AllocMem(len, MEMF_ANY);
    if (!unit)
        return FALSE;

    for (u = 0; u < 30; u++)
    {
        IPTR pp[24];

        NewRawDoFmt(hdskBase->DiskDevice, (VOID_FUNC)RAWFMTFUNC_STRING, unit, u + hdskBase->unitBase);

        if (Host_ProbeGeometry(hdskBase, unit, &dg) == 0)
        {
            struct DeviceNode *devnode;
            TEXT dosdevname[4] = "HD0";

            if (!ExpansionBase)
            {
                ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 40);
                if (!ExpansionBase)
                {
                    FreeMem(unit, len);
                    return FALSE;
                }
            }

            if (u < 10)
                dosdevname[2] += u % 10;
            else
                dosdevname[2] = 'A' - 10 + u;

            D(bug("hostdisk: Automounting %s as %s\n", unit, dosdevname));

            pp[0]               = (IPTR)dosdevname;
            pp[1]               = (IPTR)GM_UNIQUENAME(LibName);
            pp[2]               = u;
            pp[DE_TABLESIZE    + 4] = DE_BOOTBLOCKS;
            pp[DE_SIZEBLOCK    + 4] = dg.dg_SectorSize >> 2;
            pp[DE_NUMHEADS     + 4] = dg.dg_Heads;
            pp[DE_SECSPERBLOCK + 4] = 1;
            pp[DE_BLKSPERTRACK + 4] = dg.dg_TrackSectors;
            pp[DE_RESERVEDBLKS + 4] = 2;
            pp[DE_LOWCYL       + 4] = 0;
            pp[DE_HIGHCYL      + 4] = dg.dg_Cylinders - 1;
            pp[DE_NUMBUFFERS   + 4] = 10;
            pp[DE_BUFMEMTYPE   + 4] = MEMF_PUBLIC;
            pp[DE_MAXTRANSFER  + 4] = 0x00200000;
            pp[DE_MASK         + 4] = -2; /* On Windows Host_Read() fails with ERROR_INVALID_PARAMETER on odd addresses */
            pp[DE_BOOTPRI      + 4] = 0;
            pp[DE_DOSTYPE      + 4] = AROS_MAKE_ID('D','O','S','\001');
            pp[DE_CONTROL      + 4] = 0;
            pp[DE_BOOTBLOCKS   + 4] = 2;

            devnode = MakeDosNode(pp);
            if (!devnode)
            {
                CloseLibrary(&ExpansionBase->LibNode);
                FreeMem(unit, len);
                return FALSE;
            }

            AddBootNode(pp[DE_BOOTPRI + 4], ADNF_STARTPROC, devnode, NULL);
        }
    }

    if (ExpansionBase)
        CloseLibrary(&ExpansionBase->LibNode);
        
    FreeMem(unit, len);
    return TRUE;
}

ADD2INITLIB(Automount, 20);
