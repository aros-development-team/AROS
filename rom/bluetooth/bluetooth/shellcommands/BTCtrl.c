/*
** BTCtrl -- shell control of bluetooth.library: discovery, names, scan
** modes, device registration/pairing/connection, saving the config.
*/

#include <aros/isoascii.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/exec.h>
#include <dos/dosextens.h>
#include <libraries/bluetooth.h>
#include <proto/bluetooth.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARGS_HARDWARE     0
#define ARGS_UNIT         1
#define ARGS_SCAN         2
#define ARGS_STOP         3
#define ARGS_DURATION     4
#define ARGS_NONAMES      5
#define ARGS_WAIT         6
#define ARGS_NAME         7
#define ARGS_DISCOVERABLE 8
#define ARGS_CONNECTABLE  9
#define ARGS_DEVICE       10
#define ARGS_REGISTER     11
#define ARGS_UNREGISTER   12
#define ARGS_PAIR         13
#define ARGS_UNPAIR       14
#define ARGS_CONNECT      15
#define ARGS_DISCONNECT   16
#define ARGS_FORGET       17
#define ARGS_REMOTENAME   18
#define ARGS_SERVICES     19
#define ARGS_SAVE         20
#define ARGS_SIZEOF       21

static const char *template = "HARDWARE/K,UNIT/N,SCAN/S,STOP/S,DURATION/N,NONAMES/S,WAIT/S,"
                              "NAME/K,DISCOVERABLE/K,CONNECTABLE/K,"
                              "DEVICE/K,REGISTER/S,UNREGISTER/S,PAIR/S,UNPAIR/S,CONNECT/S,DISCONNECT/S,FORGET/S,REMOTENAME/S,SERVICES/S,"
                              "SAVE/S";
const char *version = "$VER: BTCtrl 1.0 (18.08.2026) " ISOASCII_COPYRIGHT " The AROS Development Team";
static IPTR ArgsArray[ARGS_SIZEOF];
static struct RDArgs *ArgsHook = NULL;

struct Library *BluetoothBase = NULL;

void fail(char *str)
{
    if(ArgsHook)
    {
        FreeArgs(ArgsHook);
        ArgsHook = NULL;
    }
    if(BluetoothBase)
    {
        CloseLibrary(BluetoothBase);
    }
    if(str)
    {
        PutStr(str);
        exit(20);
    }
    exit(0);
}

static BOOL BoolArg(STRPTR str)
{
    if(!str)
    {
        return(FALSE);
    }
    return((stricmp(str, "yes") == 0) || (stricmp(str, "on") == 0) || (stricmp(str, "true") == 0) || (*str == '1'));
}

static struct Node *FindHardware(void)
{
    struct List *bthlist;
    struct Node *bth;
    STRPTR devname;
    IPTR unit;

    btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &bthlist, TAG_END);
    bth = bthlist->lh_Head;
    while(bth->ln_Succ)
    {
        btGetAttrs(BGA_HARDWARE, bth, BHA_DeviceName, &devname, BHA_DeviceUnit, &unit, TAG_END);
        if(!ArgsArray[ARGS_HARDWARE] ||
           ((!stricmp(FilePart(devname), FilePart((STRPTR) ArgsArray[ARGS_HARDWARE]))) &&
            (!ArgsArray[ARGS_UNIT] || (unit == *((ULONG *) ArgsArray[ARGS_UNIT])))))
        {
            return(bth);
        }
        bth = bth->ln_Succ;
    }
    return(NULL);
}

static struct Node *FindDevice(struct Node *bth)
{
    struct List *devlist;
    struct Node *bd;
    STRPTR addr;
    STRPTR name;
    STRPTR want = (STRPTR) ArgsArray[ARGS_DEVICE];

    btGetAttrs(BGA_HARDWARE, bth, BHA_DeviceList, &devlist, TAG_END);
    bd = devlist->lh_Head;
    while(bd->ln_Succ)
    {
        btGetAttrs(BGA_DEVICE, bd, BDA_AddressString, &addr, BDA_Name, &name, TAG_END);
        if(!stricmp(addr, want) || (name && !stricmp(name, want)))
        {
            return(bd);
        }
        bd = bd->ln_Succ;
    }
    return(NULL);
}

