/*
 *			P I N G . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 *	Pavel Fedin
 *	September, 2005
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 * Bugs -
 *	More statistics could always be gathered.
 */

/****** netutil.doc/ping ****************************************************

    NAME
        ping - send ICMP ECHO_REQUEST packets to network hosts

    SYNOPSIS
        ping [-dfnqrvR] [-c count] [-i wait] [-l preload] [-p pattern]
             [-s packetsize] [-L [ hosts ]] host

    DESCRIPTION
        Ping uses the ICMP protocol's mandatory ECHO_REQUEST datagram to
        elicit an ICMP ECHO_RESPONSE from a host or gateway.  ECHO_REQUEST
        datagrams (``pings'') have an IP and ICMP header, followed by a
        ``struct timeval'' and then an arbitrary number of ``pad'' bytes
        used to fill out the packet.  The options are as follows: Other
        options are:

        -c count
                Stop after sending (and receiving) count ECHO_RESPONSE
                packets.

        -d      Set the SO_DEBUG option on the socket being used.

        -f      Flood ping.  Outputs packets as fast as they come back or
                one hundred times per second, whichever is more.  For every
                ECHO_REQUEST sent a period ``.'' is printed, while for ever
                ECHO_REPLY received a backspace is printed.  This provides a
                rapid display of how many packets are being dropped.  Only
                the super-user may use this option.  This can be very hard
                on a network and should be used with caution.

        -i wait
                Wait wait seconds between sending each packet. The default
                is to wait for one second between each packet.  This option
                is incompatible with the -f option.

        -L [hosts]
	        Use loose routing IP option.  Includes IPOPT_LSRR option in
                the ECHO_REQUEST packet with all specified hosts in the
                route.  Many hosts wont support loose routing, such a host
                can either ignore or return the loose routed ICMP packet in
                the middle of the route.

        -l preload
                If preload is specified, ping sends that many packets as
                fast as possible before falling into its normal mode of
                behavior.

        -n      Numeric output only.  No attempt will be made to lookup
                symbolic names for host addresses.

        -p pattern
                You may specify up to 16 ``pad'' bytes to fill out the
                packet you send.  This is useful for diagnosing
                data-dependent problems in a network.  For example, ``-p
                ff'' will cause the sent packet to be filled with all ones.

        -q      Quiet output.  Nothing is displayed except the summary lines
                at startup time and when finished.

        -R      Record route.  Includes the RECORD_ROUTE option in the
                ECHO_REQUEST packet and displays the route buffer on
                returned packets.  Note that the IP header is only large
                enough for nine such routes.  Many hosts ignore or discard
                this option.

        -r      Bypass the normal routing tables and send directly to a host
                on an attached network.  If the host is not on a
                directly-attached network, an error is returned.  This
                option can be used to ping a local host through an interface
                that has no route through it.

        -s packetsize
                Specifies the number of data bytes to be sent.  The default
                is 56, which translates into 64 ICMP data bytes when
                combined with the 8 bytes of ICMP header data.

        -v      Verbose output.  ICMP packets other than ECHO_RESPONSE that
                are received are listed.

        When using ping for fault isolation, it should first be run on the
        local host, to verify that the local network interface is up and
        running.  Then, hosts and gateways further and further away should
        be ``pinged''.  Round-trip times and packet loss statistics are
        computed.  If duplicate packets are received, they are not included
        in the packet loss calculation, although the round trip time of
        these packets is used in calculating the minimum/average/maximum
        round-trip time numbers.  When the specified number of packets have
        been sent (and received) or if the program is terminated with a
        SIGINT, a brief summary is displayed.

        This program is intended for use in network testing, measurement and
        management.  Because of the load it can impose on the network, it is
        unwise to use ping during normal operations or from automated
        scripts.

    ICMP PACKET DETAILS
        An IP header without options is 20 bytes.  An ICMP ECHO_REQUEST
        packet contains an additional 8 bytes worth of ICMP header followed
        by an arbitrary amount of data.  When a packetsize is given, this
        indicated the size of this extra piece of data (the default is 56).
        Thus the amount of data received inside of an IP packet of type ICMP
        ECHO_REPLY will always be 8 bytes more than the requested data space
        (the ICMP header).

        If the data space is at least eight bytes large, ping uses the first
        eight bytes of this space to include a timestamp which it uses in
        the computation of round trip times.  If less than eight bytes of
        pad are specified, no round trip times are given.

    DUPLICATE AND DAMAGED PACKETS
        Ping will report duplicate and damaged packets.  Duplicate packets
        should never occur, and seem to be caused by inappropriate
        link-level retransmissions.  Duplicates may occur in many situations
        and are rarely (if ever) a good sign, although the presence of low
        levels of duplicates may not always be cause for alarm.

        Damaged packets are obviously serious cause for alarm and often
        indicate broken hardware somewhere in the ping packet's path (in the
        network or in the hosts).

    TRYING DIFFERENT DATA PATTERNS
        The (inter)network layer should never treat packets differently
        depending on the data contained in the data portion.  Unfortunately,
        data-dependent problems have been known to sneak into networks and
        remain undetected for long periods of time.  In many cases the
        particular pattern that will have problems is something that doesn't
        have sufficient ``transitions'', such as all ones or all zeros, or a
        pattern right at the edge, such as almost all zeros.  It isn't
        necessarily enough to specify a data pattern of all zeros (for
        example) on the command line because the pattern that is of interest
        is at the data link level, and the relationship between what you
        type and what the controllers transmit can be complicated.

        This means that if you have a data-dependent problem you will
        probably have to do a lot of testing to find it.  If you are lucky,
        you may manage to find a file that either can't be sent across your
        network or that takes much longer to transfer than other similar
        length files.  You can then examine this file for repeated patterns
        that you can test using the -p option of ping.

    TTL DETAILS
        The TTL value of an IP packet represents the maximum number of IP
        routers that the packet can go through before being thrown away.  In
        current practice you can expect each router in the Internet to
        decrement the TTL field by exactly one.

        The TCP/IP specification states that the TTL field for TCP packets
        should be set to 60, but many systems use smaller values (4.3 BSD
        uses 30, 4.2 used 15). AROSTCP normally uses TTL value 30.

        The maximum possible value of this field is 255, and most systems
        set the TTL field of ICMP ECHO_REQUEST packets to 255.  This is why
        you will find you can ``ping'' some hosts, but not reach them with
        telnet or ftp.

        In normal operation ping prints the ttl value from the packet it re-
        ceives.  When a remote system receives a ping packet, it can do one
        of three things with the TTL field in its response:

        ·   Not change it; this is what Berkeley Unix systems did before the
            4.3BSD-Tahoe release.  In this case the TTL value in the
            received packet will be 255 minus the number of routers in the
            round-trip path.

        ·   Set it to 255; this is what AROSTCP and current (as of 1994)
            Berkeley Unix systems do.  In this case the TTL value in the
            received packet will be 255 minus the number of routers in the
            path from the remote system to the pinging host.

        ·   Set it to some other value.  Some machines use the same value
            for ICMP packets that they use for TCP packets, for example
            either 30 or 60.  Others may use completely wild values.

    LOOSE SOURCE ROUTING DETAILS 
        When a packet is routed with loose routing in IP, the destination
        address of datagram is originally set to the first address in the
        routing list.  When the datagram reaches its destination, the
        destination address is changed to the next address in the list and
        the datagram is routed to that destination. After the whole routing
        list is exhausted, the datagram is handled to upper-level protocols.

        The loose routing options can be ignored by hosts between the
        gateways in the loose routing list.  However, if the host in the
        list don't understand loose routing, it may think that the datagram
        is destined to it and respond to it.  Also, many hosts simply drop
        the packets with IP options.

    BUGS
        Many Hosts and Gateways ignore the RECORD_ROUTE and
        LOOSE_SOURCE_ROUTING options.

        The maximum IP header length is too small for options like
        RECORD_ROUTE to be completely useful.  There's not much that that
        can be done about this, however.

        Flood pinging is not recommended in general, and flood pinging the
        broadcast address should only be done under very controlled
        conditions.

    SEE ALSO
        netstat,  ifconfig

    AUTHOR
        Mike Muuss, U. S. Army Ballistic Research Laboratory, December, 1983

        The ping command appeared in 4.3BSD.

        The loose routing and working record route options were added by
        Pekka Pessi, AmiTCP/IP Group, Helsinki Univ. of Technology.

*****************************************************************************
* */

