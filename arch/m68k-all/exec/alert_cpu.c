/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context parsing routines. Dummy nonfunctional template.
          See code in arch/i386/all/exec/alert_cpu.c for working example.
    Lang: english
*/

#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"

static inline char *addHex(char *buff, ULONG val)
{
    *(buff++) = "0123456789ABCDEF"[val & 0xf];

    return buff;
}

static inline char *addHexen(char *buff, ULONG val)
{
    int i;

    for (i = 0; i < 8; i++)
    	buff = addHex(buff, (val >> (28 - i * 4)) & 0xf);

    return buff;
}

char *FormatCPUContext(char *buffer, struct ExceptionContext *ctx, struct ExecBase *SysBase)
{
    char *buf = buffer;
    char tmp[16];
    int i;
    UWORD sr = (ctx)->sr;
    for (i = 0; i < 8; i++) {
    	tmp[0] = 'D';
    	tmp[1] = '0' + i;
    	tmp[2] = ':';
    	tmp[3] = ' ';
    	tmp[4] = 0;
    	buf = Alert_AddString(buf, tmp);
    	buf = addHexen(buf, (ctx)->d[i]);
    	if ((i%4) == 3)
    	    *buf = '\n';
    	else
    	    *buf = ' ';
    	buf++;
    }
    for (i = 0; i < 8; i++) {
    	tmp[0] = 'A';
    	tmp[1] = '0' + i;
    	tmp[2] = ':';
    	tmp[3] = ' ';
    	tmp[4] = 0;
    	buf = Alert_AddString(buf, tmp);
    	buf = addHexen(buf, (ctx)->a[i]);
    	if ((i%4) == 3)
    	    *buf = '\n';
    	else
    	    *buf = ' ';
    	buf++;
    }
    buf = Alert_AddString(buf, "SR: T="); buf = addHex(buf, (sr >> 14) & 3);
    buf = Alert_AddString(buf, " S="); buf = addHex(buf, (sr >> 13) & 1);
    buf = Alert_AddString(buf, " M="); buf = addHex(buf, (sr >> 5) & 1);
    buf = Alert_AddString(buf, " X="); buf = addHex(buf, (sr >> 4) & 1);
    buf = Alert_AddString(buf, " N="); buf = addHex(buf, (sr >> 3) & 1);
    buf = Alert_AddString(buf, " Z="); buf = addHex(buf, (sr >> 2) & 1);
    buf = Alert_AddString(buf, " V="); buf = addHex(buf, (sr >> 1) & 1);
    buf = Alert_AddString(buf, " C="); buf = addHex(buf, (sr >> 0) & 1);
    buf = Alert_AddString(buf, " IMASK="); buf = addHex(buf, (sr >> 8) & 7);
    /* PC was already printed out */
    // buf = Alert_AddString(buf, "\nPC: "); buf = addHexen(buf, ctx->pc);
    *(buf++) = '\n';
    *buf = 0;

    return buf;
}

/* Unwind a single stack frame */
APTR UnwindFrame(APTR fp, APTR *caller)
{
    return NULL;
}
