/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
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

#include <conf.h>

#include <sys/param.h>
#include <kern/amiga_includes.h>	/* This is needed by sys/synch.h */
#include <sys/synch.h>

#include <net/netisr.h>


extern struct MsgPort *SanaPort;

ULONG netisr = 0L;

/*
 * scnednetisr(): schedule net "interrupt" 
 * This routine signals TCP/IP task to run net_poll via sana_poll
 */
void schednetisr(int isr)
{
  netisr |= 1<<isr;
  Signal(SanaPort->mp_SigTask, 1<<SanaPort->mp_SigBit);
}

void schednetisr_nosignal(int isr)
{
  netisr |= 1<<isr;
}

/* 
 * net_poll(): run scheduled network level protocols
 *             this routine is called from sana_poll
 */
void 
  net_poll(void)
{
  extern void ipintr(void);
  extern void impintr(void);
  extern void isointr(void);
  extern void ccittintr(void);
  extern void nsintr(void);

  spl_t s = splimp();
  int n = netisr; 
  netisr = 0;
  splx(s);

  if (!n) return;
#if	INET
  if (n & (1<<NETISR_IP)) {
    ipintr();
  }
#endif /* INET */
#if	NIMP > 0
  if (n & (1<<NETISR_IMP)) {
    impintr();
  }
#endif /* NIMP > 0 */
#if	ISO
  if (n & (1<<NETISR_ISO)) {
    isointr();
  }
#endif /* ISO */
#if	CCITT
  if (n & (1<<NETISR_CCITT)) {
    ccittintr();
  }
#endif /* CCITT */
#if	NS
  if (n & (1<<NETISR_NS)) {
    nsintr();
  }
#endif /* NS */
#if 0
  /* raw input do not go through the isr */
  if (n & (1<<NETISR_RAW)) {
    rawintr();
  }
#endif
}