#ifndef AMIGA
#define AMIGA
#endif

#include <aros/config.h>

#include <proto/socket.h>

#define ioctl IoctlSocket

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>

#define	DEFDATALEN	(64 - 8)	/* default data length */
#define	MAXIPLEN	60
#define	MAXICMPLEN	76
#define	MAXPACKET	(65536 - 60 - 8)/* max packet size */
#define	MAXWAIT		10		/* max seconds to wait for response */
#define	NROUTES		9		/* number of record route slots */

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

/* various options */
int options;
#define	F_FLOOD		0x001
#define	F_INTERVAL	0x002
#define	F_NUMERIC	0x004
#define	F_PINGFILLED	0x008
#define	F_QUIET		0x010
#define	F_RROUTE	0x020
#define	F_SO_DEBUG	0x040
#define	F_SO_DONTROUTE	0x080
#define	F_VERBOSE	0x100
#define F_LOOSEROUTE    0x200

#ifndef LONG_MAX
#define LONG_MAX 0xffffffff
#endif

/*
 * MAX_DUP_CHK is the number of bits in received table, i.e. the maximum
 * number of received sequence numbers we can keep track of.  Change 128
 * to 8192 for complete accuracy...
 */
#define	MAX_DUP_CHK	(8 * 128)
int mx_dup_ck = MAX_DUP_CHK;
char rcvd_tbl[MAX_DUP_CHK / 8];

struct sockaddr whereto;	/* who to ping */
int datalen = DEFDATALEN;
int s = -1;			/* socket file descriptor */
u_char *outpack;
char BSPACE = '\b';		/* characters written for flood */
char DOT = '.';
char *hostname;
long ident;			/* process id to identify our packets */

