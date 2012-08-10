/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2012 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#define DEBUG 0
#include <aros/debug.h>

#include <conf.h>

#include <exec/errors.h>

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>
#include <kern/amiga_gui.h>

#include <sys/synch.h>

#include <net/if.h>
#include <net/netisr.h>

#define NDEBUG
#include <assert.h>

#if INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#if NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <net/if_sana.h>
#include <net/if_types.h>
#include <net/sana2arp.h>

#include <net/sana2config.h>
#include <net/sana2request.h>
#include <net/sana2errno.h>

#include <libraries/bsdsocket.h>
#include <libraries/miamipanel.h>
#include <proto/dos.h>

#define ARP_MTU (sizeof(struct s2_arppkt))

int debug_sana = 1;

/* Global port for all SANA-II network interfaces */
struct MsgPort *SanaPort = NULL;

/* queue for sana network interfaces */
struct sana_softc *ssq = NULL;

/* These are wire type dependant parameters of 
 * Sana-II Network Interface
 */
//extern struct wiretype_parameters wiretype_table[];

/* 
 * Local prototypes
 */
struct ifnet *iface_make(struct ssconfig *ifc);
static void sana_run(struct sana_softc *ssc, int requests, struct ifaddr *ifa);
static void sana_unrun(struct sana_softc *ssc);
static void sana_up(struct sana_softc *ssc);
static BOOL sana_down(struct sana_softc *ssc);
static struct mbuf *
sana_read(struct sana_softc *ssc, struct IOIPReq *req, 
	  UWORD  flags, UWORD *sent, const char *banner, size_t mtu);
static void sana_ip_read(struct sana_softc *ssc, struct IOIPReq *req);
static void sana_arp_read(struct sana_softc *ssc, struct IOIPReq *req);
static void sana_online(struct sana_softc *ssc, struct IOIPReq *req);
static void sana_connect(struct sana_softc *ssc, struct IOIPReq *req);
static void free_written_packet(struct sana_softc *ssc, struct IOIPReq *req);
static int sana_query(struct ifnet *ifn, struct TagItem *tag);

/*
 * Initialize Sana-II interface
 *
 * This routine creates needed message port for Sana-II IO
 * It returns our signal mask, or 0L on an error.
 */
ULONG 
sana_init(void)
{
  assert(!SanaPort);

  SanaPort = CreateMsgPort();	/* V36 function, creates a PA_SIGNAL port */

  if (SanaPort) {
    SanaPort->mp_Node.ln_Name = (void *)"sana_if.port";
    loattach();
    return (ULONG) 1 << SanaPort->mp_SigBit;
  }

  return 0L;
}

/*
 * Clean up Sana-II Interfaces
 *
 * Note: main interface queue is SNAFU after deinitializing
 */
void 
sana_deinit(void)
{
  struct sana_softc *ssc; 
  struct IOSana2Req *req;

  assert(SanaPort);

  while (ssq) {
    sana_down(ssq);
    if (ssq->ss_if.if_flags & IFF_RUNNING) {
      sana_unrun(ssq);
    }
    ssc = ssq;
    ssq = ssc->ss_next;
    /* Close device */ 
    req = CreateIOSana2Req(ssc);
    if (req) {
      CloseDevice((struct IORequest*)req);
      DeleteIOSana2Req(req);
    } else {
      __log(LOG_ERR, "sana_deinit(): Couldn't close device %s\n",
	  ssc->ss_name);
    }
  }

  if (SanaPort) {
    /* Clear possible pending signals */
    SetSignal(1<<SanaPort->mp_SigBit, 0L);
    DeleteMsgPort(SanaPort);
    SanaPort = NULL;
  }
}

/*
 * sana_poll()
 *  This routine polls SanaPort and processes replied
 *  requests appropriately
 */
BOOL
sana_poll(void)
{
  struct IOIPReq * io;
  spl_t s = splnet();

  while (io = (struct IOIPReq *)GetMsg(SanaPort)) {
    /* touch the network interface */
    GetSysTime(&io->ioip_if->ss_if.if_lastchange);
    if (io->ioip_dispatch) {
      (*io->ioip_dispatch)(io->ioip_if, io);
     } else {
       __log(LOG_ERR, "No dispatch function in request for %s\n",
	   io->ioip_if->ss_name);
     }
  }

  net_poll();

  splx(s);

  return FALSE;
}

#ifdef COMPAT_AMITCP2
/*
 * Name points to the full device name.
 * Device name is a legal DOS file name,
 * appended with a slash and a decimal unit number
 *
 * Some explanation on the device names:
 * There is a DOS wrapper around Exec OpenDevice() function.
 * The device is first searched from the Exec list, if that fails
 * DOS tries to load the segment file with the device name. 
 * If that fails too, the filename is catenated to string "DEVS:" and
 * DOS tries again. 
 *
 * AmiTCP uses internally only the Exec device name (ie. device name
 * without pathpart)
 */

/*
 * Map exec device name to
 * interface structure pointer.
 */
struct ifnet *aifunit(register char *name)
{
  register char *cp;
  register struct ifnet *ifp;
  long unit;
  unsigned len;
  char *ep, c;