static void WaitForDiscovery(struct Node *bth)
{
    struct MsgPort *mp;
    APTR eh;
    struct Message *msg;
    IPTR discovering = TRUE;

    if(!(mp = CreateMsgPort()))
    {
        return;
    }
    eh = btAddEventHandler(mp, BEHMF_DISCOVERYSTOP|BEHMF_ADDDEVICE|BEHMF_DEVICEUPDATE);
    if(eh)
    {
        while(discovering)
        {
            ULONG sigs = Wait((1UL<<mp->mp_SigBit)|SIGBREAKF_CTRL_C);
            if(sigs & SIGBREAKF_CTRL_C)
            {
                btStopDiscovery(bth);
            }
            while((msg = GetMsg(mp)))
            {
                IPTR ev = 0;
                APTR p1 = NULL;
                btGetAttrs(BGA_EVENTNOTE, msg, BENA_EventID, &ev, BENA_Param1, &p1, TAG_END);
                if((ev == BEHMB_ADDDEVICE) || (ev == BEHMB_DEVICEUPDATE))
                {
                    STRPTR name = NULL, addr = NULL;
                    LONG rssi = 127;
                    btLockReadBase();
                    btGetAttrs(BGA_DEVICE, p1, BDA_Name, &name, BDA_AddressString, &addr, BDA_RSSI, &rssi, TAG_END);
                    Printf("  %s %s %s (%ld dBm)\n", (ev == BEHMB_ADDDEVICE) ? (STRPTR) "found  " : (STRPTR) "update ",
                           addr, name ? name : (STRPTR) "", rssi);
                    btUnlockBase();
                }
                ReplyMsg(msg);
            }
            btGetAttrs(BGA_HARDWARE, bth, BHA_IsDiscovering, &discovering, TAG_END);
        }
        btRemEventHandler(eh);
    }
    DeleteMsgPort(mp);
}