/* counters */
long npackets;			/* max packets to transmit */
long nreceived;			/* # of packets we got back */
long nrepeats;			/* number of duplicates */
long ntransmitted;		/* sequence # for outbound packets = #sent */
int interval = 1;		/* interval between packets */

/* timing */
int timing;			/* flag to do timing */
unsigned long tmin = LONG_MAX;		/* minimum round trip time */
unsigned long tmax;			/* maximum round trip time */
unsigned long tsum;			/* sum of all times, for doing average */

char *pr_addr(u_long l);
void catcher(void), pinger(void), finish(void), usage(void);
void pr_pack(char *buf,	int cc,	struct sockaddr_in *from);
void pr_icmph(struct icmp *icp);
void pr_iph(struct ip *ip);
void pr_retip(struct ip *ip);
void fill(char *bp, char *patp);
int in_cksum(u_short *addr, int len);
void tvsub(register struct timeval *out, register struct timeval *in);
VOID CleanUpExit(LONG error);

#ifdef AMIGA
/*
 * Other Amiga dependent stuff
 */
#ifdef __SASC
#include <clib/exec_protos.h>
#include <pragmas/exec_sysbase_pragmas.h>
extern struct ExecBase *SysBase;
#endif
/* Disable ^C signaling */
void __chkabort(void) {}

#include <dos/dos.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/timer.h>

struct MsgPort *timerport = NULL;
struct timerequest *timermsg = NULL;
BOOL notopen = TRUE;
#define TimerBase (timermsg->tr_node.io_Device)

#define SOCKET_VERSION 3
struct Library *SocketBase;
const TEXT version[] = "$VER: ping 3.10 (14.10.2005)";
const TEXT socket_name[] = "bsdsocket.library";

void
clean_timer(void)
{
  if (timermsg) {
    if (!notopen) {
      if (!CheckIO((struct IORequest*)timermsg)) {
	AbortIO((struct IORequest*)timermsg);
	WaitIO((struct IORequest*)timermsg);
      }
      CloseDevice((struct IORequest*)timermsg);
      notopen = TRUE;
    }
    DeleteIORequest((APTR)timermsg);
    timermsg = NULL;
  }
  if (timerport) {
    DeleteMsgPort(timerport);
    timerport = NULL;
  }
}
#endif