  /* AmigaTCP/IP uses the slash as unit number separator 
   * because Exec device name may contain digits.
   */
  char *up;
  cp = ep = name - 1;
  /* Find pathpart */
  for (up = name; *up; up++) 
    if (*up == '/' || *up == ':') {
      cp = ep;
      ep = up;
    }
  /* Name is too long, or there is no unit number */
  if (up >= cp + IFNAMSIZ || cp == ep)	
    return ((struct ifnet *)0);
  cp++;

  /*
   * cp points first char in device name,
   * ep to unit number separator ('/')
   * and up to NUL ('\0') at the end of string
   */
  len = ep - cp;
  c = *ep;
  *ep = '\0';			/* sentinel */
  for (unit = 0, up--; *up >= '0' && *up <= '9'; up--) 
    unit = unit * 10 + *up - '0';
  if (up != ep) {
    *ep = c;
    return NULL;
  }

  /* Pathpart is not included in search */
  for (ifp = ifnet; ifp; ifp = ifp->if_next) {
    if (bcmp(ifp->if_name, cp, len))
      continue;
    if (unit == ifp->if_unit)
      break;
  }
  {
    extern struct ifnet *aiface_find(char *, long unit);
    *ep = '\0';			/* sentinel */
    if (ifp == 0)
      ifp = aiface_find(name, unit);
    *ep = c;
  }
  return (ifp);
}

struct ifnet *
aiface_find(char *name, long unit)
{
  struct  = sana2tag_find_exec(name, unit);

  /* No alias found, use defaults */
  if (sifp == NULL) {
    static short sana_units = 0;
    struct interface_parameters sifp[1];
    const static long tag_end = TAG_END;

    sifp->ifname = "sana";
    sifp->unit = sana_units++;
    sifp->execname = name;
    sifp->execunit = unit;
    sifp->tags = (struct TagItem *)&tag_end;
    return make_iface(sifp, sifp->unit);
  }
  return make_iface(sifp, sifp->unit);
}
#endif

struct ifnet *
iface_make(struct ssconfig *ifc)
{
	register struct sana_softc *ssc = NULL;
	register struct IOSana2Req *req;
	struct Sana2DeviceQuery devicequery;

	/* Allocate the request for opening the device */
	if ((req = CreateIOSana2Req(NULL)) == NULL) 
	{
		__log(LOG_ERR, "iface_find(): CreateIOSana2Req failed\n");
	}
	else
	{
		req->ios2_BufferManagement = buffermanagement;

		DSANA(__log(LOG_DEBUG,"Opening device %s unit %ld", ifc->args->a_dev, *ifc->args->a_unit);)
		if (OpenDevice(ifc->args->a_dev, *ifc->args->a_unit, 
			(struct IORequest *)req, 0L))
		{
			sana2perror("OpenDevice", req);
			
			/* Allocate the interface structure */
			ssc = (struct sana_softc *)
			bsd_malloc(sizeof(*ssc) + strlen(ifc->args->a_dev) + 1,
							M_IFNET, M_WAITOK);
  
			if (!ssc)
			{
				__log(LOG_ERR, "iface_find: out of memory\n");
			}
			else
			{
				aligned_bzero_const(ssc, sizeof(*ssc));
			
				/* Save request pointers */
				ssc->ss_dev     = req->ios2_Req.io_Device;
				ssc->ss_unit    = req->ios2_Req.io_Unit;

				ssc->ss_if.if_type = IFT_OTHER;
				ssc->ss_if.if_flags &= ~(IFF_RUNNING|IFF_UP);

				/* Initialize */ 

D(bug("[AROSTCP] if_sana.c: iface_make: Current IP from config = %s\n", ifc->args[0].a_ip));
				ifc->args[0].a_ip = "0.0.0.0";
D(bug("[AROSTCP] if_sana.c: iface_make: IP set to 0.0.0.0\n"));

				ssconfig(ssc, ifc);
	
				NewList((struct List*)&ssc->ss_freereq);

				if_attach((struct ifnet*)ssc);
				ifinit();
	
				ssc->ss_next = ssq;
				ssq = ssc;
			}
		}
		else
		{
			/* Ask for our type, address length, MTU
			* Obl. bitch: nobody tells, WHO is supplying
			* DevQueryFormat and DeviceLevel
			*/
			req->ios2_Req.io_Command   = S2_DEVICEQUERY;
			req->ios2_StatData         = &devicequery;
			devicequery.SizeAvailable  = sizeof(devicequery);
			devicequery.DevQueryFormat = 0L;

			DoIO((struct IORequest *)req);
			if (req->ios2_Req.io_Error)
			{
				sana2perror("S2_DEVICEQUERY", req);
			}
			else
			{
				/* Get Our Station address */
				req->ios2_StatData = NULL;
				req->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
				DoIO((struct IORequest *)req);
		
				if (req->ios2_Req.io_Error)
				{
					sana2perror("S2_GETSTATIONADDRESS", req);
				}
				else
				{
					req->ios2_Req.io_Command = 0;
		  
					/* Allocate the interface structure */
					ssc = (struct sana_softc *)
					bsd_malloc(sizeof(*ssc) + strlen(ifc->args->a_dev) + 1,
									M_IFNET, M_WAITOK);
		  
					if (!ssc)
					{
						__log(LOG_ERR, "iface_find: out of memory\n");
					}
					else
					{
						aligned_bzero_const(ssc, sizeof(*ssc));
			
						/* Save request pointers */
						ssc->ss_dev     = req->ios2_Req.io_Device;
						ssc->ss_unit    = req->ios2_Req.io_Unit;
						ssc->ss_bufmgnt = req->ios2_BufferManagement;
						
						/* Address must be full bytes */
						ssc->ss_if.if_addrlen  = (devicequery.AddrFieldSize + 7) >> 3;
						bcopy(req->ios2_DstAddr, ssc->ss_hwaddr, ssc->ss_if.if_addrlen);
						ssc->ss_if.if_mtu      = devicequery.MTU;
						ssc->ss_maxmtu         = devicequery.MTU;
						ssc->ss_if.if_baudrate = devicequery.BPS;
						ssc->ss_hwtype         = devicequery.HardwareType;	
						
						/* These might be different on different hwtypes */
						ssc->ss_if.if_output = sana_output;
						ssc->ss_if.if_ioctl  = sana_ioctl;
						ssc->ss_if.if_query  = sana_query;

						/* Map SANA-II hardware types to RFC1573 standard */
						switch (ssc->ss_hwtype) 
						{
						case S2WireType_Ethernet:
							ssc->ss_if.if_type = IFT_ETHER;
							break;
						case S2WireType_IEEE802:
							ssc->ss_if.if_type = IFT_IEEE80211;
							break;
						case S2WireType_Arcnet:
							ssc->ss_if.if_type = IFT_ARCNET;
							break;
						case S2WireType_LocalTalk:
							ssc->ss_if.if_type = IFT_LOCALTALK;
							break;
						case S2WireType_PPP:
							ssc->ss_if.if_type = IFT_PPP;
							break;
						case S2WireType_SLIP:
						case S2WireType_CSLIP:
							ssc->ss_if.if_type = IFT_SLIP;
							break;
						case S2WireType_PLIP:
							ssc->ss_if.if_type = IFT_PARA;
							break;
						default:
							ssc->ss_if.if_type = IFT_OTHER;
						}

						/* Initialize */ 
						ssconfig(ssc, ifc);
			
						NewList((struct List*)&ssc->ss_freereq);

						if_attach((struct ifnet*)ssc);
						ifinit();
			
						ssc->ss_next = ssq;
						ssq = ssc;
					}
				}
			}
			if (!ssc)
				CloseDevice((struct IORequest *)req);
		}    
		DeleteIOSana2Req(req);
	}

  return (struct ifnet *)ssc;
}

