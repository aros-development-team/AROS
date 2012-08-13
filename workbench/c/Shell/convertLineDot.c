/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>

#include "Shell.h"

static LONG getArgumentIdx(ShellState *ss, STRPTR name, LONG len)
{
    struct SArg *a;
    LONG i;

    for (i = 0; i < ss->argcount; ++i)
    {
	a = ss->args + i;

	if (strncmp(a->name, name, len) == 0)
	    return i;
    }

    if (ss->argcount >= MAXARGS)
	return -1;

    ss->argcount++;
    a = ss->args + i;
    CopyMem(name, a->name, len);
    a->name[len] = '\0';
    a->namelen = len;
    a->type = NORMAL;
    return i;
}

static LONG dotDef(ShellState *ss, STRPTR szz, Buffer *in, LONG len)
{
    struct SArg *a;
    LONG i, result;
    TEXT buf[256];

    in->cur += len;
    i = in->cur;

    if ((result = bufferReadItem(buf, sizeof(buf), in, DOSBase)) == ITEM_UNQUOTED)
    {
	len = in->cur - i;

	i = getArgumentIdx(ss, buf, len);
	if (i < 0)
	    return ERROR_TOO_MANY_ARGS;

	a = ss->args + i;
	i = ++in->cur;

	switch (bufferReadItem(buf, sizeof(buf), in, DOSBase))
	{
	case ITEM_QUOTED:
	case ITEM_UNQUOTED:
	    break;
	default:
	    return ERROR_REQUIRED_ARG_MISSING;
	}

	len = in->cur - i;

	if (a->def)
	    FreeMem((APTR) a->def, a->deflen + 1);

	a->def = (IPTR) AllocMem(len + 1, MEMF_LOCAL);
	CopyMem(buf, (APTR) a->def, len);
	((STRPTR) a->def)[len] = '\0';
	a->deflen = len;
	return 0;
    }

    return ERROR_REQUIRED_ARG_MISSING;
}

static LONG dotKey(ShellState *ss, STRPTR s, Buffer *in)
{
    LONG error, i, j, len;
    struct SArg *a;
    TEXT t[256];

    /* modify the template to read numbers as strings ! */
    for (i = 0, j = 0; s[j] != '\0' && s[j] != '\n' && i < sizeof(t); ++j)
    {
	if (s[j] == '/' && s[j + 1] == 'N')
	    ++j;
	else
	    t[i++] = s[j];
    }

    if (i >= sizeof(t))
	return ERROR_LINE_TOO_LONG;

    t[i] = '\0';

    /* Free the old ReadArgs value */
    if (ss->arg_rd)
        FreeDosObject(DOS_RDARGS, ss->arg_rd);

    if ((ss->arg_rd = ReadArgs(t, ss->arg, NULL)) == NULL)
    {
	error = IoErr();
	return error;
    }

    ss->argcount = 0;

    for (i = 0; i < MAXARGS; ++i)
    {
	STRPTR arg;

	for (len = 0; *s != '/' && *s != ',' && *s != '\n' && *s != '\0'; ++s)
	    ++len;

	j = getArgumentIdx(ss, s - len, len);
	arg = (STRPTR) ss->arg[j];
	a = ss->args + j;

	while (*s == '/')
	{
	    switch (*++s)
	    {
	    case 'a': case 'A': a->type |= REQUIRED; break;
	    case 'f': case 'F': a->type |= REST; break;
	    case 'k': case 'K': a->type |= KEYWORD; break;
	    case 'm': case 'M': a->type |= MULTIPLE; break;
	    case 'n': case 'N': a->type |= NUMERIC; break;
	    case 's': case 'S': a->type |= SWITCH; break;
	    case 't': case 'T': a->type |= TOGGLE; break;
	    default:
		return ERROR_BAD_TEMPLATE;
	    }
	    ++s;
	}

	if (arg && !(a->type & (SWITCH | TOGGLE)))
	    a->len = cliLen(arg);

	if (arg && a->type & NUMERIC) /* verify numbers validity */
	{
	    if (a->type & MULTIPLE)
	    {
		STRPTR *m = (STRPTR *) arg;

		while (*m)
		    if (cliNan(*m++))
			return ERROR_BAD_NUMBER;
	    }
	    else if (cliNan(arg))
		return ERROR_BAD_NUMBER;
	}

	if (*s++ != ',')
	    break;
    }

    in->cur = in->len;
    return 0;
}

