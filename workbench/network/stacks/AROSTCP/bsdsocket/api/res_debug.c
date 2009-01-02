/*-
 * Copyright (c) 1985, 1990 Regents of the University of California.
 * All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)res_debug.c	5.36 (Berkeley) 3/6/91
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_debug.c	5.36 (Berkeley) 3/6/91";
#endif /* LIBC_SCCS and not lint */

#ifdef RES_DEBUG
	
#include <conf.h>

#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <api/arpa_nameser.h>
#include <api/apicalls.h>
#include <api/resolv.h>
#include <stdio.h>
/* #include <string.h> */

void __fp_query();
char *__p_class(), *__p_time(), *__p_type();
static char *p_cdname(), *p_rr();

char *_res_opcodes[] = {
	"QUERY",
	"IQUERY",
	"CQUERYM",
	"CQUERYU",
	"4",
	"5",
	"6",
	"7",
	"8",
	"UPDATEA",
	"UPDATED",
	"UPDATEDA",
	"UPDATEM",
	"UPDATEMA",
	"ZONEINIT",
	"ZONEREF",
};

char *_res_resultcodes[] = {
	"NOERROR",
	"FORMERR",
	"SERVFAIL",
	"NXDOMAIN",
	"NOTIMP",
	"REFUSED",
	"6",
	"7",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"NOCHANGE",
};

__p_query(msg, libPtr)
	char *msg;
	struct SocketBase *libPtr;
{
	__fp_query(msg,libPtr);
}

/*
 * Print the contents of a query.
 * This is intended to be primarily a debugging routine.
 */
void
__fp_query(msg,libPtr)
	char *msg;
	struct SocketBase *libPtr;
{
	register char *cp;
	register HEADER *hp;
	register int n;

	/*
	 * Print header fields.
	 */
	hp = (HEADER *)msg;
	cp = msg + sizeof(HEADER);
	Printf("HEADER:\n");
	Printf("\topcode = %s", _res_opcodes[hp->opcode]);
	Printf(", id = %ld", ntohs(hp->id));
	Printf(", rcode = %s\n", _res_resultcodes[hp->rcode]);
	Printf("\theader flags: ");
	if (hp->qr)
		Printf(" qr");
	if (hp->aa)
		Printf(" aa");
	if (hp->tc)
		Printf(" tc");
	if (hp->rd)
		Printf(" rd");
	if (hp->ra)
		Printf(" ra");
	if (hp->pr)
		Printf(" pr");
	Printf("\n\tqdcount = %ld", ntohs(hp->qdcount));
	Printf(", ancount = %ld", ntohs(hp->ancount));
	Printf(", nscount = %ld", ntohs(hp->nscount));
	Printf(", arcount = %ld\n\n", ntohs(hp->arcount));
	/*
	 * Print question records.
	 */
	if (n = ntohs(hp->qdcount)) {
		Printf("QUESTIONS:\n");
		while (--n >= 0) {
			Printf("\t");
			cp = p_cdname(cp, msg);
			if (cp == NULL)
				return;
			Printf(", type = %s", __p_type(_getshort(cp)));
			cp += sizeof(u_short);
			Printf(
			    ", class = %s\n\n", __p_class(_getshort(cp)));
			cp += sizeof(u_short);
		}
	}
	/*
	 * Print authoritative answer records
	 */
	if (n = ntohs(hp->ancount)) {
		Printf("ANSWERS:\n");
		while (--n >= 0) {
			Printf("\t");
			cp = p_rr(cp, msg, libPtr);
			if (cp == NULL)
				return;
		}
	}
	/*
	 * print name server records
	 */
	if (n = ntohs(hp->nscount)) {
		Printf("NAME SERVERS:\n");
		while (--n >= 0) {
			Printf("\t");
			cp = p_rr(cp, msg, libPtr);
			if (cp == NULL)
				return;
		}
	}
	/*
	 * print additional records
	 */
	if (n = ntohs(hp->arcount)) {
		Printf("ADDITIONAL RECORDS:\n");
		while (--n >= 0) {
			Printf("\t");
			cp = p_rr(cp, msg, libPtr);
			if (cp == NULL)
				return;
		}
	}
}

