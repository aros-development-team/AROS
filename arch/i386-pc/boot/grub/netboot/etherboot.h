/**************************************************************************
ETHERBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters
  Date: Dec/93

**************************************************************************/

/* Enable GRUB-specific stuff.  */
#define GRUB	1

#ifdef GRUB
/* Include GRUB-specific macros and prototypes here.  */
# include <shared.h>

/* FIXME: For now, enable the DHCP support. Perhaps I should segregate
   the DHCP support from the BOOTP support, and permit both to
   co-exist.  */
# undef NO_DHCP_SUPPORT

/* In GRUB, the relocated address in Etherboot doesn't have any sense.
   Just define it as a bogus value.  */
# define RELOC	0

/* Force to use the internal buffer.  */
# define INTERNAL_BOOTP_DATA	1

/* Likewise.  */
# define USE_INTERNAL_BUFFER	1

/* FIXME: Should be an option.  */
# define BACKOFF_LIMIT	7
#endif /* GRUB */

#include "osdep.h"

/* These could be customised for different languages perhaps */
#define	ASK_PROMPT	"Boot from (N)etwork or from (L)ocal? "
#define	ANS_NETWORK	'N'
#define	ANS_LOCAL	'L'
#ifndef	ANS_DEFAULT	/* in case left out in Makefile */
#define	ANS_DEFAULT	ANS_NETWORK
#endif

#define	TAGGED_IMAGE		/* eventually optional */
#if	!defined(TAGGED_IMAGE) && !defined(AOUT_IMAGE) && !defined(ELF_IMAGE)
#define	TAGGED_IMAGE		/* choose at least one */
#endif

#ifdef GRUB
# define CTRL_C		3
#else /* ! GRUB */
# define ESC		0x1B
#endif /* ! GRUB */

#ifndef	DEFAULT_BOOTFILE
#define DEFAULT_BOOTFILE	"/tftpboot/kernel"
#endif

/* Clean up console settings... mainly CONSOLE_CRT and CONSOLE_SERIAL are used
 * in the sources (except start.S and serial.S which cannot include
 * etherboot.h).  At least one of the CONSOLE_xxx has to be set, and
 * CONSOLE_DUAL sets both CONSOLE_CRT and CONSOLE_SERIAL.  If none is set,
 * CONSOLE_CRT is assumed.  */
#ifdef	CONSOLE_DUAL
#undef CONSOLE_CRT
#define CONSOLE_CRT
#undef CONSOLE_SERIAL
#define CONSOLE_SERIAL
#endif
#if	defined(CONSOLE_CRT) && defined(CONSOLE_SERIAL)
#undef CONSOLE_DUAL
#define CONSOLE_DUAL
#endif
#if	!defined(CONSOLE_CRT) && !defined(CONSOLE_SERIAL)
#define CONSOLE_CRT
#endif

#ifndef	DOWNLOAD_PROTO_NFS
#undef DOWNLOAD_PROTO_TFTP
#define DOWNLOAD_PROTO_TFTP	/* default booting protocol */
#endif

#ifdef	DOWNLOAD_PROTO_TFTP
#define download(fname,loader) tftp((fname),(loader))
#endif
#ifdef	DOWNLOAD_PROTO_NFS
#define download(fname,loader) nfs((fname),(loader))
#endif

#ifndef	MAX_TFTP_RETRIES
#define MAX_TFTP_RETRIES	20
#endif

#ifndef	MAX_BOOTP_RETRIES
#define MAX_BOOTP_RETRIES	20
#endif

#ifndef	MAX_BOOTP_EXTLEN
#if	(RELOC < 0x94000)
/* Force internal buffer (if external buffer would overlap with our code...) */
#undef INTERNAL_BOOTP_DATA
#define INTERNAL_BOOTP_DATA
#endif
/* sizeof(struct bootp_t) == 0x240 */
#if	defined(INTERNAL_BOOTP_DATA) || (RELOC >= 0x94240)
#define MAX_BOOTP_EXTLEN	1024
#else
#define MAX_BOOTP_EXTLEN	(1024-sizeof(struct bootp_t))
#endif
#endif