/*
 * Allocate Sana-II IORequests for TCP/IP process
 */
static void
sana_run(struct sana_softc *ssc, int requests, struct ifaddr *ifa)
{
  int i;
  spl_t s = splimp();
  struct IOIPReq *req, *next = ssc->ss_reqs;

  DSANA(__log(LOG_DEBUG,"sana_run(%s%d) called",ssc->ss_if.if_name, ssc->ss_if.if_unit);)
  /*
   * Configure the Sana-II device driver
   * (now with factory address)
   */
  if ((ssc->ss_if.if_flags & IFF_RUNNING) == 0) {
    struct IOSana2Req *req;

    if (req = CreateIOSana2Req(ssc)) {
      req->ios2_Req.io_Command = S2_CONFIGINTERFACE;
      bcopy(ssc->ss_hwaddr, req->ios2_SrcAddr, ssc->ss_if.if_addrlen);

      DoIO((struct IORequest*)req);

      if (req->ios2_Req.io_Error == 0 ||
	  req->ios2_WireError == S2WERR_IS_CONFIGURED) {
	    /* Mark us as running */
	    ssc->ss_if.if_flags |= IFF_RUNNING;
	    if (ssc->ss_cflags & SSF_TRACK) {
#ifdef INET
          /* Ask for packet type specific statistics */
          req->ios2_Req.io_Command = S2_TRACKTYPE;
          req->ios2_PacketType = ssc->ss_ip.type;
          DoIO((struct IORequest*)req);
          /* It is *not* safe to turn tracking off */
          if (req->ios2_Req.io_Error &&
              req->ios2_WireError != S2WERR_ALREADY_TRACKED)
            sana2perror("S2_TRACKTYPE for IP", req);
          if (ssc->ss_arp.reqno) {
            req->ios2_Req.io_Command = S2_TRACKTYPE;
            req->ios2_PacketType = ssc->ss_arp.type;
            DoIO((struct IORequest*)req);
            if (req->ios2_Req.io_Error &&
              req->ios2_WireError != S2WERR_ALREADY_TRACKED)
            sana2perror("S2_TRACKTYPE for ARP", req);
          }
#endif
        }
      }
      else
      {
        sana2perror("S2_CONFIGINTERFACE", req);
      }
      DeleteIOSana2Req(req);
    }
  }

  if ((ssc->ss_if.if_flags & IFF_RUNNING)) {
    /* Initialize ioRequests, add them into free queue */
    for (i = 0; i < requests ; i++) {
      if (!(req = CreateIORequest(SanaPort, sizeof(*req)))) break;
      req->ioip_s2.ios2_Req.io_Device    = ssc->ss_dev;    
      req->ioip_s2.ios2_Req.io_Unit      = ssc->ss_unit;   
      req->ioip_s2.ios2_BufferManagement = ssc->ss_bufmgnt;
      aligned_bcopy(ssc->ss_hwaddr, req->ioip_s2.ios2_SrcAddr, ssc->ss_if.if_addrlen);
      req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
      req->ioip_s2.ios2_Data = req;
      req->ioip_if = ssc;
      req->ioip_next = next;
      AddTail((struct List*)&ssc->ss_freereq, (struct Node*)req);
      next = req;
    }
    ssc->ss_reqs = next;

    /* Order a notify when driver connects to a (wireless) network */
    if (ssc->ss_if.if_data.ifi_aros_usedhcp == 1) {
      if ((req = CreateIORequest(SanaPort, sizeof(*req))) != NULL) {
        ssc->ss_eventsent++;
        req->ioip_s2.ios2_Req.io_Device    = ssc->ss_dev;    
        req->ioip_s2.ios2_Req.io_Unit      = ssc->ss_unit;   
        req->ioip_s2.ios2_BufferManagement = ssc->ss_bufmgnt;
        req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        req->ioip_if = ssc;
        req->ioip_next = NULL;
        req->ioip_s2.ios2_Req.io_Command = S2_ONEVENT;
        req->ioip_s2.ios2_WireError = S2EVENT_OFFLINE | S2EVENT_CONNECT;
        req->ioip_dispatch = sana_connect;
        BeginIO((struct IORequest *)req);
        ssc->ss_connectreq = req;
      }
    }
  }
  splx(s);
}