LONG convertLineDot(ShellState *ss, Buffer *in)
{
    STRPTR s = in->buf + in->cur;
    TEXT *res = NULL;

#if 0 /* version using libc */
    if (*++s == ' ') /* dot comment */
	res = s;
    else if (strncasecmp(s, "bra ", 4) == 0)
    {
	res = &ss->bra;
	s += 4;
    }
    else if (strncasecmp(s, "ket ", 4) == 0)
    {
	res = &ss->ket;
	s += 4;
    }
    else if (strncasecmp(s, "key ", 4) == 0)
	return dotKey(ss, s + 4, in);
    else if (strncasecmp(s, "k ", 2) == 0)
	return dotKey(ss, s + 2, in);
    else if (strncasecmp(s, "def ", 4) == 0)
	return dotDef(ss, s + 4, in, 5);
    else if (strncasecmp(s, "default ", 8) == 0)
	return dotDef(ss, s + 4, in, 9);
    else if (strncasecmp(s, "dol ", 4) == 0)
    {
	res = &ss->dollar;
	s += 4;
    }
    else if (strncasecmp(s, "dollar ", 7) == 0)
    {
	res = &ss->dollar;
	s += 7;
    }
    else if (strncasecmp(s, "dot ", 4) == 0)
    {
	res = &ss->dot;
	s += 4;
    }
    else if (strncasecmp(s, "popis", 5) == 0)
    {
	popInterpreterState(ss);
	res = s;
    }
    else if (strncasecmp(s, "pushis", 6) == 0)
    {
	pushInterpreterState(ss);
	res = s;
    }
#else /* this ugly version is 424 bytes smaller on x64 */
    if (*++s == ' ') /* dot comment */
	res = s;
    else if (*s == 'b' || *s == 'B') /* .bra */
    {
	if (*++s == 'r' || *s == 'R')
	    if (*++s == 'a' || *s == 'A')
		if (*++s == ' ')
		    res = &ss->bra;
    }
    else if (*s == 'k' || *s == 'K')
    {
	if (*++s == ' ') /* .k */
	    return dotKey(ss, ++s, in);

	if (*s == 'e' || *s == 'E')
	{
	    if (*++s == 'y' || *s == 'Y') /* .key */
	    {
		if (*++s == ' ')
		    return dotKey(ss, ++s, in);
	    }
	    else if (*s == 't' || *s == 'T') /* .ket */
		if (*++s == ' ')
		    res = &ss->ket;
	}
    }
    else if (*s == 'd' || *s == 'D')
    {
	if (*++s == 'e' || *s == 'E')
	{
	    if (*++s == 'f' || *s == 'F')
	    {
		if (*++s == ' ') /* .def */
		    return dotDef(ss, ++s, in, 5);

		if (*s == 'a' || *s == 'A')
		    if (*++s == 'u' || *s == 'U')
			if (*++s == 'l' || *s == 'L')
			    if (*++s == 't' || *s == 'T')
				if (*++s == ' ') /* .default */
				    return dotDef(ss, ++s, in, 9);
	    }
	}
	else if (*s == 'o' || *s == 'O')
	{
	    if (*++s == 'l' || *s == 'L')
	    {
		if (*++s == ' ') /* .dol */
		    res = &ss->dollar;
		else if (*s == 'l' || *s == 'L')
		    if (*++s == 'a' || *s == 'A')
			if (*++s == 'r' || *s == 'R')
			    if (*++s == ' ') /* .dollar */
				res = &ss->dollar;
	    }
	    else if (*s == 't' || *s == 'T')
		if (*++s == ' ') /* .dot */
		    res = &ss->dot;
	}
    }
    else if (*s == 'p')
    {
	if (*++s == 'o' && s[1] == 'p') /* .popis */
	{
	    popInterpreterState(ss);
	    res = s;
	}
	else if (*s == 'u' && s[1] == 's' && s[2] == 'h') /* .pushis */
	{
	    pushInterpreterState(ss);
	    res = s;
	}
    }
#endif

    if (res)
    {
	if (*s == '\0')
	    return ERROR_REQUIRED_ARG_MISSING;

	*res = s[1]; /* FIXME skip spaces */
	in->cur = in->len;
	return 0;
    }

    return ERROR_ACTION_NOT_KNOWN;
}
