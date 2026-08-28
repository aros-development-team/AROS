/*
** BTDevLister -- lists the Bluetooth hardware, devices and classes known to
** bluetooth.library (the counterpart of PsdDevLister).
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

#define ARGS_HARDWARE  0
#define ARGS_DEVICES   1
#define ARGS_CLASSES   2
#define ARGS_SERVICES  3
#define ARGS_ALL       4
#define ARGS_SIZEOF    5

static const char *template = "HARDWARE/S,DEVICES/S,CLASSES/S,SERVICES/S,ALL/S";
const char *version = "$VER: BTDevLister 1.0 (18.08.2026) " ISOASCII_COPYRIGHT " The AROS Development Team";
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

static void ListServices(struct Node *bd)
{
    struct List *svclist;
    struct Node *bsv;
    struct List *eplist;
    struct Node *bep;

    btGetAttrs(BGA_DEVICE, bd, BDA_ServiceList, &svclist, TAG_END);
    bsv = svclist->lh_Head;
    while(bsv->ln_Succ)
    {
        STRPTR name = NULL;
        IPTR uuid16 = 0;
        IPTR proto = 0;
        IPTR psm = 0;
        IPTR chan = 0;
        IPTR starth = 0;
        IPTR endh = 0;
        STRPTR binding = NULL;
        struct Node *bcls = NULL;
        btGetAttrs(BGA_SERVICE, bsv,
                   BSVA_Name, &name,
                   BSVA_UUID16, &uuid16,
                   BSVA_Protocol, &proto,
                   BSVA_PSM, &psm,
                   BSVA_RFCOMMChannel, &chan,
                   BSVA_StartHandle, &starth,
                   BSVA_EndHandle, &endh,
                   BSVA_BindingClass, &bcls,
                   BSVA_EndpointList, &eplist,
                   TAG_END);
        if(bcls)
        {
            btGetAttrs(BGA_BTCLASS, bcls, BCA_ClassName, &binding, TAG_END);
        }
        Printf("    Service %s (0x%04lx %s)", name ? name : (STRPTR) "<unnamed>", uuid16,
               btNumToStr(BNTS_UUID16, uuid16, "unknown"));
        switch(proto)
        {
            case BSVP_L2CAP:
                Printf(", L2CAP PSM 0x%04lx", psm);
                break;
            case BSVP_RFCOMM:
                Printf(", RFCOMM channel %ld", chan);
                break;
            case BSVP_ATT:
                Printf(", GATT handles 0x%04lx-0x%04lx", starth, endh);
                break;
        }
        if(binding)
        {
            Printf(", bound to %s", binding);
        }
        PutStr("\n");
        bep = eplist->lh_Head;
        while(bep->ln_Succ)
        {
            IPTR canread = 0, canwrite = 0, type = 0, eppsm = 0, cid = 0, epchan = 0, handle = 0, epuuid = 0, props = 0, mtu = 0;
            STRPTR epname = NULL;
            btGetAttrs(BGA_ENDPOINT, bep,
                       BEA_CanRead, &canread, BEA_CanWrite, &canwrite, BEA_Type, &type, BEA_PSM, &eppsm, BEA_CID, &cid,
                       BEA_RFCOMMChannel, &epchan, BEA_Handle, &handle, BEA_UUID16, &epuuid,
                       BEA_Properties, &props, BEA_MaxPktSize, &mtu, BEA_Name, &epname,
                       TAG_END);
            Printf("      Endpoint [%s%s] %s", canread ? (STRPTR) "R" : (STRPTR) "-", canwrite ? (STRPTR) "W" : (STRPTR) "-",
                   epname ? epname : (STRPTR) "");
            switch(type)
            {
                case BEPT_L2CAP:
                    Printf("L2CAP PSM 0x%04lx", eppsm);
                    break;
                case BEPT_L2CAP_FIXED:
                    Printf("L2CAP CID 0x%04lx", cid);
                    break;
                case BEPT_RFCOMM:
                    Printf("RFCOMM channel %ld", epchan);
                    break;
                case BEPT_GATT_CHAR:
                    Printf("GATT char 0x%04lx (%s) handle 0x%04lx props 0x%02lx", epuuid,
                           btNumToStr(BNTS_UUID16, epuuid, "?"), handle, props);
                    break;
            }
            Printf(", MTU %ld\n", mtu);
            bep = bep->ln_Succ;
        }
        bsv = bsv->ln_Succ;
    }
}

static void ListDevices(struct Node *bth, BOOL services)
{
    struct List *devlist;
    struct Node *bd;

    btGetAttrs(BGA_HARDWARE, bth, BHA_DeviceList, &devlist, TAG_END);
    bd = devlist->lh_Head;
    if(!bd->ln_Succ)
    {
        PutStr("  No devices known.\n");
    }
    while(bd->ln_Succ)
    {
        STRPTR name = NULL;
        STRPTR addr = NULL;
        IPTR cod = 0;
        IPTR appearance = 0;
        LONG rssi = 127;
        IPTR isclassic = 0, isle = 0, isreg = 0, isbonded = 0, isconn = 0, isdisc = 0, isdead = 0;
        IPTR numsvc = 0;
        IPTR addrtype = 0;
        struct Node *bcls = NULL;
        STRPTR binding = NULL;
        STRPTR origname = NULL;

        btGetAttrs(BGA_DEVICE, bd,
                   BDA_Name, &name,
                   BDA_OrigName, &origname,
                   BDA_AddressString, &addr,
                   BDA_AddressType, &addrtype,
                   BDA_ClassOfDevice, &cod,
                   BDA_Appearance, &appearance,
                   BDA_RSSI, &rssi,
                   BDA_IsClassic, &isclassic,
                   BDA_IsLE, &isle,
                   BDA_IsRegistered, &isreg,
                   BDA_IsBonded, &isbonded,
                   BDA_IsConnected, &isconn,
                   BDA_IsDiscovered, &isdisc,
                   BDA_IsDead, &isdead,
                   BDA_NumServices, &numsvc,
                   BDA_BindingClass, &bcls,
                   TAG_END);
        if(bcls)
        {
            btGetAttrs(BGA_BTCLASS, bcls, BCA_ClassName, &binding, TAG_END);
        }
        Printf("  %s [%s%s] %s%s%s", addr, addrtype ? (STRPTR) "random" : (STRPTR) "public",
               isle && isclassic ? (STRPTR) ", dual" : (isle ? (STRPTR) ", LE" : (STRPTR) ", BR/EDR"),
               name ? name : (STRPTR) "<unnamed>",
               (origname && name && strcmp(origname, name)) ? (STRPTR) " (" : (STRPTR) "",
               (origname && name && strcmp(origname, name)) ? origname : (STRPTR) "");
        if(origname && name && strcmp(origname, name))
        {
            PutStr(")");
        }
        PutStr("\n");
        if(cod)
        {
            Printf("    Class of device 0x%06lx (%s", cod, btNumToStr(BNTS_MAJORCLASS, (cod >> 8) & 0x1f, "?"));
            Printf(", %s)\n", btNumToStr(BNTS_MINORCLASS, ((cod >> 8) & 0x1f) << 8 | ((cod >> 2) & 0x3f), "?"));
        }
        if(appearance)
        {
            Printf("    Appearance 0x%04lx (%s)\n", appearance, btNumToStr(BNTS_APPEARANCE, appearance, "?"));
        }
        if(rssi != 127)
        {
            Printf("    RSSI %ld dBm\n", rssi);
        }
        Printf("    %s%s%s%s%s%ld service(s)%s%s\n",
               isdisc ? (STRPTR) "discovered, " : (STRPTR) "",
               isreg ? (STRPTR) "registered, " : (STRPTR) "",
               isbonded ? (STRPTR) "bonded, " : (STRPTR) "",
               isconn ? (STRPTR) "connected, " : (STRPTR) "",
               isdead ? (STRPTR) "unreachable, " : (STRPTR) "",
               numsvc,
               binding ? (STRPTR) ", bound to " : (STRPTR) "",
               binding ? binding : (STRPTR) "");
        if(services)
        {
            ListServices(bd);
        }
        bd = bd->ln_Succ;
    }
}

int main(int argc, char *argv[])
{
    struct List *bthlist;
    struct Node *bth;
    struct List *bclist;
    struct Node *bc;
    BOOL all;

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        fail("Wrong arguments!\n");
    }
    all = ArgsArray[ARGS_ALL] || !(ArgsArray[ARGS_HARDWARE] || ArgsArray[ARGS_DEVICES] || ArgsArray[ARGS_CLASSES] || ArgsArray[ARGS_SERVICES]);

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        fail("Unable to open bluetooth.library\n");
    }

    btLockReadBase();
    if(all || ArgsArray[ARGS_HARDWARE] || ArgsArray[ARGS_DEVICES] || ArgsArray[ARGS_SERVICES])
    {
        btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &bthlist, TAG_END);
        bth = bthlist->lh_Head;
        if(!bth->ln_Succ)
        {
            PutStr("No Bluetooth hardware registered.\n");
        }
        while(bth->ln_Succ)
        {
            STRPTR devname, prodname, manufacturer, addr, localname, mnfname;
            IPTR unit, state, isclassic, isle, hciver, lmpver, mnfid, numdev, discovering, discoverable, connectable;
            btGetAttrs(BGA_HARDWARE, bth,
                       BHA_DeviceName, &devname,
                       BHA_DeviceUnit, &unit,
                       BHA_ProductName, &prodname,
                       BHA_Manufacturer, &manufacturer,
                       BHA_AddressString, &addr,
                       BHA_LocalName, &localname,
                       BHA_State, &state,
                       BHA_IsClassic, &isclassic,
                       BHA_IsLE, &isle,
                       BHA_HCIVersion, &hciver,
                       BHA_LMPVersion, &lmpver,
                       BHA_ManufacturerID, &mnfid,
                       BHA_ManufacturerName, &mnfname,
                       BHA_NumDevices, &numdev,
                       BHA_IsDiscovering, &discovering,
                       BHA_Discoverable, &discoverable,
                       BHA_Connectable, &connectable,
                       TAG_END);
            Printf("Hardware %s/%ld: %s (%s)\n", devname, unit, prodname, manufacturer);
            Printf("  Address %s, name '%s', %s\n", addr, localname ? localname : (STRPTR) "",
                   btNumToStr(BNTS_HWSTATE, state, "?"));
            Printf("  HCI %s, LMP %s, %s (0x%04lx), %s%s%s\n",
                   btNumToStr(BNTS_LMPVERSION, hciver, "?"),
                   btNumToStr(BNTS_LMPVERSION, lmpver, "?"),
                   mnfname, mnfid,
                   isclassic ? (STRPTR) "BR/EDR" : (STRPTR) "",
                   (isclassic && isle) ? (STRPTR) " + " : (STRPTR) "",
                   isle ? (STRPTR) "LE" : (STRPTR) "");
            Printf("  %ld device(s)%s%s%s\n", numdev,
                   discovering ? (STRPTR) ", discovering" : (STRPTR) "",
                   discoverable ? (STRPTR) ", discoverable" : (STRPTR) "",
                   connectable ? (STRPTR) ", connectable" : (STRPTR) "");
            if(all || ArgsArray[ARGS_DEVICES] || ArgsArray[ARGS_SERVICES])
            {
                ListDevices(bth, all || ArgsArray[ARGS_SERVICES]);
            }
            bth = bth->ln_Succ;
        }
    }
    if(all || ArgsArray[ARGS_CLASSES])
    {
        btGetAttrs(BGA_STACK, NULL, BSA_ClassList, &bclist, TAG_END);
        bc = bclist->lh_Head;
        if(!bc->ln_Succ)
        {
            PutStr("No classes loaded.\n");
        }
        while(bc->ln_Succ)
        {
            STRPTR clsname, fullpath;
            IPTR usecnt;
            btGetAttrs(BGA_BTCLASS, bc,
                       BCA_ClassName, &clsname,
                       BCA_FullPath, &fullpath,
                       BCA_UseCount, &usecnt,
                       TAG_END);
            Printf("Class %s (%s), %ld binding(s)\n", clsname, fullpath, usecnt);
            bc = bc->ln_Succ;
        }
    }
    btUnlockBase();
    fail(NULL);
    return(0);
}