/*
 * Free Sana-II IO Requests 
 * Note: this is protected by splimp();
 */
static void
sana_unrun(struct sana_softc *ssc)
{
  struct IOIPReq *req, *next;
  
  for ( next = ssc->ss_reqs; req = next ;) {
    next = req -> ioip_next;
    WaitIO((struct IORequest *)req);
    DeleteIORequest((struct IORequest *)req);
  }
  ssc->ss_reqs = next;
  
  WaitIO((struct IORequest *)ssc->ss_connectreq);
  DeleteIORequest((struct IORequest *)ssc->ss_connectreq);

  ssc->ss_if.if_flags &= ~IFF_RUNNING;
}

/*
 * Generic SANA-II interface ioctl
 *
 * Interface setup is thru IOCTL.
 */
int 
sana_ioctl(register struct ifnet *ifp, int cmd, caddr_t data)
{
  register struct sana_softc *ssc = (struct sana_softc*)ifp;
  register struct ifaddr *ifa = (struct ifaddr *)data;
  register struct ifreq *ifr = (struct ifreq *)data;
  
  spl_t s = splimp();
  int error = 0;

D(bug("[ATCP-SANA] sana_ioctl()\n"));

  switch (cmd) {

  case SIOCSIFFLAGS:
D(bug("[ATCP-SANA] sana_ioctl: SIOCSIFFLAGS - \n"));

    if (((ifr->ifr_flags & (IFF_UP|IFF_RUNNING)) == (IFF_UP|IFF_RUNNING)) && ((ssc->ss_if.if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING))
    {
D(bug("[ATCP-SANA] sana_ioctl: SIFFLAGS bringing interface up .. \n"));
      sana_up(ssc);
    }
    /* Call sana_down() in every case */
    if ((ifr->ifr_flags & IFF_UP) == 0)
    {
D(bug("[ATCP-SANA] sana_ioctl: SIFFLAGS bringing interface DOWN .. \n"));
      sana_down(ssc);
    }
    if ((ifr->ifr_flags & IFF_NOARP) == 0)
    {
D(bug("[ATCP-SANA] sana_ioctl: SIFFLAGS Allocating interface ARP tables .. \n"));
      alloc_arptable(ssc, 0);
    }
    break;

    /*
     * Set interface address (and mark interface up).
     */
  case SIOCSIFADDR:		/* Set Interface Address */
D(bug("[ATCP-SANA] sana_ioctl: SIOCSIFADDR - Set Interface Address\n"));
    if (!(ssc->ss_if.if_flags & IFF_RUNNING))
    {
D(bug("[ATCP-SANA] sana_ioctl: SIFADDR set interface as running .. \n"));
      sana_run(ssc, ssc->ss_reqno, ifa);
    }
    if ((ssc->ss_if.if_flags & IFF_RUNNING) && !(ssc->ss_if.if_flags & IFF_UP)) {
      if (ssc->ss_if.if_flags & IFF_NOUP)
      {
D(bug("[ATCP-SANA] sana_ioctl: SIFADDR Clearing interface NOUP flag .. \n"));
        ssc->ss_if.if_flags &= ~IFF_NOUP;
      }
      else
      {
D(bug("[ATCP-SANA] sana_ioctl: SIFADDR bringing interface UP .. \n"));
        sana_up(ssc);
      }
    }
    if ((ssc->ss_if.if_flags & IFF_NOARP) == 0)
    {
D(bug("[ATCP-SANA] sana_ioctl: SIFADDR Allocating interface ARP tables .. \n"));
      alloc_arptable(ssc, 0);
    }
    
  case SIOCAIFADDR:		/* Alter Interface Address */
D(bug("[ATCP-SANA] sana_ioctl: SIOCAIFADDR - Alter Interface Address\n"));
    switch (ifa->ifa_addr->sa_family) {
#if INET
    case AF_INET:
      ssc->ss_ipaddr = IA_SIN(ifa)->sin_addr;
      break;
#endif
    }
    break;

  case SIOCSIFDSTADDR:		/* Sets P-P-link destination address */
D(bug("[ATCP-SANA] sana_ioctl: SIOCSIFDSTADDR - [*] Set P-P-link destination address\n"));
    break;

  default:
D(bug("[ATCP-SANA] sana_ioctl: UNKNOWN SIOC\n"));
    error = EINVAL;
    break;
  }
  splx(s);
  return (error);
}

/*
 * sana_send_read(): 
 * send read requests with given types, dispatcher & c  
 * MUST be called at splimp()
 */
static inline WORD 
sana_send_read(struct sana_softc *ssc, WORD count, ULONG type, ULONG mtu,
	       void (*dispatch)(struct sana_softc *, struct IOIPReq *),
	       UWORD command, UBYTE flags)
{
  struct IOIPReq *req = NULL;
  WORD i;

  for (i = 0; i < count; i++) {
    if (!(req = (struct IOIPReq*)RemHead((struct List*)&ssc->ss_freereq)))
      return i;
    req->ioip_dispatch = dispatch;
    req->ioip_s2.ios2_PacketType = type;
    req->ioip_Command = command;
    req->ioip_s2.ios2_Req.io_Flags = flags;
    if (!ioip_alloc_mbuf(req, mtu))
      goto no_resources;
    BeginIO((struct IORequest*)req);
  }
  return i;

 no_resources:
  if (req)
    AddHead((struct List*)&ssc->ss_freereq, (struct Node*)req);
  __log(LOG_ERR, "sana_send_read: could not queue enough read requests\n");
  return i;
}

/*
 * Called when interface goes online
 */
static void
sana_restore(struct sana_softc *ssc)
{
  spl_t s;
  struct timeval now;

DSANA(__log(LOG_DEBUG,"sana_restore(%s%d) called", ssc->ss_if.if_name, ssc->ss_if.if_unit);)
D(bug("[ATCP-SANA] sana_restore('%s%d')\n", ssc->ss_if.if_name, ssc->ss_if.if_unit));

  s = splimp();
  ssc->ss_if.if_flags |= IFF_UP;
  GetSysTime(&now);
  ssc->ss_if.if_data.ifi_aros_ontime.tv_secs = now.tv_secs;
  ssc->ss_if.if_data.ifi_aros_ontime.tv_micro = now.tv_micro;
  ssc->ss_if.if_data.ifi_aros_lasttotal = ssc->ss_if.if_ibytes + ssc->ss_if.if_obytes;
  gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Online);
  /* Send read requests to device driver */
#if	INET
  /* IP */
  ssc->ss_ip.sent +=
    sana_send_read(ssc, ssc->ss_ip.reqno - ssc->ss_ip.sent, ssc->ss_ip.type,
		   ssc->ss_if.if_mtu, sana_ip_read, CMD_READ, 0);

  ssc->ss_arp.sent +=
    sana_send_read(ssc, ssc->ss_arp.reqno - ssc->ss_arp.sent, ssc->ss_arp.type,
		   ARP_MTU, sana_arp_read, CMD_READ, 0);

#endif /* INET */
#if	ISO
#endif /* ISO */
#if	CCITT
#endif /* CCITT */
#if	NS
#endif /* NS */
#if 0
  ssc->ss_rawsent +=
    sana_send_read(ssc, ssc->ss_rawreqno - ssc->ss_rawsent, 0,
		   ssc->ss_if.if_mtu, sana_raw_read,
		   S2_READORPHAN, SANA2_IOF_RAW);
#endif
  splx(s);
  return;
}