int main(argc, argv)
	int argc;
	char **argv;
{
  extern int errno, optind;
  extern char *optarg;
  struct timeval timeout;
  struct hostent *hp;
  struct sockaddr_in *to;
  struct protoent *proto;
  register int i;
  int ch, fdmask, hold, packlen, preload;
  u_char *datap, *packet;
  char *target, hnamebuf[MAXHOSTNAMELEN];
#ifdef IP_OPTIONS
  u_char rspace[3 + 4 * NROUTES + 1]; /* record route space */
#endif
#ifdef AMIGA
  ULONG timermask;
#endif

  SocketBase = OpenLibrary(socket_name, SOCKET_VERSION);
  if(SocketBase == NULL) {
    fprintf(stderr, "ping: cannot open bsdsocket.library version 3.\n");
    return RETURN_FAIL;
  }
  SetErrnoPtr(&errno, sizeof(errno));

  outpack = malloc(MAXPACKET);
  if (outpack == NULL) {
    perror("ping");
    CleanUpExit(1);
  }
  preload = 0;
  datap = &outpack[8 + sizeof(struct timeval)];
  while ((ch = getopt(argc, argv, "LRc:dfh:i:l:np:qrs:v")) != EOF)
    switch(ch) {
    case 'c':
      npackets = atoi(optarg);
      if (npackets <= 0) {
	(void)fprintf(stderr, "ping: bad number of packets to transmit.\n");
	CleanUpExit(1);
      }
      break;
    case 'd':
      options |= F_SO_DEBUG;
      break;
    case 'f':
#ifndef AMIGA
      if (getuid()) {
	(void)fprintf(stderr, "ping: %s\n", strerror(EPERM));
	CleanUpExit(1);
      }
#endif
      options |= F_FLOOD;
      setbuf(stdout, (char *)NULL);
      break;
    case 'i':			/* wait between sending packets */
      interval = atoi(optarg);
      if (interval <= 0) {
	(void)fprintf(stderr, "ping: bad timing interval.\n");
	CleanUpExit(1);
      }
      options |= F_INTERVAL;
      break;
    case 'L':
      options |= F_LOOSEROUTE;
      break;
    case 'l':
      preload = atoi(optarg);
      if (preload < 0) {
	(void)fprintf(stderr, "ping: bad preload value.\n");
	CleanUpExit(1);
      }
      break;
    case 'n':
      options |= F_NUMERIC;
      break;
    case 'p':			/* fill buffer with user pattern */
      options |= F_PINGFILLED;
      fill((char *)datap, optarg);
      break;
    case 'q':
      options |= F_QUIET;
      break;
    case 'R':
      options |= F_RROUTE;
      break;
    case 'r':
      options |= F_SO_DONTROUTE;
      break;
    case 's':			/* size of packet to send */
      datalen = atoi(optarg);
      if (datalen > MAXPACKET) {
	(void)fprintf(stderr, "ping: packet size too large.\n");
	CleanUpExit(1);
      }
      if (datalen <= 0) {
	(void)fprintf(stderr, "ping: illegal packet size.\n");
	CleanUpExit(1);
      }
      break;
    case 'v':
      options |= F_VERBOSE;
      break;
    default:
      usage();
    }
  argc -= optind;
  argv += optind;

  if (argc < 1 || (options & F_LOOSEROUTE) == 0 && argc > 1)
    usage();

  if ((options & (F_LOOSEROUTE | F_RROUTE)) == (F_LOOSEROUTE | F_RROUTE)) {
    fprintf(stderr, "ping: -L and -R options cannot be used concurrently\n");
    CleanUpExit(1);
  }

  {
    u_char *cp = rspace;

    if (options & F_LOOSEROUTE) {
#ifdef IP_OPTIONS
      rspace[IPOPT_OPTVAL] = IPOPT_LSRR;
      rspace[IPOPT_OLEN] = 3;
      rspace[IPOPT_OFFSET] = IPOPT_MINOFF;
      cp = rspace + IPOPT_OFFSET + 1;
#else
      (void)fprintf(stderr, "ping: source routing not "
		    "available in this implementation.\n");
      CleanUpExit(1);
#endif				/* IP_OPTIONS */
    }

    while (target = *argv++) {
      bzero((char *)&whereto, sizeof(struct sockaddr));
      to = (struct sockaddr_in *)&whereto;
#ifdef _SOCKADDR_LEN
      to->sin_len = sizeof(*to);
#endif
      to->sin_family = AF_INET;
      to->sin_addr.s_addr = inet_addr(target);
      if (to->sin_addr.s_addr != (u_int)-1)
	hostname = target;
      else {
	hp = gethostbyname(target);
	if (!hp) {
	  fprintf(stderr, "ping: unknown host %s\n", target);
	  CleanUpExit(1);
	}
	to->sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
	(void)strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
	hostname = hnamebuf;
      }

#ifdef IP_OPTIONS
      if (options & F_LOOSEROUTE) {
	if (rspace + sizeof(rspace) - 4 < cp) {
	  fprintf(stderr, "ping: too many hops for "
		  "source routing %s\n", target);
	  CleanUpExit(1);
	}
	*cp++ = (to->sin_addr.s_addr >> 24);
	*cp++ = (to->sin_addr.s_addr >> 16);
	*cp++ = (to->sin_addr.s_addr >> 8);
	*cp++ = (to->sin_addr.s_addr >> 0);
	rspace[IPOPT_OLEN] += 4;
      }
#endif
    }
  }

  if (options & F_FLOOD && options & F_INTERVAL) {
    (void)fprintf(stderr, "ping: -f and -i incompatible options.\n");
    CleanUpExit(1);
  }

  if (datalen >= sizeof(struct timeval)) /* can we time transfer */
    timing = 1;
  packlen = datalen + MAXIPLEN + MAXICMPLEN;
  if (!(packet = (u_char *)malloc((u_int)packlen))) {
    (void)fprintf(stderr, "ping: out of memory.\n");
    CleanUpExit(1);
  }
  if (!(options & F_PINGFILLED))
    for (i = 8; i < datalen; ++i)
      *datap++ = i;

  ident = getpid() & 0xFFFF;

  if (!(proto = getprotobyname("icmp"))) {
    (void)fprintf(stderr, "ping: unknown protocol icmp.\n");
    CleanUpExit(1);
  }

#ifdef AMIGA
  atexit(clean_timer);

  timerport = CreateMsgPort();
  if (!timerport) {
    (void)fprintf(stderr, "ping: could not create timer port.\n");
    CleanUpExit(1);
  }
  timermask = 1<<timerport->mp_SigBit;

  timermsg = (APTR)CreateIORequest(timerport, sizeof(*timermsg));
  if (!timermsg) {
    (void)fprintf(stderr, "ping: could not create timer message.\n");
    CleanUpExit(1);
  }

  if (notopen = OpenDevice("timer.device",
#if (AROS_FLAVOUR & AROS_FLAVOUR_EMULATION)
            // UNIT_MICROHZ isn't available on AROS/hosted
            UNIT_VBLANK,
#else
            UNIT_MICROHZ,
#endif
		 (struct IORequest *)timermsg, 0)) {
    (void)fprintf(stderr, "ping: could not open timer device.\n");
    CleanUpExit(1);
  }

  timermsg->tr_node.io_Command = TR_ADDREQUEST;
  timermsg->tr_time.tv_secs = 1L;
  timermsg->tr_time.tv_micro = 0L;
  /* don't confuse CheckIO */
  timermsg->tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
  SetSocketSignals(timermask | SIGBREAKF_CTRL_C, 0L, 0L);
#endif

  if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
    perror("ping: socket");
    CleanUpExit(1);
  }
  hold = 1;
  if (options & F_SO_DEBUG)
    (void)setsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&hold,
		     sizeof(hold));
  if (options & F_SO_DONTROUTE)
    (void)setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (char *)&hold,
		     sizeof(hold));

