#include <conf.h>

#include <aros/libcall.h>
#include <libraries/bsdsocket.h>
#include <utility/tagitem.h>
#include <proto/utility.h>
#include <api/amiga_api.h>
#include <net/if_protos.h>
#include <netinet/in.h>

long __QueryInterfaceTagList(STRPTR name, const struct TagItem *tags, struct SocketBase *libPtr)
{
    struct ifnet *ifp;

#if defined(__AROS__)
    D(bug("[AROSTCP] amiga_netstat.c: __QueryInterfaceTagList()\n"));
#endif

    ifp = ifunit(name);
    if(ifp) {
        struct TagItem *tag;
        while(tag = NextTagItem((struct TagItem **)&tags)) {
            switch(tag->ti_Tag) {
            case IFQ_HardwareAddressSize: {
                *((ULONG *)tag->ti_Data) = ifp->if_addrlen;
                break;
            }
            case IFQ_MTU: {
                *((ULONG *)tag->ti_Data) = ifp->if_mtu;
                break;
            }
            case IFQ_BPS: {
                *((ULONG *)tag->ti_Data) = ifp->if_baudrate;
                break;
            }
            case IFQ_PacketsReceived: {
                *((ULONG *)tag->ti_Data) = ifp->if_ipackets;
                break;
            }
            case IFQ_PacketsSent: {
                *((ULONG *)tag->ti_Data) = ifp->if_opackets;
                break;
            }
            case IFQ_BadData: {
//				log(LOG_CRIT, "IFQ_BadData is not implemented");
                *((ULONG *)tag->ti_Data) = 0; /*** TODO ***/
                break;
            }
            case IFQ_Overruns: {
//				log(LOG_CRIT, "IFQ_Overruns is not implemented");
                *((ULONG *)tag->ti_Data) = 0;
                break;
            }
            case IFQ_UnknownTypes: {
                *((ULONG *)tag->ti_Data) = ifp->if_noproto;
                break;
            }
            case IFQ_LastStart: {
                memcpy((void *)tag->ti_Data, &ifp->if_data.ifi_aros_ontime, sizeof(struct timeval));
                break;
            }
            /*
             * The interface's first IPv4 address, and its destination
             * (point-to-point), broadcast address and netmask: a struct
             * sockaddr_in at ti_Data, all zero but the family and length
             * when the interface has none. ifa_broadaddr is ifa_dstaddr,
             * so which one it is comes from the flags, as for the SIOCGIF*
             * ioctls: the other stays zero.
             */
            case IFQ_Address:
            case IFQ_DestinationAddress:
            case IFQ_BroadcastAddress:
            case IFQ_NetMask: {
                struct sockaddr_in *sin = (struct sockaddr_in *)tag->ti_Data;
                struct ifaddr *ifa;

                bzero(sin, sizeof(*sin));
                sin->sin_len = sizeof(*sin);
                sin->sin_family = AF_INET;
                for(ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
                    struct sockaddr *sa;

                    if(ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
                        continue;
                    sa = tag->ti_Tag == IFQ_Address ? ifa->ifa_addr :
                         tag->ti_Tag == IFQ_NetMask ? ifa->ifa_netmask :
                         tag->ti_Tag == IFQ_DestinationAddress ?
                             ((ifp->if_flags & IFF_POINTOPOINT) ? ifa->ifa_dstaddr : NULL) :
                         ((ifp->if_flags & IFF_BROADCAST) ? ifa->ifa_broadaddr : NULL);
                    /* a netmask's sa_len may stop before its zero bytes */
                    if(sa != NULL && sa->sa_len > 4)
                        bcopy(&((struct sockaddr_in *)sa)->sin_addr, &sin->sin_addr,
                              sa->sa_len - 4 < 4 ? sa->sa_len - 4 : 4);
                    break;
                }
                break;
            }
            case IFQ_Metric: {
                *((ULONG *)tag->ti_Data) = ifp->if_metric;
                break;
            }
            case IFQ_State: {
                *((LONG *)tag->ti_Data) = (ifp->if_flags & IFF_UP) ? SM_Up : SM_Down;
                break;
            }
            case IFQ_AddressBindType: {
                /*** TODO: this is not 100% correct, there is also IFABT_Unknown ***/
                if(ifp->if_data.ifi_aros_usedhcp)
                    *((LONG *)tag->ti_Data) = IFABT_Dynamic;
                else
                    *((LONG *)tag->ti_Data) = IFABT_Static;
                break;
            }
            case IFQ_AddressLeaseExpires: {
                /* Implementing this requires        */
                bzero((void *)tag->ti_Data, sizeof(struct DateStamp)); /* tighter integration of dhclient   */
                break;				                       /* and kernel. Left for better times */
            }
            case IFQ_PrimaryDNSAddress:
            case IFQ_SecondaryDNSAddress: {
                bzero((void *)tag->ti_Data, sizeof(struct sockaddr_in)); /* See above. In addition we don't */
                break;				                         /* know what is SANA-IIR4 yet	    */
            }
            case IFQ_GetBytesIn: {
                ((SBQUAD_T *)tag->ti_Data)->sbq_High = 0;
                ((SBQUAD_T *)tag->ti_Data)->sbq_Low = ifp->if_ibytes;
                break;
            }
            case IFQ_GetBytesOut: {
                ((SBQUAD_T *)tag->ti_Data)->sbq_High = 0;
                ((SBQUAD_T *)tag->ti_Data)->sbq_Low = ifp->if_obytes;
                break;
            }
            default: {
                if((ifp->if_query) && (ifp->if_query(ifp, tag) == 0))
                    break;
                *((ULONG *)tag->ti_Data) = 0;
            }
            };

        }
        return 0;
    } else {
        writeErrnoValue(libPtr, ENXIO);
        return -1;
    }
}

#if defined(__CONFIG_ROADSHOW__)
AROS_LH2(long, QueryInterfaceTagList,
         AROS_LHA(STRPTR, name, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         struct SocketBase *, libPtr, 78, UL)
{
    AROS_LIBFUNC_INIT

#if defined(__AROS__)
    D(bug("[AROSTCP.RS] amiga_netstat.c: QueryInterfaceTagList()\n"));
#endif

    return  __QueryInterfaceTagList(name, tags, libPtr);

    AROS_LIBFUNC_EXIT
}
#endif