/*
 * sana_up():
 * send read requests
 */
static void
sana_up(struct sana_softc *ssc)
{
  struct IOSana2Req *req;
  DSANA(__log(LOG_DEBUG,"sana_up(%s%d) called", ssc->ss_if.if_name, ssc->ss_if.if_unit);)
D(bug("[ATCP-SANA] sana_up('%s%d')\n", ssc->ss_if.if_name, ssc->ss_if.if_unit));

  gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_GoingOnline);

  if (req = CreateIOSana2Req(ssc))
  {
    req->ios2_Req.io_Command = S2_ONLINE;
    req->ios2_Req.io_Error   = S2ERR_NO_ERROR;

    DoIO((struct IORequest*)req);

    if ((req->ios2_Req.io_Error) && (req->ios2_WireError != S2WERR_UNIT_ONLINE)) {
      sana2perror("S2_ONLINE", req);
      gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Offline);
    } else {
      __log(LOG_NOTICE, "%s%d is now online.", ssc->ss_name, ssc->ss_if.if_unit);
      sana_restore(ssc);
    }
    DeleteIOSana2Req(req);
  }
}

#if __SASC
/*
 * "Fix" for numerous sana2 drivers, which expect to get Unit * in the
 * register A3 when their AbortIO function is called.
 * Note that Exec AbortIO() does NOT put it there.
 */
extern VOID _AbortSanaIO(struct IORequest *, struct Unit *);
#pragma libcall DeviceBase _AbortSanaIO 24 B902

static inline __asm VOID 
AbortSanaIO(register __a1 struct IORequest *ioRequest)
{
#define DeviceBase ioRequest->io_Device
  _AbortSanaIO(ioRequest, ioRequest->io_Unit);
#undef DeviceBase
}
#else /* implement later for other compilers */
#define AbortSanaIO AbortIO
#endif

/*
 * sana_down(): Mark interface as down, abort all pending requests
 */