#ifdef IP_OPTIONS
  if (options & F_LOOSEROUTE) {
    /* pad to long word */
    rspace[rspace[IPOPT_OLEN]] = IPOPT_EOL;
    if (setsockopt(s, IPPROTO_IP, IP_OPTIONS, rspace,
		   rspace[IPOPT_OLEN] + 1) < 0) {
      perror("ping: source routing");
      CleanUpExit(1);
    }
  }
#endif

  /* record route option */
  if (options & F_RROUTE) {
#ifdef IP_OPTIONS
    rspace[IPOPT_OPTVAL] = IPOPT_RR;
    rspace[IPOPT_OLEN] = sizeof(rspace)-1;
    rspace[IPOPT_OFFSET] = IPOPT_MINOFF;
    rspace[rspace[IPOPT_OLEN]] = IPOPT_EOL;
    if (setsockopt(s, IPPROTO_IP, IP_OPTIONS, rspace,
		   sizeof(rspace)) < 0) {
      perror("ping: record route");
      CleanUpExit(1);
    }
#else
    (void)fprintf(stderr,
		  "ping: record route not available in this implementation.\n");
    CleanUpExit(1);
#endif				/* IP_OPTIONS */
  }

  /*
   * When pinging the broadcast address, you can get a lot of answers.
   * Doing something so evil is useful if you are trying to stress the
   * ethernet, or just want to fill the arp cache to get some stuff for
   * /etc/ethers.
   */
  hold = 48 * 1024;
  (void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&hold,
		   sizeof(hold));

  if (to->sin_family == AF_INET)
    (void)printf("PING %s (%s): %d data bytes\n", hostname,
		 inet_ntoa(*(struct in_addr *)&to->sin_addr.s_addr),
		 datalen);
  else
    (void)printf("PING %s: %d data bytes\n", hostname, datalen);

#ifndef AMIGA
  (void)signal(SIGINT, finish);
  (void)signal(SIGALRM, catcher);
#endif

  while (preload--)		/* fire off them quickies */
    pinger();

  if ((options & F_FLOOD) == 0)
    catcher();			/* start things going */

  for (;;) {
    struct sockaddr_in from;
    register int cc;
    int fromlen;

#ifdef AMIGA
    /* Check for special signals */
    ULONG sm = SetSignal(0L, timermask | SIGBREAKF_CTRL_C);
    if (sm & SIGBREAKF_CTRL_C)
      finish();
    if (sm & timermask && GetMsg(timerport))
      catcher();
#endif

    if (options & F_FLOOD) {
      pinger();
      timeout.tv_sec = 0;
      timeout.tv_usec = 10000;
      fdmask = 1 << s;
      if (select(s + 1, (fd_set *)&fdmask, (fd_set *)NULL,
		 (fd_set *)NULL, &timeout) < 1)
	continue;
    }
    fromlen = sizeof(from);
    if ((cc = recvfrom(s, (char *)packet, packlen, 0,
		       (struct sockaddr *)&from, &fromlen)) < 0) {
      if (errno == EINTR)
	continue;
      perror("ping: recvfrom");
      continue;
    }
    pr_pack((char *)packet, cc, &from);
    if (npackets && nreceived >= npackets)
      break;
  }
  finish();
  /* NOTREACHED */
  return 0;
}

/*
 * catcher --
 *	This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 *
 * bug --
 *	Our sense of time will slowly skew (i.e., packets will not be
 * launched exactly at 1-second intervals).  This does not affect the
 * quality of the delay and loss statistics.
 *
 * notes --
 *      This routine uses timer.device in Amiga implementation instead
 * of SIGALRM.
 */
void
catcher()
{
#ifdef AMIGA
  static int waittime = 0;

  if (waittime)
    finish();

  pinger();

  if (!npackets || ntransmitted < npackets) {
    timermsg->tr_time.tv_sec = interval;
    SendIO((struct IORequest*)timermsg);
  } else {
    if (nreceived) {
      waittime = 2 * tmax / 1000;
      if (!waittime)
	waittime = 1;
    } else
      waittime = MAXWAIT;
    timermsg->tr_time.tv_sec = waittime;
    SendIO((struct IORequest*)timermsg);
  }
#else
	int waittime;

	pinger();
	(void)signal(SIGALRM, catcher);
	if (!npackets || ntransmitted < npackets)
		alarm((u_int)interval);
	else {
		if (nreceived) {
			waittime = 2 * tmax / 1000;
			if (!waittime)
				waittime = 1;
		} else
			waittime = MAXWAIT;
		(void)signal(SIGALRM, finish);
		(void)alarm((u_int)waittime);
	}
#endif
}

/*
 * pinger --
 * 	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in native
 * byte-order, to compute the round-trip time.
 */