#ifndef	MAX_ARP_RETRIES
#define MAX_ARP_RETRIES		20
#endif

#ifndef	MAX_RPC_RETRIES
#define MAX_RPC_RETRIES		20
#endif

#define	TICKS_PER_SEC		18

/* Inter-packet retry in ticks */
#define TIMEOUT			(10*TICKS_PER_SEC)

/* These settings have sense only if compiled with -DCONGESTED */
/* total retransmission timeout in ticks */
#define TFTP_TIMEOUT		(30*TICKS_PER_SEC)
/* packet retransmission timeout in ticks */
#define TFTP_REXMT		(3*TICKS_PER_SEC)

#ifndef	NULL
#define NULL	((void *)0)
#endif

#define TRUE		1
#define FALSE		0

#define ETHER_ADDR_SIZE		6	/* Size of Ethernet address */
#define ETHER_HDR_SIZE		14	/* Size of ethernet header */
#define ETH_MIN_PACKET		64
#define ETH_MAX_PACKET		1518

#define VENDOR_NONE	0
#define VENDOR_WD	1
#define VENDOR_NOVELL	2
#define VENDOR_3COM	3
#define VENDOR_3C509	4
#define VENDOR_CS89x0	5

#define FLAG_PIO	0x01
#define FLAG_16BIT	0x02
#define FLAG_790	0x04

#define ARP_CLIENT	0
#define ARP_SERVER	1
#define ARP_GATEWAY	2
#define ARP_ROOTSERVER	3
#define ARP_SWAPSERVER	4
#define MAX_ARP		ARP_SWAPSERVER+1

#define	RARP_REQUEST	3
#define	RARP_REPLY	4

#define IP		0x0800
#define ARP		0x0806
#define	RARP		0x8035

#define BOOTP_SERVER	67
#define BOOTP_CLIENT	68
#define TFTP_PORT	69
#define SUNRPC_PORT	111

#define IP_UDP		17
/* Same after going through htonl */
#define IP_BROADCAST	0xFFFFFFFF

#define ARP_REQUEST	1
#define ARP_REPLY	2

#define BOOTP_REQUEST	1
#define BOOTP_REPLY	2

#define TAG_LEN(p)		(*((p)+1))
#define RFC1533_COOKIE		99, 130, 83, 99
#define RFC1533_PAD		0
#define RFC1533_NETMASK		1
#define RFC1533_TIMEOFFSET	2
#define RFC1533_GATEWAY		3
#define RFC1533_TIMESERVER	4
#define RFC1533_IEN116NS	5
#define RFC1533_DNS		6
#define RFC1533_LOGSERVER	7
#define RFC1533_COOKIESERVER	8
#define RFC1533_LPRSERVER	9
#define RFC1533_IMPRESSSERVER	10
#define RFC1533_RESOURCESERVER	11
#define RFC1533_HOSTNAME	12
#define RFC1533_BOOTFILESIZE	13
#define RFC1533_MERITDUMPFILE	14
#define RFC1533_DOMAINNAME	15
#define RFC1533_SWAPSERVER	16
#define RFC1533_ROOTPATH	17
#define RFC1533_EXTENSIONPATH	18
#define RFC1533_IPFORWARDING	19
#define RFC1533_IPSOURCEROUTING	20
#define RFC1533_IPPOLICYFILTER	21
#define RFC1533_IPMAXREASSEMBLY	22
#define RFC1533_IPTTL		23
#define RFC1533_IPMTU		24
#define RFC1533_IPMTUPLATEAU	25
#define RFC1533_INTMTU		26
#define RFC1533_INTLOCALSUBNETS	27
#define RFC1533_INTBROADCAST	28
#define RFC1533_INTICMPDISCOVER	29
#define RFC1533_INTICMPRESPOND	30
#define RFC1533_INTROUTEDISCOVER 31
#define RFC1533_INTROUTESOLICIT	32
#define RFC1533_INTSTATICROUTES	33
#define RFC1533_LLTRAILERENCAP	34
#define RFC1533_LLARPCACHETMO	35
#define RFC1533_LLETHERNETENCAP	36
#define RFC1533_TCPTTL		37
#define RFC1533_TCPKEEPALIVETMO	38
#define RFC1533_TCPKEEPALIVEGB	39
#define RFC1533_NISDOMAIN	40
#define RFC1533_NISSERVER	41
#define RFC1533_NTPSERVER	42
#define RFC1533_VENDOR		43
#define RFC1533_NBNS		44
#define RFC1533_NBDD		45
#define RFC1533_NBNT		46
#define RFC1533_NBSCOPE		47
#define RFC1533_XFS		48
#define RFC1533_XDM		49
#ifndef	NO_DHCP_SUPPORT
#define RFC2132_REQ_ADDR	50
#define RFC2132_MSG_TYPE	53
#define RFC2132_SRV_ID		54
#define RFC2132_PARAM_LIST	55
#define RFC2132_MAX_SIZE	57