static BOOL
sana_down(struct sana_softc *ssc)
{
  struct IOSana2Req * sreq;
  spl_t s = splimp();
  struct IOIPReq *req = ssc->ss_reqs;
  BOOL success;

  DSANA(__log(LOG_DEBUG,"sana_down(%s%d) called", ssc->ss_if.if_name, ssc->ss_if.if_unit);)
  gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_GoingOffline);
  /* Completed, Remove()'d requests are not aborted */
  while (req) {
    if (!CheckIO((struct IORequest*)req)) {
      AbortSanaIO((struct IORequest*)req);
    }
    req = req->ioip_next;
  }
  if (ssc->ss_dev && (ssc->ss_dev->dd_Library.lib_OpenCnt == 1)) {
    if (sreq = CreateIOSana2Req(ssc)) {
        sreq->ios2_Req.io_Command = S2_OFFLINE;

        DoIO((struct IORequest*)sreq);
        if (sreq->ios2_Req.io_Error) {
	  sana2perror("S2_OFFLINE", sreq);
	  success = FALSE;
        }
	else
	  success = TRUE;
        DeleteIOSana2Req(sreq);
    }
  } else
    success = TRUE;
  if (success) {
    __log(LOG_NOTICE, "%s%d is now offline.", ssc->ss_name, ssc->ss_if.if_unit);
    gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Offline);
  } else
    gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Online);

  splx(s);

  return(TRUE);
}

/*
 * sana_read: deattach a packet from IORequest
 *            resend the IORequest
 */
static struct mbuf *
sana_read(struct sana_softc *ssc, struct IOIPReq *req, 
	  UWORD  flags, UWORD *sent, const char *banner, size_t mtu)
{
  register struct mbuf *m = req->ioip_packet;
  register spl_t s = splimp();

  req->ioip_packet = NULL;

  switch (req->ioip_Error) {
  case 0:
    if (req->ioip_s2.ios2_Req.io_Flags & SANA2IOF_BCAST) 
      m->m_flags |= M_BCAST;
    if (req->ioip_s2.ios2_Req.io_Flags & SANA2IOF_MCAST)
      m->m_flags |= M_MCAST;
    ssc->ss_if.if_ibytes += req->ioip_s2.ios2_DataLength;
    break;
  case S2ERR_OUTOFSERVICE:
    /*
     * Somebody put Sana-II driver offline.
     * We put down also the network interface 
     */
    if (ssc->ss_if.if_flags & IFF_UP) {
      /* Show a log message */
      sana2perror(ssc->ss_if.if_name, (struct IOSana2Req *)req);

      /* tell it to protocols */
      if_down((struct ifnet *)ssc); 

      /* Free mbufs allocated for packet */
      m_freem(req->ioip_reserved);
      req->ioip_reserved = NULL;

      /* Order an notify when driver is put back online */
      ssc->ss_eventsent++;
      req->ioip_s2.ios2_Req.io_Command = S2_ONEVENT;
      req->ioip_s2.ios2_WireError = S2EVENT_ONLINE;
      req->ioip_dispatch = sana_online;
      BeginIO((struct IORequest *)req);
      req = NULL;
      ssc->ss_if.if_flags &= ~IFF_UP;
      gui_set_interface_state(&ssc->ss_if, MIAMIPANELV_AddInterface_State_Offline);
    }
    m_freem(m);
    m = NULL;
    break;
  default:
    if (debug_sana && req->ioip_Error != IOERR_ABORTED) 
      sana2perror(banner, (struct IOSana2Req *)req);
    m_freem(m);
    m = NULL;
  }

  if (ssc->ss_if.if_flags & IFF_UP) {
    /* Return request to the Sana-II driver */
    if (ioip_alloc_mbuf(req, mtu)) {
      req->ioip_s2.ios2_Req.io_Flags = flags;
      BeginIO((struct IORequest*)req);
      splx(s);
      return m;
    }
    __log(LOG_ERR, "sana_read (%s): not enough mbufs\n", ssc->ss_name);
  } 

  /* do not resend, free used resources */
  (*sent)--;
  if (req) {
    m_freem(req->ioip_reserved);
    req->ioip_reserved = NULL;
    req->ioip_dispatch = NULL;
    AddHead((struct List*)&ssc->ss_freereq, (struct Node*)req);
  }

  if (m) {
    m_freem(m);
  }

  splx(s);
  return NULL;
}

/*
 * sana_ip_read(): feed a packet to the IP queue
 * (This routine is called from sana_poll)
 */
static void
sana_ip_read(struct sana_softc *ssc, struct IOIPReq *req)
{
  struct mbuf *m = sana_read(ssc, req, 0, &ssc->ss_ip.sent, "sana_ip_read",
			     ssc->ss_if.if_mtu);
  spl_t s;

  if (m) {
    s = splimp();
    if (IF_QFULL(&ipintrq)) {
      IF_DROP(&ipintrq);
      m_freem(m);
      /* m = NULL; */
    } else {
      /* Set interface pointer (needed for broadcasts) */
      m->m_pkthdr.rcvif = (struct ifnet *)ssc; 
      IF_ENQUEUE(&ipintrq, m);
      /* A signal might be needed if we use PA_EXCEPTION port */
      schednetisr_nosignal(NETISR_IP);
      /* m = NULL; */
    }
    splx(s);
  }
}

/*
 * sana_arp_read(): process an ARP packet
 * (This routine is called from sana_poll)
 */
