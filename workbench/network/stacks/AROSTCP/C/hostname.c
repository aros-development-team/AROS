/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2005
 *	Pavel Fedin
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#define ENABLE_SETHOSTNAME

#include <proto/socket.h>
#include <net/if_dl.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#if !defined(__AROS__)
# include <err.h>
#else
/* TODO: We dont have <err.h> */
#endif
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <proto/miami.h>

void usage(void);

#if defined(__AROS__)
#include <dos/dos.h>
#include <proto/exec.h>
struct Library *SocketBase;
struct Library *MiamiBase;
#endif

const char version[] = "$VER: hostname 1.1 (22.12.2005)";

int
main(int argc, char *argv[])
{
	int ch, sflag;
	char *p, hostname[MAXHOSTNAMELEN];

   if (!(SocketBase = OpenLibrary("bsdsocket.library", 3)))
   {
      return RETURN_FAIL;   
   }
   if (!(MiamiBase = OpenLibrary("miami.library", 0)))
   {
      return RETURN_FAIL;   
   }
	
	sflag = 0;
	while ((ch = getopt(argc, argv, "s")) != -1)
		switch (ch) {
		case 's':
			sflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#ifdef ENABLE_SETHOSTNAME
	if (argc > 1)
#else
	if (argc)
#endif
		usage();
#ifdef ENABLE_SETHOSTNAME
	if (*argv) {
		if (sethostname(*argv, (int)strlen(*argv)))
		{
#if !defined(__AROS__)
			err(1, "sethostname");
#endif
		}
	} else {
#endif
		if (gethostname(hostname, (int)sizeof(hostname)))
		{
#if !defined(__AROS__)
			err(1, "gethostname");
#endif
		}
		if (sflag) {
			p = strchr(hostname, '.');
			if (p != NULL)
				*p = '\0';
		}
		(void)printf("%s\n", hostname);
#ifdef ENABLE_SETHOSTNAME
	}
#endif
	exit(0);
}

void
usage(void)
{
#ifdef ENABLE_SETHOSTNAME
	(void)fprintf(stderr, "usage: hostname [-s] [name-of-host]\n");
#else
	(void)fprintf(stderr, "usage: hostname [-s]\n");
#endif
	exit(1);
}
