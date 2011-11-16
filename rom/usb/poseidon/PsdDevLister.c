/*
** PsdDevLister by Chris Hodges <chrisly@platon42.de>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/exec.h>
#include <libraries/poseidon.h>
#include <dos/dos.h>

#include <proto/poseidon.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARGS_SHOWROOT 0
#define ARGS_QUICK    1
#define ARGS_STRINGS  2
#define ARGS_SIZEOF   3

static const char *template = "SHOWROOT/S,QUICK/S,STRINGS/S";
const char *version = "$VER: PsdDevLister 4.0 (03.06.09) by Chris Hodges <chrisly@platon42.de>";
static IPTR ArgsArray[ARGS_SIZEOF];
static struct RDArgs *ArgsHook = NULL;

void fail(char *str)
{
    if(ArgsHook)
    {
        FreeArgs(ArgsHook);
        ArgsHook = NULL;
    }
    if(str)
    {
        PutStr(str);
        exit(20);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    struct Library *ps;
    STRPTR errmsg = NULL;
    APTR pd;
    APTR pp;
    struct MsgPort *mp;
    IPTR devadr;
    IPTR devhubport;
    struct Node *devhub;
    IPTR devusbvers;
    IPTR devclass;
    IPTR devsubclass;
    IPTR devproto;
    IPTR devvendorid;
    IPTR devprodid;
    IPTR devversion;
    STRPTR devmanufact;
    STRPTR devprodname;
    STRPTR devserial;
    STRPTR devidstr;
    STRPTR devhubname;
    IPTR devcurrlang;
    IPTR devclonecount;
    UWORD *devlangarray;
    IPTR devislowspeed;
    IPTR devishighspeed;
    IPTR devisconnected;
    IPTR devhasaddress;
    IPTR devhasdevdesc;
    IPTR devisconfigured;
    IPTR devissuspended;
    IPTR devneedssplit;
    IPTR devnumcfgs;
    IPTR devlowpower;
    IPTR devpoweravail;
    IPTR devpowerdrain;
    IPTR devmaxpktsize0;
    IPTR devhubthinktime;
#ifdef AROS_USB30_CODE
    IPTR devissuperspeed;
#endif

    struct List *cfgs;
    struct Node *pc;
    IPTR cfgselfpow;
    IPTR cfgremwake;
    IPTR cfgnum;
    IPTR cfgmaxpower;
    STRPTR cfgname;
    IPTR cfgnumifs;

    struct List *ifs;
    struct Node *pif;
    struct Node *altpif;
    struct List *altpiflist;

    struct List *descriptors = NULL;
    struct Node *pdd;
    IPTR desctype;
    UBYTE *descdata;
    IPTR desclen;
    UWORD cnt;
    STRPTR descname;

    IPTR ifnum;
    IPTR ifaltnum;
    IPTR ifclass;
    IPTR ifsubclass;
    IPTR ifproto;
    STRPTR ifname;
    STRPTR ifidstr;
    IPTR ifnumeps;

    APTR binding;
    IPTR hasappbinding;
    struct Task *apptask;
    struct Library *bindingcls;

    struct List *eps;
    struct Node *pep;
    IPTR episin;
    IPTR epnum;
    IPTR eptranstype;
    IPTR epmaxpktsize;
    IPTR epinterval;
    IPTR epnumtransmu;
    IPTR epsynctype;
    IPTR epusagetype;

    STRPTR strdesc;
    STRPTR strthinktime;

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))        
    {
        fail("Wrong arguments!\n");
    }
    
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        pd = NULL;
        mp = CreateMsgPort();
        psdLockReadPBase();
        while((pd = psdGetNextDevice(pd)))
        {
            psdLockReadDevice(pd);
            devhubport = 0;
            devneedssplit = 0;
            devlowpower = 0;
            devpoweravail = 0;
            devpowerdrain = 0;
            devmaxpktsize0 = 0;
            devhubthinktime = 0;
            devissuspended = 0;
            psdGetAttrs(PGA_DEVICE, pd,
                        DA_Address, &devadr,
                        DA_HubDevice, &devhub,
                        DA_HubThinkTime, &devhubthinktime,
                        DA_AtHubPortNumber, &devhubport,
                        DA_UsbVersion, &devusbvers,
                        DA_MaxPktSize0, &devmaxpktsize0,
                        DA_Class, &devclass,
                        DA_SubClass, &devsubclass,
                        DA_Protocol, &devproto,
                        DA_VendorID, &devvendorid,
                        DA_ProductID, &devprodid,
                        DA_Version, &devversion,
                        DA_Manufacturer, &devmanufact,
                        DA_ProductName, &devprodname,
                        DA_SerialNumber, &devserial,
                        DA_CloneCount, &devclonecount,
                        DA_IDString, &devidstr,
                        DA_CurrLangID, &devcurrlang,
                        DA_LangIDArray, &devlangarray,
                        DA_IsLowspeed, &devislowspeed,
                        DA_IsHighspeed, &devishighspeed,
                        #ifdef AROS_USB30_CODE
                        DA_IsSuperspeed, &devissuperspeed,
                        #endif
                        DA_IsConnected, &devisconnected,
                        DA_NeedsSplitTrans, &devneedssplit,
                        DA_HasAddress, &devhasaddress,
                        DA_HasDevDesc, &devhasdevdesc,
                        DA_IsConfigured, &devisconfigured,
                        DA_IsSuspended, &devissuspended,
                        DA_NumConfigs, &devnumcfgs,
                        DA_ConfigList, &cfgs,
                        DA_Binding, &binding,
                        DA_BindingClass, &bindingcls,
                        DA_HasAppBinding, &hasappbinding,
                        DA_LowPower, &devlowpower,
                        DA_PowerDrained, &devpowerdrain,
                        DA_PowerSupply, &devpoweravail,
                        DA_DescriptorList, &descriptors,
                        TAG_END);
            if(devhub)
            {
                psdGetAttrs(PGA_DEVICE, devhub,
                            DA_ProductName, &devhubname,
                            TAG_END);
            } else {
                devhubname = "Root";
                if(!ArgsArray[ARGS_SHOWROOT])
                {
                    Printf("Skipping '%s' (use SHOWROOT to list)\n", devidstr);
                    PutStr("---------------------------------------------------------------------------\n");
    	            psdUnlockDevice(pd);
                    continue;
                }
            }
            strthinktime = "";
            if((devclass == HUB_CLASSCODE) && devishighspeed)
            {
                switch(devhubthinktime)
                {
                    case 0:
                        strthinktime = " (Hub Think Time: 8 bit times)";
                        break;
                    case 1:
                        strthinktime = " (Hub Think Time: 16 bit times)";
                        break;
                    case 2:
                        strthinktime = " (Hub Think Time: 24 bit times)";
                        break;
                    case 3:
                        strthinktime = " (Hub Think Time: 32 bit times)";
                        break;
                }
            }
            Printf("  Poseidon DevID  : '%s'\n"
                   "  Product Name    : '%s' (ID: %04lx, Vers: %04lx)\n"
                   "  Manufacturer    : '%s' (Vendor: %04lx (%s))\n"
                   "  Serial Number   : '%s' (USBVers: %04lx)\n"
                   "  Device State    : %s%s%s%s%s%s%s\n"
                   "  Device Address  : %ld (Port %ld at %s)\n"
                   "  Class/Sub/Proto : %ld/%ld/%ld (%s)\n"
                   "  MaxPktSize EP 0 : %ld%s\n"
                   "  Power Check     : Supply = %ldmA, Drain = %ldmA%s\n"
                   "  Current Language: %s\n",
                   devidstr,
                   devprodname, devprodid, devversion,
                   devmanufact, devvendorid,
                   psdNumToStr(NTS_VENDORID, (LONG) devvendorid, "unknown"),
                   devserial, devusbvers,
                    #ifdef AROS_USB30_CODE
                   devislowspeed ? "lowspeed " : (devissuperspeed ? "superspeed " : (devishighspeed ? "highspeed " : "fullspeed ")),
                    #else
                   devislowspeed ? "lowspeed " : (devishighspeed ? "highspeed " : "fullspeed "),
                    #endif
                   devisconnected ? "connected " : "disconnected ",
                   devhasaddress ? "hasaddress " : "",
                   devhasdevdesc ? "hasdevdesc " : "",
                   devisconfigured ? "configured " : "",
                   devissuspended ? "suspended " : "",
                   devneedssplit ? "\n  SplitTransaction: USB1.1 at USB2.0 port!" : "",
                   devadr,
                   devhubport, devhubname,
                   devclass, devsubclass, devproto,
                   psdNumToStr(NTS_COMBOCLASS, (devclass<<NTSCCS_CLASS)|(devsubclass<<NTSCCS_SUBCLASS)|(devproto<<NTSCCS_PROTO)|
                                               NTSCCF_CLASS|NTSCCF_SUBCLASS|NTSCCF_PROTO, "<unknown>"),
                   devmaxpktsize0, strthinktime,
                   devpoweravail, devpowerdrain,
                   devlowpower ? " LOWPOWER!" : "",
                   psdNumToStr(NTS_LANGID, devcurrlang, "<unknown>"));
            if(devlangarray && (!ArgsArray[ARGS_QUICK]))
            {
                UWORD *wptr = devlangarray;
                ULONG perline = 0;
                PutStr("  Supported Langs : ");
                while(*wptr)
                {
                    PutStr(psdNumToStr(NTS_LANGID, (ULONG) *wptr, "<unknown>"));
                    wptr++;
                    if(*wptr)
                    {
                        PutStr(", ");
                    }
                    if(++perline > 2)
                    {
                        PutStr("\n                    ");
                        perline = 0;
                    }
                }
                PutStr("\n");
                if(ArgsArray[ARGS_STRINGS])
                {
                    ULONG idx = 1;
                    pp = psdAllocPipe(pd, mp, NULL);
                    while(pp)
                    {
                        strdesc = psdGetStringDescriptor(pp, idx);
                        if(strdesc)
                        {
                            Printf("  String descriptor %04ld: '%s'\n", idx, strdesc);
                            psdFreeVec(strdesc);
                        } else {
                            if(idx > 10)
                            {
                                break;
                            }
                        }
                        idx++;
                        if(idx > 100)
                        {
                            PutStr("  ... aborting\n");
                            break;
                        }
                    }
                    psdFreePipe(pp);
                }
            }
            if(binding)
            {
                if(hasappbinding)
                {
                    psdGetAttrs(PGA_APPBINDING, binding,
                                ABA_Task, &apptask,
                                TAG_END);
                    Printf("\n  This device is bound to application %s with context %08lx.\n",
                           apptask->tc_Node.ln_Name, binding);
                } else {
                    Printf("\n  This device is bound to %s, context %08lx.\n",
                           bindingcls->lib_Node.ln_Name, binding);
                }
            }
            Printf("\n  %ld configuration(s):\n", devnumcfgs);
            pc = cfgs->lh_Head;
            while(pc->ln_Succ)
            {
                psdGetAttrs(PGA_CONFIG, pc,
                            CA_SelfPowered, &cfgselfpow,
                            CA_RemoteWakeup, &cfgremwake,
                            CA_ConfigNum, &cfgnum,
                            CA_MaxPower, &cfgmaxpower,
                            CA_ConfigName, &cfgname,
                            CA_NumInterfaces, &cfgnumifs,
                            CA_InterfaceList, &ifs,
                            TAG_END);
                Printf("\n  · Config %ld (%s)\n"
                       "    Attrs   : %s %s\n"
                       "    MaxPower: %ld mA\n",
                       cfgnum, cfgname,
                       cfgselfpow ? "self-powered " : "bus-powered ",
                       cfgremwake ? "remote-wakeup" : "",
                       cfgmaxpower);
                Printf("\n    %ld interface(s) for this config:", cfgnumifs);
                pif = ifs->lh_Head;
                while(pif->ln_Succ)
                {
                    altpif = pif;
                    do
                    {
                        psdGetAttrs(PGA_INTERFACE, altpif,
                                    IFA_InterfaceNum, &ifnum,
                                    IFA_AlternateNum, &ifaltnum,
                                    IFA_Class, &ifclass,
                                    IFA_SubClass, &ifsubclass,
                                    IFA_Protocol, &ifproto,
                                    IFA_InterfaceName, &ifname,
                                    IFA_IDString, &ifidstr,
                                    IFA_NumEndpoints, &ifnumeps,
                                    IFA_EndpointList, &eps,
                                    IFA_AlternateIfList, &altpiflist,
                                    IFA_Binding, &binding,
                                    IFA_BindingClass, &bindingcls,
                                    TAG_END);
                        Printf("\n    · Interface %ld (%s) (ID: '%s')\n"
                               "      Alternate Setting: %ld\n"
                               "      Class/Sub/Proto  : %ld/%ld/%ld (%s)\n",
                               ifnum, ifname, ifidstr,
                               ifaltnum,
                               ifclass, ifsubclass, ifproto,
                               psdNumToStr(NTS_COMBOCLASS, (ifclass<<NTSCCS_CLASS)|(ifsubclass<<NTSCCS_SUBCLASS)|(ifproto<<NTSCCS_PROTO)|
                                               NTSCCF_CLASS|NTSCCF_SUBCLASS|NTSCCF_PROTO, "<unknown>"));
                        if(binding)
                        {
                            Printf("\n      This interface is bound to %s, context %08lx.\n",
                                   bindingcls->lib_Node.ln_Name, binding);
                        }
                        if(ArgsArray[ARGS_QUICK])
                        {
                            Printf("\n      %ld endpoint(s).\n", ifnumeps);
                        } else {
                            Printf("\n      %ld endpoint(s) for this interface:\n", ifnumeps);
                            pep = eps->lh_Head;
                            while(pep->ln_Succ)
                            {
                                epnumtransmu = 0;
                                epsynctype = 0;
                                epusagetype = 0;
                                psdGetAttrs(PGA_ENDPOINT, pep,
                                            EA_IsIn, &episin,
                                            EA_EndpointNum, &epnum,
                                            EA_TransferType, &eptranstype,
                                            EA_MaxPktSize, &epmaxpktsize,
                                            EA_Interval, &epinterval,
                                            EA_NumTransMuFrame, &epnumtransmu,
                                            EA_SyncType, &epsynctype,
                                            EA_UsageType, &epusagetype,
                                            TAG_END);
                                Printf("      · Endpoint %ld (%s %s)\n"
                                       "        MaxPktSize: %s%ld\n",
                                       epnum, psdNumToStr(NTS_TRANSTYPE, eptranstype, "?"),
                                       episin ? "<-[ IN" : "OUT ]->",
                                       (epnumtransmu == 2) ? "2x " : ((epnumtransmu == 3) ? "3x " : ""),
                                       epmaxpktsize);

                                if(devishighspeed || ((eptranstype != USEAF_CONTROL) && (eptranstype != USEAF_BULK)))
                                {
                                    Printf("        %s  : %ld %s\n",
                                           (((eptranstype == USEAF_CONTROL) || (eptranstype == USEAF_BULK)) && devishighspeed) ? "NAK-Rate" : "Interval",
                                           epinterval,
                                           devishighspeed ? "µFrames" : "ms");
                                }
                                if(eptranstype == USEAF_ISOCHRONOUS)
                                {
                                     Printf("        SyncType  : %s\n"
                                            "        UsageType : %s\n",
                                            psdNumToStr(NTS_SYNCTYPE, epsynctype, "?"),
                                            psdNumToStr(NTS_USAGETYPE, epusagetype, "?"));
                                }
                                pep = pep->ln_Succ;
                            }
                        }
                        if(altpif == pif)
                        {
                            altpif = (struct Node *) altpiflist->lh_Head;
                            /* check for alternate settings */
                            if(!altpif->ln_Succ)
                            {
                                Printf("\n      No alternate settings.\n");
                                break;
                            }
                            Printf("\n      Alternate settings:\n");
                        } else {
                            altpif = altpif->ln_Succ;
                            if(!altpif->ln_Succ)
                            {
                                break;
                            }
                        }
                    } while(TRUE);
                    pif = pif->ln_Succ;
                }
                pc = pc->ln_Succ;
            }
            if(descriptors && (!ArgsArray[ARGS_QUICK]))
            {
                pdd = descriptors->lh_Head;
                PutStr("\n  Standard Descriptors:");
                while(pdd->ln_Succ)
                {
                    pc = NULL;
                    pif = NULL;
                    pep = NULL;
                    ifclass = 0;
                    descname = NULL;
                    psdGetAttrs(PGA_DESCRIPTOR, pdd,
                                DDA_Config, &pc,
                                DDA_Interface, &pif,
                                DDA_Endpoint, &pep,
                                DDA_DescriptorType, &desctype,
                                DDA_DescriptorLength, &desclen,
                                DDA_DescriptorData, &descdata,
                                DDA_Name, &descname,
                                TAG_END);
                    if(pif)
                    {
                        psdGetAttrs(PGA_INTERFACE, pif,
                                    IFA_Class, &ifclass,
                                    TAG_END);
                    }
                    if(!descname)
                    {
                        descname = psdNumToStr(NTS_DESCRIPTOR, desctype|(ifclass<<8), NULL);
                    }
                    if(!descname)
                    {
                        descname = psdNumToStr(NTS_DESCRIPTOR, desctype, "<unknown>");
                    }
                    Printf("\n    Desc. %02lx (%s), %ld bytes",
                           desctype,
                           descname,
                           desclen);
                    if(pc)
                    {
                        psdGetAttrs(PGA_CONFIG, pc,
                                    CA_ConfigNum, &cfgnum,
                                    TAG_END);
                        if(pif)
                        {
                            psdGetAttrs(PGA_INTERFACE, pif,
                                        IFA_InterfaceNum, &ifnum,
                                        IFA_AlternateNum, &ifaltnum,
                                        TAG_END);
                            if(pep)
                            {
                                psdGetAttrs(PGA_ENDPOINT, pep,
                                            EA_EndpointNum, &epnum,
                                            TAG_END);
                                Printf(" (EP %ld, IF %ld/%ld, Cfg %ld)", epnum, ifnum, ifaltnum, cfgnum);
                            } else {
                                Printf(" (Iface %ld/%ld, Cfg %ld)", ifnum, ifaltnum, cfgnum);
                            }
                        } else {
                            Printf(" (Config %ld)", cfgnum);
                        }
                    }
                    // skip basic descriptors
                    if(!((desctype >= UDT_DEVICE) && (desctype <= UDT_ENDPOINT)))
                    {
                        PutStr("\n");
                        cnt = 0;
                        while(desclen--)
                        {
                            if(!(cnt & 15))
                            {
                                Printf("      %02lx:", cnt);
                            }
                            Printf(" %02lx", *descdata++);
                            if((!(++cnt & 15)) && desclen)
                            {
                                PutStr("\n");
                            }
                        }
                    }
                    pdd = pdd->ln_Succ;
                }
                PutStr("\n");
            }
            Printf("\n  Google: http://www.google.com/search?q=usb+0x%04lx+0x%04lx\n",
                   devvendorid, devprodid);
            psdUnlockDevice(pd);
            PutStr("---------------------------------------------------------------------------\n");
        }
        psdUnlockPBase();
        DeleteMsgPort(mp);
        CloseLibrary(ps);
    } else {
        errmsg = "Unable to open poseidon.library\n";
    }

    fail(errmsg);
    return(0); // never gets here, just to shut the compiler up
}