static void
sana_arp_read(struct sana_softc *ssc, struct IOIPReq *req)
{
  struct mbuf *m; 
  UBYTE hwaddr[MAXADDRSANA];

  bcopy(req->ioip_s2.ios2_SrcAddr, hwaddr, ssc->ss_if.if_addrlen);

  m = sana_read(ssc, req, 0, &ssc->ss_arp.sent, "sana_arp_read", ARP_MTU);

  if (m)
    arpinput(ssc, m, hwaddr);
}

/*
 * sana_online(): process an ONLINE event
 */
static void
sana_online(struct sana_softc *ssc, struct IOIPReq *req)
{
  LONG events = req->ioip_s2.ios2_WireError;

  if (req->ioip_s2.ios2_Req.io_Error == 0 &&
      events & S2EVENT_ONLINE) {
    ssc->ss_eventsent--;
    req->ioip_dispatch = NULL;
    AddHead((struct List*)&ssc->ss_freereq, (struct Node*)req);
    __log(LOG_NOTICE, "%s is online again.", ssc->ss_name);
    sana_restore(ssc);
    return;
  }

  /* An error? */
  if (debug_sana && req->ioip_Error != IOERR_ABORTED) { 
    sana2perror("sana_online", (struct IOSana2Req *)req);
    req->ioip_s2.ios2_Req.io_Command = S2_ONEVENT;
    req->ioip_s2.ios2_WireError = S2EVENT_ONLINE;
    BeginIO((struct IORequest *)req);
  } else {
    /* Aborted -- probably because "ifconfig xxx/0 down" */
    ssc->ss_eventsent--;
    req->ioip_dispatch = NULL;
    AddHead((struct List*)&ssc->ss_freereq, (struct Node*)req);
  }
}

/*
 * sana_online(): process a CONNECT event
 */
static void
sana_connect(struct sana_softc *ssc, struct IOIPReq *req)
{
  LONG events = req->ioip_s2.ios2_WireError;

  if (req->ioip_s2.ios2_Req.io_Error == 0 &&
      events == S2EVENT_CONNECT) {

    /* New network -> new address */
    kill_dhclient((struct ifnet *) ssc);
    run_dhclient((struct ifnet *) ssc);
  }

  /* Send request back for next event */
  if (!(events & S2EVENT_OFFLINE)) {
    req->ioip_s2.ios2_Req.io_Command = S2_ONEVENT;
    req->ioip_s2.ios2_WireError =  S2EVENT_OFFLINE | S2EVENT_CONNECT;
    BeginIO((struct IORequest *)req);
  } else {
    ssc->ss_eventsent--;
  }
}

/*
 * sana_output: send a packet to Sana-II driver
 */
int
sana_output(struct ifnet *ifp, struct mbuf *m0, 
	    struct sockaddr *dst, struct rtentry *rt)
{
  register struct sana_softc *ssc = (struct sana_softc *)ifp;
  ULONG type;
  int error = 0;
  struct in_addr idst;

  /* If a broadcast, send a copy to ourself too */
  struct mbuf *mcopy = (struct mbuf *)NULL;
  struct IOIPReq *req = NULL;
  register struct mbuf *m = m0;

  int len = m->m_pkthdr.len;
  spl_t s = splimp();

  ifp->if_opackets++;		/* stats */