int main(int argc, char *argv[])
{
    struct Node *bth;
    struct Node *bd = NULL;
    LONG rc = RETURN_OK;

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        fail("Wrong arguments!\n");
    }
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        fail("Unable to open bluetooth.library\n");
    }

    btLockReadBase();
    bth = FindHardware();
    btUnlockBase();
    if(!bth)
    {
        if(ArgsArray[ARGS_SAVE])
        {
            if(btSaveCfgToDisk(NULL, FALSE))
            {
                PutStr("Configuration saved.\n");
                fail(NULL);
            }
            fail("Saving the configuration failed!\n");
        }
        fail("No matching Bluetooth hardware found.\n");
    }

    if(ArgsArray[ARGS_NAME])
    {
        btSetAttrs(BGA_HARDWARE, bth, BHA_LocalName, ArgsArray[ARGS_NAME], TAG_END);
        Printf("Local name set to '%s'.\n", (STRPTR) ArgsArray[ARGS_NAME]);
    }
    if(ArgsArray[ARGS_DISCOVERABLE] || ArgsArray[ARGS_CONNECTABLE])
    {
        struct TagItem tags[3];
        UWORD n = 0;
        if(ArgsArray[ARGS_DISCOVERABLE])
        {
            tags[n].ti_Tag = BHA_Discoverable;
            tags[n++].ti_Data = BoolArg((STRPTR) ArgsArray[ARGS_DISCOVERABLE]);
        }
        if(ArgsArray[ARGS_CONNECTABLE])
        {
            tags[n].ti_Tag = BHA_Connectable;
            tags[n++].ti_Data = BoolArg((STRPTR) ArgsArray[ARGS_CONNECTABLE]);
        }
        tags[n].ti_Tag = TAG_END;
        btSetAttrsA(BGA_HARDWARE, bth, tags);
        PutStr("Scan mode updated.\n");
    }
    if(ArgsArray[ARGS_STOP])
    {
        btStopDiscovery(bth);
        PutStr("Discovery stopped.\n");
    }
    if(ArgsArray[ARGS_SCAN])
    {
        ULONG dur = ArgsArray[ARGS_DURATION] ? *((ULONG *) ArgsArray[ARGS_DURATION]) : 0;
        if(btStartDiscovery(bth,
                            dur ? BDSA_Duration : TAG_IGNORE, dur,
                            BDSA_ResolveNames, ArgsArray[ARGS_NONAMES] ? FALSE : TRUE,
                            TAG_END))
        {
            PutStr("Discovery started.\n");
            if(ArgsArray[ARGS_WAIT])
            {
                WaitForDiscovery(bth);
                PutStr("Discovery finished.\n");
            }
        } else {
            PutStr("Could not start discovery.\n");
            rc = RETURN_ERROR;
        }
    }
    if(ArgsArray[ARGS_DEVICE])
    {
        btLockReadBase();
        bd = FindDevice(bth);
        btUnlockBase();
        if(!bd)
        {
            fail("Device not found (use the address or the name).\n");
        }
        if(ArgsArray[ARGS_REMOTENAME])
        {
            struct MsgPort *mp = CreateMsgPort();
            if(mp)
            {
                APTR pp = btAllocChannel(bd, mp, NULL);
                if(pp)
                {
                    UBYTE buf[256];
                    LONG err;
                    btChannelSetup(pp, BTPR_REMOTENAME, 0, 0);
                    err = btDoChannel(pp, buf, sizeof(buf));
                    if(err)
                    {
                        Printf("Remote name request failed: %s (%ld)\n", btNumToStr(BNTS_IOERR, err, "unknown"), err);
                        rc = RETURN_WARN;
                    } else {
                        Printf("Remote name: '%s'\n", buf);
                    }
                    btFreeChannel(pp);
                }
                DeleteMsgPort(mp);
            }
        }
        if(ArgsArray[ARGS_REGISTER])
        {
            if(btRegisterDevice(bd))
            {
                PutStr("Device registered.\n");
            } else {
                PutStr("Registering the device failed.\n");
                rc = RETURN_ERROR;
            }
        }
        if(ArgsArray[ARGS_UNREGISTER])
        {
            if(btUnregisterDevice(bd))
            {
                PutStr("Device unregistered.\n");
            } else {
                PutStr("Unregistering the device failed.\n");
                rc = RETURN_ERROR;
            }
        }
        if(ArgsArray[ARGS_PAIR])
        {
            if(btPairDevice(bd, TAG_END))
            {
                PutStr("Pairing started.\n");
            } else {
                PutStr("Pairing failed to start.\n");
                rc = RETURN_ERROR;
            }
        }
        if(ArgsArray[ARGS_UNPAIR])
        {
            if(btUnpairDevice(bd))
            {
                PutStr("Device unpaired.\n");
            } else {
                PutStr("Unpairing failed.\n");
                rc = RETURN_ERROR;
            }
        }
        if(ArgsArray[ARGS_CONNECT])
        {
            if(btConnectDevice(bd))
            {
                PutStr("Connected.\n");
            } else {
                PutStr("Connection failed.\n");
                rc = RETURN_ERROR;
            }
        }
        if(ArgsArray[ARGS_SERVICES])
        {
            if(btEnumerateServices(bd))
            {
                PutStr("Services enumerated.\n");
            } else {
                PutStr("Service enumeration failed.\n");
                rc = RETURN_ERROR;
            }
        }
        if(ArgsArray[ARGS_DISCONNECT])
        {
            if(btDisconnectDevice(bd))
            {
                PutStr("Disconnected.\n");
            } else {
                PutStr("Disconnect failed.\n");
                rc = RETURN_ERROR;
            }
        }
        if(ArgsArray[ARGS_FORGET])
        {
            btUnregisterDevice(bd);
            btFreeDevice(bd);
            PutStr("Device forgotten.\n");
        }
    }
    if(ArgsArray[ARGS_SAVE])
    {
        if(btSaveCfgToDisk(NULL, FALSE))
        {
            PutStr("Configuration saved.\n");
        } else {
            PutStr("Saving the configuration failed!\n");
            rc = RETURN_ERROR;
        }
    }
    FreeArgs(ArgsHook);
    ArgsHook = NULL;
    CloseLibrary(BluetoothBase);
    BluetoothBase = NULL;
    return(rc);
}