#define DHCPDISCOVER		1
#define DHCPOFFER		2
#define DHCPREQUEST		3
#define DHCPACK			5
#endif	/* NO_DHCP_SUPPORT */

#define RFC1533_VENDOR_MAJOR	0
#define RFC1533_VENDOR_MINOR	0

#define RFC1533_VENDOR_MAGIC	128
#define RFC1533_VENDOR_ADDPARM	129
#ifdef	IMAGE_FREEBSD
#define RFC1533_VENDOR_HOWTO    132
#endif
#define RFC1533_VENDOR_MNUOPTS	160
#define RFC1533_VENDOR_SELECTION 176
#define RFC1533_VENDOR_MOTD	184
#define RFC1533_VENDOR_NUMOFMOTD 8
#define RFC1533_VENDOR_IMG	192
#define RFC1533_VENDOR_NUMOFIMG	16

#ifdef GRUB
# define RFC1533_VENDOR_CONFIGFILE	150
#endif /* GRUB */

#define RFC1533_END		255
#define BOOTP_VENDOR_LEN	64
#ifndef	NO_DHCP_SUPPORT
#define DHCP_OPT_LEN		312
#endif	/* NO_DHCP_SUPPORT */

#define	TFTP_DEFAULTSIZE_PACKET	512
#define	TFTP_MAX_PACKET		1432 /* 512 */

#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5
#define TFTP_OACK	6

#define TFTP_CODE_EOF	1
#define TFTP_CODE_MORE	2
#define TFTP_CODE_ERROR	3
#define TFTP_CODE_BOOT	4
#define TFTP_CODE_CFG	5

#define AWAIT_ARP	0
#define AWAIT_BOOTP	1
#define AWAIT_TFTP	2
#define AWAIT_RARP	3
#define AWAIT_RPC	4
#define AWAIT_QDRAIN	5	/* drain queue, process ARP requests */

typedef struct {
	unsigned long	s_addr;
} in_addr;

struct arptable_t {
	in_addr ipaddr;
	unsigned char node[6];
};

/*
 * A pity sipaddr and tipaddr are not longword aligned or we could use
 * in_addr. No, I don't want to use #pragma packed.
 */
struct arprequest {
	unsigned short hwtype;
	unsigned short protocol;
	char hwlen;
	char protolen;
	unsigned short opcode;
	char shwaddr[6];
	char sipaddr[4];
	char thwaddr[6];
	char tipaddr[4];
};

struct iphdr {
	char verhdrlen;
	char service;
	unsigned short len;
	unsigned short ident;
	unsigned short frags;
	char ttl;
	char protocol;
	unsigned short chksum;
	in_addr src;
	in_addr dest;
};

struct udphdr {
	unsigned short src;
	unsigned short dest;
	unsigned short len;
	unsigned short chksum;
};