void
pinger()
{
	register struct icmp *icp;
	register int cc;
	int i;

	icp = (struct icmp *)outpack;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = htons(ntransmitted++);
	icp->icmp_id = ident;			/* ID */

	CLR(icp->icmp_seq % mx_dup_ck);

	if (timing)
#ifdef AMIGA
/*		(void)ReadEClock((struct EClockVal *)&outpack[8]);*/ /* NC */
		(void)GetSysTime((struct timeval *)&outpack[8]);
#else
		(void)gettimeofday((struct timeval *)&outpack[8],
		    (struct timezone *)NULL);
#endif
	cc = datalen + 8;			/* skips ICMP portion */

	/* compute ICMP checksum here */
	icp->icmp_cksum = in_cksum((u_short *)icp, cc);

	i = sendto(s, (char *)outpack, cc, 0, &whereto,
	    sizeof(struct sockaddr));

	if (i < 0 || i != cc)  {
		if (i < 0)
			perror("ping: sendto");
		(void)printf("ping: wrote %s %d chars, ret=%d\n",
		    hostname, cc, i);
	}
	if (!(options & F_QUIET) && options & F_FLOOD)
		(void)write(1, &DOT, 1);
}

/*
 * pr_pack --
 *	Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
void
pr_pack(buf, cc, from)
	char *buf;
	int cc;
	struct sockaddr_in *from;
{
	register struct icmp *icp;
	register u_long l;
	register int i, j;
	register u_char *cp,*dp;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];
	struct ip *ip;
	struct timeval tv, *tp;
	long triptime;
	int hlen, dupflag;

#ifdef AMIGA
/*	ULONG efreq = ReadEClock((struct EClockVal *)&tv);*/ /* NC */
	GetSysTime((struct timeval *)&tv);
#else
	(void)gettimeofday(&tv, (struct timezone *)NULL);
#endif
	/* Check the IP header */
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		if (options & F_VERBOSE)
			(void)fprintf(stderr,
			  "ping: packet too short (%d bytes) from %s\n", cc,
			  inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr));
		return;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == ICMP_ECHOREPLY) {
		if (icp->icmp_id != ident)
			return;			/* 'Twas not our ECHO */
		++nreceived;
		if (timing) {
#ifndef icmp_data
			tp = (struct timeval *)&icp->icmp_ip;
#else
			tp = (struct timeval *)icp->icmp_data;
#endif
#ifndef AMIGA /* NC */
			/* EClockVal is actually an unsigned long long */
			if (tv.tv_micro < tp->tv_micro)
			  tv.tv_sec--;
			tv.tv_micro -= tp->tv_micro;
			tv.tv_secs  -= tp->tv_secs;
			triptime = tv.tv_micro / (efreq / 1000);
			if (tv.tv_secs)
			  triptime += tv.tv_sec * 250 * ((1<<30) / efreq);
#else
			tvsub(&tv, tp);
			triptime = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
#endif
			tsum += triptime;
			if (triptime < tmin)
				tmin = triptime;
			if (triptime > tmax)
				tmax = triptime;
		}

		if (TST(icp->icmp_seq % mx_dup_ck)) {
			++nrepeats;
			--nreceived;
			dupflag = 1;
		} else {
			SET(icp->icmp_seq % mx_dup_ck);
			dupflag = 0;
		}

		if (options & F_QUIET)
			return;

		if (options & F_FLOOD)
			(void)write(1, &BSPACE, 1);
		else {
			(void)printf("%d bytes from %s: icmp_seq=%u", cc,
			   inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr),
			   ntohs(icp->icmp_seq));
			(void)printf(" ttl=%d", ip->ip_ttl);
			if (timing)
				(void)printf(" time=%ld ms", triptime);
			if (dupflag)
				(void)printf(" (DUP!)");
			/* check the data */
			cp = (u_char*)&icp->icmp_data[8];
			dp = &outpack[8 + sizeof(struct timeval)];
			for (i = 8; i < datalen; ++i, ++cp, ++dp) {
				if (*cp != *dp) {
	(void)printf("\nwrong data byte #%d should be 0x%x but was 0x%x",
	    i, *dp, *cp);
					cp = (u_char*)&icp->icmp_data[0];
					for (i = 8; i < datalen; ++i, ++cp) {
						if ((i % 32) == 8)
							(void)printf("\n\t");
						(void)printf("%x ", *cp);
					}
					break;
				}
			}
		}
	} else {
		/* We've got something other than an ECHOREPLY */
		if (!(options & F_VERBOSE))
			return;
		(void)printf("%d bytes from %s: ", cc,
		    pr_addr(from->sin_addr.s_addr));
		pr_icmph(icp);
	}

	/* Display any IP options */
	/* The LSRR and RR len handling was broken (?) //ppessi */
	cp = (u_char *)buf + sizeof(struct ip);

	for (; hlen > (int)sizeof(struct ip); --hlen, ++cp)
		switch (*cp) {
		case IPOPT_EOL:
			hlen = 0;
			break;
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			(void)printf(*cp == IPOPT_LSRR ?
				     "\nLSRR: " : "\nSSRR: ");
			j = *++cp;
			++cp;
			hlen -= j;
			if (j > IPOPT_MINOFF)
				for (;;) {
					l = *++cp;
					l = (l<<8) + *++cp;
					l = (l<<8) + *++cp;
					l = (l<<8) + *++cp;
					if (l == 0)
						(void)printf("\t0.0.0.0");
					else
						(void)printf("\t%s", pr_addr(ntohl(l)));
					j -= 4;
					if (j < IPOPT_MINOFF)
						break;
					(void)putchar('\n');
				}
			break;
		case IPOPT_RR:
			j = *++cp;		/* get length */
			i = *++cp;		/* and pointer */
			hlen -= j;
			if (i > j)
				i = j;
			i -= IPOPT_MINOFF;
			if (i <= 0)
				continue;
			if (i == old_rrlen
			    && cp == (u_char *)buf + sizeof(struct ip) + 2
			    && !bcmp((char *)cp, old_rr, i)
			    && !(options & F_FLOOD)) {
				(void)printf("\t(same route)");
				i = ((i + 3) / 4) * 4;
				cp += i;
				break;
			}
			old_rrlen = i;
			bcopy((char *)cp, old_rr, i);
			(void)printf("\nRR: ");
			for (;;) {
				l = *++cp;
				l = (l<<8) + *++cp;
				l = (l<<8) + *++cp;
				l = (l<<8) + *++cp;
				if (l == 0)
					(void)printf("\t0.0.0.0");
				else
					(void)printf("\t%s", pr_addr(ntohl(l)));
				i -= 4;
				if (i <= 0)
					break;
				(void)putchar('\n');
			}
			break;
		case IPOPT_NOP:
			(void)printf("\nNOP");
			break;
		default:
			(void)printf("\nunknown option %x", *cp);
			break;
		}
	if (!(options & F_FLOOD)) {
		(void)putchar('\n');
		(void)fflush(stdout);
	}
}

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
int
in_cksum(addr, len)
	u_short *addr;
	int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