static char *
p_cdname(cp, msg)
	char *cp, *msg;
{
	char name[MAXDNAME];
	int n;

	if ((n = dn_expand((u_char *)msg, (u_char *)msg + 512, (u_char *)cp,
	    (u_char *)name, sizeof(name))) < 0)
		return (NULL);
	if (name[0] == '\0') {
		name[0] = '.';
		name[1] = '\0';
	}
	PutStr(name);
	return (cp + n);
}

/*
 * Print resource record fields in human readable form.
 */
static char *
p_rr(cp, msg, libPtr)
	char *cp, *msg;
	struct SocketBase *libPtr;
{
	int type, class, dlen, n, c;
	struct in_addr inaddr;
	char *cp1, *cp2;

	if ((cp = p_cdname(cp, msg)) == NULL)
		return (NULL);			/* compression error */
	Printf("\n\ttype = %s", __p_type(type = _getshort(cp)));
	cp += sizeof(u_short);
	Printf(", class = %s", __p_class(class = _getshort(cp)));
	cp += sizeof(u_short);
	Printf(", ttl = %s", __p_time(_getlong(cp)));
	cp += sizeof(u_long);
	Printf(", dlen = %d\n", dlen = _getshort(cp));
	cp += sizeof(u_short);
	cp1 = cp;
	/*
	 * Print type specific data, if appropriate
	 */
	switch (type) {
	case T_A:
		switch (class) {
		case C_IN:
		case C_HS:
			bcopy(cp, (char *)&inaddr, sizeof(inaddr));
			if (dlen == 4) {
				Printf("\tinternet address = %s\n",
					__Inet_NtoA(inaddr.s_addr, libPtr));
				cp += dlen;
			} else if (dlen == 7) {
				Printf("\tinternet address = %s",
					__Inet_NtoA(inaddr.s_addr, libPtr));
				Printf(", protocol = %d", cp[4]);
				Printf(", port = %d\n",
					(cp[5] << 8) + cp[6]);
				cp += dlen;
			}
			break;
		default:
			cp += dlen;
		}
		break;
	case T_CNAME:
	case T_MB:
	case T_MG:
	case T_MR:
	case T_NS:
	case T_PTR:
		Printf("\tdomain name = ");
		cp = p_cdname(cp, msg);
		Printf("\n");
		break;

	case T_HINFO:
		if (n = *cp++) {
			Printf("\tCPU=%.*s\n", n, cp);
			cp += n;
		}
		if (n = *cp++) {
			Printf("\tOS=%.*s\n", n, cp);
			cp += n;
		}
		break;

	case T_SOA:
		Printf("\torigin = ");
		cp = p_cdname(cp, msg);
		Printf("\n\tmail addr = ");
		cp = p_cdname(cp, msg);
		Printf("\n\tserial = %ld", _getlong(cp));
		cp += sizeof(u_long);
		Printf("\n\trefresh = %s", __p_time(_getlong(cp)));
		cp += sizeof(u_long);
		Printf("\n\tretry = %s", __p_time(_getlong(cp)));
		cp += sizeof(u_long);
		Printf("\n\texpire = %s", __p_time(_getlong(cp)));
		cp += sizeof(u_long);
		Printf("\n\tmin = %s\n", __p_time(_getlong(cp)));
		cp += sizeof(u_long);
		break;

	case T_MX:
		Printf("\tpreference = %ld,",_getshort(cp));
		cp += sizeof(u_short);
		Printf(" name = ");
		cp = p_cdname(cp, msg);
		break;

  	case T_TXT:
		PutStr("\t\"");
		cp2 = cp1 + dlen;
		while (cp < cp2) {
			if (n = (unsigned char) *cp++) {
				for (c = n; c > 0 && cp < cp2; c--)
					if (*cp == '\n') {
					    PutStr("\\");
					    WriteChars(cp++, 1);
					} else
					    WriteChars(cp++, 2);
			}
		}
		PutStr("\"\n");
  		break;

	case T_MINFO:
		Printf("\trequests = ");
		cp = p_cdname(cp, msg);
		Printf("\n\terrors = ");
		cp = p_cdname(cp, msg);
		break;

	case T_UINFO:
		Printf("\t%s\n", cp);
		cp += dlen;
		break;

	case T_UID:
	case T_GID:
		if (dlen == 4) {
			Printf("\t%ld\n", _getlong(cp));
			cp += sizeof(int);
		}
		break;

	case T_WKS:
		if (dlen < sizeof(u_long) + 1)
			break;
		bcopy(cp, (char *)&inaddr, sizeof(inaddr));
		cp += sizeof(u_long);
		Printf("\tinternet address = %s, protocol = %d\n\t",
			__Inet_NtoA(inaddr.s_addr, libPtr), *cp++);
		n = 0;
		while (cp < cp1 + dlen) {
			c = *cp++;
			do {
 				if (c & 0200)
					Printf(" %ld", n);
 				c <<= 1;
			} while (++n & 07);
		}
		PutStr("\n");
		break;

#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		{
			int NumBytes = 8;
			char *DataPtr;
			int i;

			if (dlen < NumBytes) NumBytes = dlen;
			Printf( "\tFirst %ld bytes of hex data:",
				NumBytes);
			for (i = 0, DataPtr = cp; i < NumBytes; i++, DataPtr++)
				Printf( " %x", *DataPtr);
			PutStr("\n");
			cp += dlen;
		}
		break;
#endif /* ALLOW_T_UNSPEC */