  /* Check if we are up and running... */
  if ((ssc->ss_if.if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
    error = ENETDOWN;
    goto bad;
  }

  GetSysTime(&ssc->ss_if.if_lastchange);

  /* Get a free Sana-II IO request */
  if (!(req = (struct IOIPReq*)RemHead((struct List*)&ssc->ss_freereq))) {
    error = ENOBUFS;
    goto bad;
  }

  req->ioip_s2.ios2_Req.io_Flags = 0;

  switch (dst->sa_family) {
#if INET
  case AF_INET:
    idst = ((struct sockaddr_in *)dst)->sin_addr;

    /* If the address is not resolved, arpresolve
     * stores the packet to its private queue for
     * later transmit and broadcasts the resolve
     * request packet to the (ether)net.
     * (Now ARP works only with IP and ethernet.)
     */
    if ((ssc->ss_if.if_flags & IFF_NOARP) != IFF_NOARP &&
	/* ssc = network interface 
	 * m = Packet to send 
	 * idst = destination IP address 
	 * ios2_DestAddr = destination hw address 
	 * error = error return
	 */
	!arpresolve(ssc, m, &idst, req->ioip_s2.ios2_DstAddr, &error)) {
      AddHead((struct List*)&ssc->ss_freereq, (struct Node*)req);
      splx(s);
      return (0);
    }
    type = ssc->ss_ip.type;

    /* Send to loopback if we do not hear our broadcasts */
    if ((ssc->ss_if.if_flags & IFF_SIMPLEX) && (m->m_flags & M_BCAST)) {
      mcopy = m_copy(m, 0, (int)M_COPYALL);
      (void) looutput(&ssc->ss_if, mcopy, dst, rt);
    }
    /* Set the message priority */
    req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Pri =
      (IPTOS_LOWDELAY & mtod(m, struct ip *)->ip_tos) ?
	1 : 0;
    break;
#endif
#if NS
#error NS unimplemented!!!
  case AF_NS:
    type = ssc->ss_nstype;
    /* There is hardware address straight in socket */
    /* Dunno how this works, if we have a P-to-P device */
    bcopy((caddr_t)&(((struct sockaddr_ns *)dst)->sns_addr.x_host),
	  (caddr_t)req->ioip_s2.ios2_DestAddr, ssc->ss_if.if_addrlen);
    /* Local send */
    if (!bcmp((caddr_t)req->ioip_s2.ios2_DestAddr,
	      (caddr_t)&ns_thishost, ssc->ss_if.if_addrlen)) {
      AddHead(&ssc->ss_freereq, req);
      return (looutput(ifp, m, dst, rt));
    }
    req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Pri = 0;
    break;
#endif
  case AF_UNSPEC:
    /* Raw packets. Sana-II address (a tuple of type and host)
     * specifies the destination 
     */
    if (type = ((struct sockaddr_sana2*)dst)->ss2_type) {
      bcopy(((struct sockaddr_sana2*)dst)->ss2_host, 
	    req->ioip_s2.ios2_DstAddr, 
	    ssc->ss_if.if_addrlen);
    } else {
      req->ioip_s2.ios2_Req.io_Flags = SANA2IOF_RAW;
      type = 0L;
    }
    req->ioip_s2.ios2_Req.io_Message.mn_Node.ln_Pri = 0;
    break;

#if	ISO
#endif /* ISO */
#if RMP
  case AF_RMP:
#endif

  default: 
    __log(LOG_ERR, "%s%ld: can't handle af%ld\n",
	ssc->ss_if.if_name, ssc->ss_if.if_unit, dst->sa_family);
    error = EAFNOSUPPORT; 
    goto bad; 
  }

  /*
   * Queue packet to Sana-II driver
   */
  req->ioip_Command = (m->m_flags & M_BCAST) ? S2_BROADCAST : CMD_WRITE;
  req->ioip_dispatch = free_written_packet;
  req->ioip_packet = m;
  req->ioip_s2.ios2_PacketType = type;
  req->ioip_s2.ios2_DataLength = len;

  BeginIO((struct IORequest*)req);

  /* These statistics are somewhat redundant */
  ifp->if_obytes += len;
  if (m->m_flags & M_BCAST)
    ifp->if_omcasts++;

  splx(s);
  return 0;

 bad:
  if (req)
    AddHead((struct List*)&ssc->ss_freereq, (struct Node*)req);
  ifp->if_oerrors++;

  splx(s);
  if (m)
    m_freem(m);
  return error;
}

/*
 * free_written_packet(): free mbufs of written packet,
 *                        queue IOrequest for reuse
 * (This routine is called from sana_poll)
 */
static void
free_written_packet(struct sana_softc *ssc, struct IOIPReq *req)
{
  spl_t s = splimp();

  if (req->ioip_packet) {
    m_freem(req->ioip_packet);
    req->ioip_packet = NULL;
  }
  req->ioip_dispatch = NULL;
  if (debug_sana && req->ioip_Error)
    sana2perror("sana_output", (struct IOSana2Req *)req);
  AddHead((struct List*)&ssc->ss_freereq, (struct Node*)req);
  splx(s);
}

/*
 * SANA-II-dependent part of QueryInterfaceTagList()
 */
static int sana_query(struct ifnet *ifn, struct TagItem *tag)
{
	struct sana_softc *ssc = (struct sana_softc *)ifn;

	switch (tag->ti_Tag)
	{
	case IFQ_DeviceName:
		*((STRPTR *)tag->ti_Data) = ssc->ss_execname;
		break;
	case IFQ_DeviceUnit:
		*((ULONG *)tag->ti_Data) = ssc->ss_execunit;
		break;
	case IFQ_HardwareAddress: /* Temporary, we should extract it from if_addrlist instead */
		memcpy((void *)tag->ti_Data, ssc->ss_hwaddr, ssc->ss_if.if_addrlen);
		break;
	case IFQ_HardwareType:
		*((ULONG *)tag->ti_Data) = ssc->ss_hwtype;
		break;
        case IFQ_NumReadRequests:
		__log(LOG_CRIT, "IFQ_NumReadRequests is not implemented");
		return -1;
/*		*((LONG *)tag->ti_Data) = *** TODO ***
		break;*/
	case IFQ_MaxReadRequests:
		__log(LOG_CRIT, "IFQ_MaxReadRequests is not implemented");
		return -1;
/*		*((LONG *)tag->ti_Data) =
		break;*/
	case IFQ_NumWriteRequests:
		__log(LOG_CRIT, "IFQ_NumWriteRequests is not implemented");
		return -1;
/*		*((LONG *)tag->ti_Data) =
		break;*/
	case IFQ_MaxWriteRequests:
		__log(LOG_CRIT, "IFQ_MaxWriteRequests is not implemented");
		return -1;
/*		*((LONG *)tag->ti_Data) =
		break;*/
	case IFQ_GetDebugMode:
		*((LONG *)tag->ti_Data) = debug_sana;
		break;
  	case IFQ_GetSANA2CopyStats:
		__log(LOG_CRIT, "IFQ_GetSANA2CopyStats is not implemented");
		return -1;
/*	        (struct Sana2CopyStats *)tag->ti_Data
		break;*/
	case IFQ_NumReadRequestsPending:
		__log(LOG_CRIT, "IFQ_NumReadRequestsPending is not implemented");
		return -1;
/*		*((LONG *)tag->ti_Data =
		break;*/
	case IFQ_NumWriteRequestsPending:
		__log(LOG_CRIT, "IFQ_NumWriteRequestsPending is not implemented");
		return -1;
/*		*((LONG *)tag->ti_Data =
		break;*/
	default:
		return -1;
	}
	return 0;
}