/*
 * tvsub --
 *	Subtract 2 timeval structs:  out = out - in.  Out is assumed to
 * be >= in.
 */
void
tvsub(out, in)
	register struct timeval *out, *in;
{
	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 * finish --
 *	Print out statistics, and give up.
 */
void
finish()
{
	(void)signal(SIGINT, SIG_IGN);
	(void)putchar('\n');
	(void)fflush(stdout);
	(void)printf("--- %s ping statistics ---\n", hostname);
	(void)printf("%ld packets transmitted, ", ntransmitted);
	(void)printf("%ld packets received, ", nreceived);
	if (nrepeats)
		(void)printf("+%ld duplicates, ", nrepeats);
	if (ntransmitted)
		if (nreceived > ntransmitted)
			(void)printf("-- somebody's printing up packets!");
		else
			(void)printf("%d%% packet loss",
			    (int) (((ntransmitted - nreceived) * 100) /
			    ntransmitted));
	(void)putchar('\n');
	if (nreceived && timing)
		(void)printf("round-trip min/avg/max = %ld/%lu/%ld ms\n",
		    tmin, tsum / (nreceived + nrepeats), tmax);
	CleanUpExit(0);
}

#ifdef notdef
static char *ttab[] = {
	"Echo Reply",		/* ip + seq + udata */
	"Dest Unreachable",	/* net, host, proto, port, frag, sr + IP */
	"Source Quench",	/* IP */
	"Redirect",		/* redirect type, gateway, + IP  */
	"Echo",
	"Time Exceeded",	/* transit, frag reassem + IP */
	"Parameter Problem",	/* pointer + IP */
	"Timestamp",		/* id + seq + three timestamps */
	"Timestamp Reply",	/* " */
	"Info Request",		/* id + sq */
	"Info Reply"		/* " */
};
#endif

/*
 * pr_icmph --
 *	Print a descriptive string about an ICMP header.
 */
void
pr_icmph(icp)
	struct icmp *icp;
{
	switch(icp->icmp_type) {
	case ICMP_ECHOREPLY:
		(void)printf("Echo Reply\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_UNREACH:
		switch(icp->icmp_code) {
		case ICMP_UNREACH_NET:
			(void)printf("Destination Net Unreachable\n");
			break;
		case ICMP_UNREACH_HOST:
			(void)printf("Destination Host Unreachable\n");
			break;
		case ICMP_UNREACH_PROTOCOL:
			(void)printf("Destination Protocol Unreachable\n");
			break;
		case ICMP_UNREACH_PORT:
			(void)printf("Destination Port Unreachable\n");
			break;
		case ICMP_UNREACH_NEEDFRAG:
			(void)printf("frag needed and DF set\n");
			break;
		case ICMP_UNREACH_SRCFAIL:
			(void)printf("Source Route Failed\n");
			break;
		default:
			(void)printf("Dest Unreachable, Bad Code: %d\n",
			    icp->icmp_code);
			break;
		}
		/* Print returned IP header information */
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_SOURCEQUENCH:
		(void)printf("Source Quench\n");
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_REDIRECT:
		switch(icp->icmp_code) {
		case ICMP_REDIRECT_NET:
			(void)printf("Redirect Network");
			break;
		case ICMP_REDIRECT_HOST:
			(void)printf("Redirect Host");
			break;
		case ICMP_REDIRECT_TOSNET:
			(void)printf("Redirect Type of Service and Network");
			break;
		case ICMP_REDIRECT_TOSHOST:
			(void)printf("Redirect Type of Service and Host");
			break;
		default:
			(void)printf("Redirect, Bad Code: %d", icp->icmp_code);
			break;
		}
		(void)printf("(New addr: 0x%08lx)\n",
			(long unsigned)icp->icmp_gwaddr.s_addr);
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_ECHO:
		(void)printf("Echo Request\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_TIMXCEED:
		switch(icp->icmp_code) {
		case ICMP_TIMXCEED_INTRANS:
			(void)printf("Time to live exceeded\n");
			break;
		case ICMP_TIMXCEED_REASS:
			(void)printf("Frag reassembly time exceeded\n");
			break;
		default:
			(void)printf("Time exceeded, Bad Code: %d\n",
			    icp->icmp_code);
			break;
		}
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_PARAMPROB:
		(void)printf("Parameter problem: pointer = 0x%02x\n",
		    icp->icmp_hun.ih_pptr);
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_TSTAMP:
		(void)printf("Timestamp\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_TSTAMPREPLY:
		(void)printf("Timestamp Reply\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_IREQ:
		(void)printf("Information Request\n");
		/* XXX ID + Seq */
		break;
	case ICMP_IREQREPLY:
		(void)printf("Information Reply\n");
		/* XXX ID + Seq */
		break;
#ifdef ICMP_MASKREQ
	case ICMP_MASKREQ:
		(void)printf("Address Mask Request\n");
		break;
#endif
#ifdef ICMP_MASKREPLY
	case ICMP_MASKREPLY:
		(void)printf("Address Mask Reply\n");
		break;
#endif
	default:
		(void)printf("Bad ICMP type: %d\n", icp->icmp_type);
	}
}

/*
 * pr_iph --
 *	Print an IP header with options.
 */
void
pr_iph(ip)
	struct ip *ip;
{
	int hlen;
	u_char *cp;

	hlen = ip->ip_hl << 2;
	cp = (u_char *)ip + 20;		/* point to options */

	(void)printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst Data\n");
	(void)printf(" %1x  %1x  %02x %04x %04x",
	    ip->ip_v, ip->ip_hl, ip->ip_tos, ip->ip_len, ip->ip_id);
	(void)printf("   %1x %04x", ((ip->ip_off) & 0xe000) >> 13,
	    (ip->ip_off) & 0x1fff);
	(void)printf("  %02x  %02x %04x", ip->ip_ttl, ip->ip_p, ip->ip_sum);
	(void)printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->ip_src.s_addr));
	(void)printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->ip_dst.s_addr));
	/* dump and option bytes */
	while (hlen-- > 20) {
		(void)printf("%02x", *cp++);
	}
	(void)putchar('\n');
}

/*
 * pr_addr --
 *	Return an ascii host address as a dotted quad and optionally with
 * a hostname.
 */
char *
pr_addr(l)
	u_long l;
{
	struct hostent *hp;
	static char buf[80];
	union {
	    struct in_addr addr;
	    u_long l;
	} __tmp;
	
	__tmp.l = l;
	
	if ((options & F_NUMERIC) ||
	    !(hp = gethostbyaddr((char *)&l, 4, AF_INET)))
		(void)sprintf(buf, "%s", inet_ntoa(__tmp.addr));
	else
		(void)sprintf(buf, "%s (%s)", hp->h_name,
		    inet_ntoa(__tmp.addr));
	return(buf);
}

/*
 * pr_retip --
 *	Dump some info on a returned (via ICMP) IP packet.
 */
void
pr_retip(ip)
	struct ip *ip;
{
	int hlen;
	u_char *cp;

	pr_iph(ip);
	hlen = ip->ip_hl << 2;
	cp = (u_char *)ip + hlen;

	if (ip->ip_p == 6)
		(void)printf("TCP: from port %u, to port %u (decimal)\n",
		    (*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
	else if (ip->ip_p == 17)
		(void)printf("UDP: from port %u, to port %u (decimal)\n",
			(*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
}

void
fill(bp, patp)
	char *bp, *patp;
{
	register int ii, jj, kk;
	int pat[16];
	char *cp;

	for (cp = patp; *cp; cp++)
		if (!isxdigit(*cp)) {
			(void)fprintf(stderr,
			    "ping: patterns must be specified as hex digits.\n");
			CleanUpExit(1);
		}
	ii = sscanf(patp,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &pat[0], &pat[1], &pat[2], &pat[3], &pat[4], &pat[5], &pat[6],
	    &pat[7], &pat[8], &pat[9], &pat[10], &pat[11], &pat[12],
	    &pat[13], &pat[14], &pat[15]);

	if (ii > 0)
		for (kk = 0; kk <= MAXPACKET - (8 + ii); kk += ii)
			for (jj = 0; jj < ii; ++jj)
				bp[jj + kk] = pat[jj];
	if (!(options & F_QUIET)) {
		(void)printf("PATTERN: 0x");
		for (jj = 0; jj < ii; ++jj)
			(void)printf("%02x", bp[jj] & 0xFF);
		(void)printf("\n");
	}
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: ping [-Rdfnqrv] [-c count] [-i wait] [-l preload]\n"
		      "\t[-p pattern] [-s packetsize] [-L [hosts]] host\n");
	CleanUpExit(1);
}

VOID CleanUpExit(LONG error)
{
  CloseLibrary(SocketBase);
  exit(error);
}