	default:
		Printf("\t???\n");
		cp += dlen;
	}
	if (cp != cp1 + dlen) {
		Printf("packet size error (%#x != %#x)\n", cp, cp1+dlen);
		cp = NULL;
	}
	Printf("\n");
	return (cp);
}

static	char nbuf[40];

/*
 * Return a string for the type
 */
char *
__p_type(type)
	int type;
{
	switch (type) {
	case T_A:
		return("A");
	case T_NS:		/* authoritative server */
		return("NS");
	case T_CNAME:		/* canonical name */
		return("CNAME");
	case T_SOA:		/* start of authority zone */
		return("SOA");
	case T_MB:		/* mailbox domain name */
		return("MB");
	case T_MG:		/* mail group member */
		return("MG");
	case T_MR:		/* mail rename name */
		return("MR");
	case T_NULL:		/* null resource record */
		return("NULL");
	case T_WKS:		/* well known service */
		return("WKS");
	case T_PTR:		/* domain name pointer */
		return("PTR");
	case T_HINFO:		/* host information */
		return("HINFO");
	case T_MINFO:		/* mailbox information */
		return("MINFO");
	case T_MX:		/* mail routing info */
		return("MX");
	case T_TXT:		/* text */
		return("TXT");
	case T_AXFR:		/* zone transfer */
		return("AXFR");
	case T_MAILB:		/* mail box */
		return("MAILB");
	case T_MAILA:		/* mail address */
		return("MAILA");
	case T_ANY:		/* matches any type */
		return("ANY");
	case T_UINFO:
		return("UINFO");
	case T_UID:
		return("UID");
	case T_GID:
		return("GID");
#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		return("UNSPEC");
#endif /* ALLOW_T_UNSPEC */
	default:
		(void)sprintf(nbuf, "%ld", type);
		return(nbuf);
	}
}

/*
 * Return a mnemonic for class
 */
char *
__p_class(class)
	int class;
{

	switch (class) {
	case C_IN:		/* internet class */
		return("IN");
	case C_HS:		/* hesiod class */
		return("HS");
	case C_ANY:		/* matches any class */
		return("ANY");
	default:
		(void)sprintf(nbuf, "%ld", class);
		return(nbuf);
	}
}

/*
 * Return a mnemonic for a time to live
 */
char *
__p_time(value)
	u_long value;
{
	int secs, mins, hours;
	register char *p;

	if (value == 0) {
		strcpy(nbuf, "0 secs");
		return(nbuf);
	}

	secs = value % 60;
	value /= 60;
	mins = value % 60;
	value /= 60;
	hours = value % 24;
	value /= 24;

#define	PLURALIZE(x)	x, (x == 1) ? "" : "s"
	p = nbuf;
	if (value) {
		(void)sprintf(p, "%ld day%s", PLURALIZE(value));
		while (*++p);
	}
	if (hours) {
		if (value)
			*p++ = ' ';
		(void)sprintf(p, "%ld hour%s", PLURALIZE(hours));
		while (*++p);
	}
	if (mins) {
		if (value || hours)
			*p++ = ' ';
		(void)sprintf(p, "%ld min%s", PLURALIZE(mins));
		while (*++p);
	}
	if (secs || ! (value || hours || mins)) {
		if (value || hours || mins)
			*p++ = ' ';
		(void)sprintf(p, "%ld sec%s", PLURALIZE(secs));
	}
	return(nbuf);
}

#endif /* RES_DEBUG */