struct bootp_t {
	struct iphdr ip;
	struct udphdr udp;
	char bp_op;
	char bp_htype;
	char bp_hlen;
	char bp_hops;
	unsigned long bp_xid;
	unsigned short bp_secs;
	unsigned short unused;
	in_addr bp_ciaddr;
	in_addr bp_yiaddr;
	in_addr bp_siaddr;
	in_addr bp_giaddr;
	char bp_hwaddr[16];
	char bp_sname[64];
	char bp_file[128];
#ifdef	NO_DHCP_SUPPORT
	char bp_vend[BOOTP_VENDOR_LEN];
#else
	char bp_vend[DHCP_OPT_LEN];
#endif	/* NO_DHCP_SUPPORT */
};

struct bootpd_t {
	struct bootp_t bootp_reply;
	unsigned char  bootp_extension[MAX_BOOTP_EXTLEN];
};

struct tftp_t {
	struct iphdr ip;
	struct udphdr udp;
	unsigned short opcode;
	union {
		char rrq[TFTP_DEFAULTSIZE_PACKET];
		struct {
			unsigned short block;
			char download[TFTP_MAX_PACKET];
		} data;
		struct {
			unsigned short block;
		} ack;
		struct {
			unsigned short errcode;
			char errmsg[TFTP_DEFAULTSIZE_PACKET];
		} err;
		struct {
			char data[TFTP_DEFAULTSIZE_PACKET+2];
		} oack;
	} u;
};

#define TFTP_MIN_PACKET	(sizeof(struct iphdr) + sizeof(struct udphdr) + 4)

struct rpc_t {
	struct iphdr ip;
	struct udphdr udp;
	union {
		char data[300];		/* longest RPC call must fit!!!! */
		struct {
			long id;
			long type;
			long rpcvers;
			long prog;
			long vers;
			long proc;
			long data[1];
		} call;
		struct {
			long id;
			long type;
			long rstatus;
			long verifier;
			long v2;
			long astatus;
			long data[1];
		} reply;
	} u;
};

#define PROG_PORTMAP	100000
#define PROG_NFS	100003
#define PROG_MOUNT	100005

#define MSG_CALL	0
#define MSG_REPLY	1

#define PORTMAP_GETPORT	3

#define MOUNT_ADDENTRY	1
#define MOUNT_UMOUNTALL	4

#define NFS_LOOKUP	4
#define NFS_READ	6

#define NFS_FHSIZE	32

#define NFSERR_PERM	1
#define NFSERR_NOENT	2
#define NFSERR_ACCES	13

/* Block size used for NFS read accesses.  A RPC reply packet (including  all
 * headers) must fit within a single Ethernet frame to avoid fragmentation.
 * Chosen to be a power of two, as most NFS servers are optimized for this.  */
#define NFS_READ_SIZE	1024

#define	FLOPPY_BOOT_LOCATION	0x7c00

#define	ROM_INFO_LOCATION	0x7dfa
/* at end of floppy boot block */

struct rom_info {
	unsigned short	rom_segment;
	unsigned short	rom_length;
};

/***************************************************************************
External prototypes
***************************************************************************/
/* main.c */
#ifdef GRUB
extern void print_network_configuration P((void));
extern int arp_server_override P((const char *buf));
#endif /* GRUB */

#ifndef GRUB
extern void print_bytes P((unsigned char *bytes, int len));
extern void load P((void));
extern int load_linux P((int root_mount_port,int swap_mount_port,
	int root_nfs_port,char *kernel_handle));
extern int downloadkernel P((unsigned char *, int, int, int));
extern int tftp P((const char *name, int (*)(unsigned char *, int, int, int)));
extern void rpc_init(void);
extern int nfs P((const char *name, int (*)(unsigned char *, int, int, int)));
extern void nfs_umountall P((int));
#endif /* ! GRUB */
extern int bootp P((void));
extern int rarp P((void));
extern int udp_transmit P((unsigned long destip, unsigned int srcsock,
	unsigned int destsock, int len, const void *buf));

extern int await_reply P((int type, int ival, void *ptr, int timeout));
extern int decode_rfc1533 P((unsigned char *, int, int, int));
extern unsigned short ipchksum P((unsigned short *, int len));
extern void rfc951_sleep P((int));
extern void cleanup_net P((void));
extern void cleanup P((void));

/* config.c */
extern void print_config(void);
extern void eth_reset(void);
extern int eth_probe(void);
extern int eth_poll(void);
extern void eth_transmit(const char *d, unsigned int t, unsigned int s, const void *p);
extern void eth_disable(void);

#ifndef GRUB
/* bootmenu.c */
extern int execute P((char *string));
extern void bootmenu P((int));
extern void show_motd P((void));
extern void parse_menuopts P((char *,int));
extern int  getoptvalue P((char **, int *, int *));
extern void selectImage P((char **));

/* osloader.c */
#if	defined(AOUT_IMAGE) || defined(ELF_IMAGE)
extern int howto;
#endif
extern int os_download P((unsigned int, unsigned char *,unsigned int));
#endif /* ! GRUB */

/* misc.c */
extern void twiddle P((void));
extern void sleep P((int secs));
#ifndef GRUB
extern int strcasecmp P((char *a, char *b));
extern char *substr P((char *a, char *b));
#endif /* ! GRUB */
extern int getdec P((char **));
#ifndef GRUB
extern void printf P((const char *, ...));
extern char *sprintf P((char *, const char *, ...));
#endif /* ! GRUB */
extern int inet_aton P((char *p, in_addr *i));
#ifndef GRUB
extern void gateA20_set P((void));
extern void gateA20_unset P((void));
extern void putchar P((int));
extern int getchar P((void));
extern int iskey P((void));

/* start*.S */
extern int getc P((void));
extern void putc P((int));
extern int ischar P((void));
extern int getshift P((void));
extern unsigned int memsize P((void));
extern unsigned short basememsize P((void));
extern void disk_init P((void));
extern unsigned int disk_read P((int drv,int c,int h,int s,char *buf));
extern void xstart P((unsigned long, unsigned long, char *));
extern unsigned long currticks P((void));
extern int setjmp P((void *jmpbuf));
extern void longjmp P((void *jmpbuf, int where));
extern void exit P((int status));
extern void slowdownio P((void));

/* serial.S */
extern int serial_getc P((void));
extern void serial_putc P((int));
extern int serial_ischar P((void));
extern int serial_init P((void));

/* ansiesc.c */
extern void ansi_reset P((void));
extern void enable_cursor P((int));
extern void handleansi P((unsigned char));

/* md5.c */
extern void md5_put P((unsigned int ch));
extern void md5_done P((unsigned char *buf));

/* floppy.c */
extern int bootdisk P((int dev,int part));
#endif /* ! GRUB */

/***************************************************************************
External variables
***************************************************************************/
/* main.c */
#ifdef GRUB
extern int ip_abort;
extern int network_ready;
#endif /* GRUB */

#ifndef GRUB
extern const char *kernel;
extern char kernel_buf[128];
#endif /* ! GRUB */
extern struct rom_info rom;
#ifndef GRUB
extern int hostnamelen;
extern unsigned long netmask;
extern int jmp_bootmenu[10];
#endif /* ! GRUB */
extern struct arptable_t arptable[MAX_ARP];
#ifndef GRUB
#ifdef	IMAGE_MENU
extern char *motd[RFC1533_VENDOR_NUMOFMOTD];
extern int menutmo,menudefault;
extern unsigned char *defparams;
extern int defparams_max;
#endif
#endif /* ! GRUB */
#if	defined(ETHERBOOT32) && !defined(INTERNAL_BOOTP_DATA)
#define	BOOTP_DATA_ADDR	((struct bootpd_t *)0x93C00)
#else
extern struct bootpd_t bootp_data;
#define	BOOTP_DATA_ADDR	(&bootp_data)
#endif
extern unsigned char *end_of_rfc1533;
#ifdef	IMAGE_FREEBSD
extern int freebsd_howto;
#endif

/* config.c */
extern struct nic nic;

/* bootmenu.c */

/* osloader.c */

#ifndef GRUB
/* created by linker */
extern char _start[], _edata[], _end[];
#endif /* ! GRUB */

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
