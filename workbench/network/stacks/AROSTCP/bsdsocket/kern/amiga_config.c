/*
 * amiga_config.c --- Configuring AmiTCP/IP
 *
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
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
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <netdb.h>		/* pathnames */

#include <kern/amiga_gui.h>
#include <kern/amiga_includes.h>
#include <kern/amiga_config.h>
#include <kern/amiga_netdb.h>
#include <utility/tagitem.h>
#include <dos/rdargs.h>

#include <proto/dos.h>

#include "net/netdbpaths.h"
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

/* External functions */
int ultoa(unsigned long ul,char *buffer);

extern struct Task *AROSTCP_Task; /* referenced by sendbreak() */

/* Parsing error messages */
UBYTE ERR_UNKNOWN[]     = "Unknown command";
UBYTE ERR_ILLEGAL_VAR[] = "%s: unknown variable %s";
UBYTE ERR_ILLEGAL_IND[] = "%s: unknown index %s";
UBYTE ERR_SYNTAX[]      = "Syntax error";
UBYTE ERR_TOO_LONG[]    = "Result too long";
UBYTE ERR_MEMORY[]      = "Memory exhausted";
UBYTE ERR_NONETDB[]     = "No active net database";
UBYTE ERR_VALUE[]       = "Illegal value";
UBYTE ERR_NOWRITE[]     = "%s: Variable %s is not writeable";

/* Command-line template */
const TEXT cmd_template[] = "WITH/K,NOO=NOCONFIG/S,DEBUG/S";

/* Array of parsing functions. Note that the order is same as in the
 * enum keyword.
 */
var_f rexx_parse_funs[] = {
  getvalue,			/* KEY_QUERY */
  setvalue,			/* KEY_SET */
  readfile,			/* KEY_READ */
  parseroute,			/* KEY_ROUTE */
  do_netdb,			/* KEY_ADD */
  reset_netdb,			/* KEY_RESET */
  sendbreak			/* KEY_KILL */
  };

/*
 * Parse a Rexx command line
 */
LONG
parseline(struct CSource *csarg, UBYTE **errstrp, struct CSource *res)
{
  UBYTE Buffer[KEYWORDLEN];
  enum keyword keyword;

  /* Parse the command keyword */
  LONG item = ReadItem(Buffer, sizeof(Buffer), csarg);

  if (item == 0)
    return RETURN_OK;
  else if (item < 0) {
    *errstrp = ERR_SYNTAX;
    return RETURN_WARN;
  }
  
  if ((keyword = FindArg((UBYTE*)REXXKEYWORDS, Buffer)) < 0) {
    *errstrp = ERR_UNKNOWN;
    return RETURN_WARN;
  }

  return rexx_parse_funs[keyword](csarg, errstrp, res);
}

/* 
 * 'Parse' the "KILL" command
 */
LONG 
sendbreak(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  Signal(AROSTCP_Task, SIGBREAKF_CTRL_C);
  return RETURN_OK;
}

extern UBYTE *KW_VARS;
extern struct cfg_variable variables[];

/* 
 * Parse the "Query" commands
 */
LONG 
getvalue(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  UBYTE Buffer[KEYWORDLEN];
  WORD var, index;
  LONG vlen;
  UBYTE *value = NULL;

  Buffer[0] = '\0';

  while ((var = ReadItem(Buffer, sizeof(Buffer), args)) > 0) {
    if ((var = FindArg(KW_VARS, Buffer)) < 0 ||
	(variables[var].flags & VF_READ) != VF_READ) { 
      res->CS_CurChr = 0;
      csprintf(res, ERR_ILLEGAL_VAR, "getvalue", Buffer);
      *errstrp = res->CS_Buffer;
      return RETURN_WARN;
    } else {

      if (variables[var].flags & VF_TABLE) {
	if (ReadItem(Buffer, sizeof(Buffer), args) <= 0 ||
	   (index = FindArg((UBYTE *)variables[var].index, Buffer)) < 0) {
	  res->CS_CurChr = 0;
	  csprintf(res, ERR_ILLEGAL_IND, "getvalue", Buffer);
	  *errstrp = res->CS_Buffer;
	  return RETURN_WARN;
	} 
      } else {
	index = 0;
      }

      switch (variables[var].type) {
      case VAR_FUNC:
	if (variables[var].value) {
	  if (vlen = (*(var_f)(variables[var].value))(args, errstrp, res)) 
	    return vlen;
	} else {
	  *errstrp = ERR_ILLEGAL_VAR;
	  return RETURN_ERROR;
	}
	value = (char *)1; /* successful flag.. */
	continue; /* while() */
      case VAR_LONG:
      case VAR_FLAG:
	vlen = ultoa(((LONG*)variables[var].value)[index], Buffer);
	value = Buffer;
	break;
      case VAR_STRP:
	value = ((UBYTE **)variables[var].value)[index];
	vlen  = strlen(value);
	break;
      case VAR_INET:
	{ 
	  ULONG s_addr = 
	    ((struct in_addr *)variables[var].value)[index].s_addr;
	  vlen = sprintf(Buffer, "%ld.%ld.%ld.%ld", 	  
			 (s_addr>>24) & 0xff, (s_addr>>16) & 0xff, 
			 (s_addr>>8) & 0xff, s_addr & 0xff);
	  value = Buffer;
	}
	break;
      case VAR_ENUM:
	{
	  ULONG i = 0, nth;
	  STRPTR p;

	  nth = ((ULONG*)variables[var].value)[index];
	  /*
	   * search nth keyword from the template
	   */
	  value = (STRPTR)variables[var].notify;
	  while (*value && i < nth)
	    if (*value++ == ',')
	      i++;
	  if (i < nth) {	/* value not found */
	    *errstrp = ERR_VALUE;
	    return RETURN_ERROR;
	  }
	  /*
           * find the length of the answer
	   */
	  p = value;
	  while(*p && *p != '=' && *p != ',')
	    p++;

	  vlen = p - value;
	}
	break;
      }
      /* prepend by space? */
      if (res->CS_CurChr) 
	res->CS_Buffer[res->CS_CurChr++] = ' ';
      if (vlen + res->CS_CurChr > res->CS_Length) {
	*errstrp = ERR_TOO_LONG;
	return RETURN_ERROR;
      } 
      bcopy(value, res->CS_Buffer + res->CS_CurChr, (WORD)vlen);
      res->CS_CurChr += vlen;
    }
  }
  if (!value || var != ITEM_NOTHING) {
    *errstrp = ERR_SYNTAX;
    return RETURN_WARN;
  }
  res->CS_Buffer[res->CS_CurChr] = '\0';
  return RETURN_OK;
}

/* 
 * Parse the "Set" commands
 *
 * TODO: notifications for VAR_INET;
 */
LONG 
setvalue(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  UBYTE Buffer[KEYWORDLEN];
  LONG  BufLen = sizeof(Buffer);
  LONG vlen, item;
  WORD var, index;
  UBYTE *value;
  void *dp = NULL;		/* pointer to data item */

  Buffer[0] = '\0';

  while ((item = ReadItem(Buffer, BufLen, args)) > 0) {
    if ((var = FindArg(KW_VARS, Buffer)) < 0 ||
	(!(variables[var].flags & VF_WRITE) &&
	 (initialized || !(variables[var].flags & VF_CONF)))) { 
      res->CS_CurChr = 0;
      csprintf(res, (var < 0) ? ERR_ILLEGAL_VAR : ERR_NOWRITE, 
	       "setvalue", Buffer);
      *errstrp = res->CS_Buffer;
      return RETURN_WARN;
    } else {
      if (variables[var].flags & VF_TABLE) {
        if (ReadItem(Buffer, BufLen, args) <= 0 ||
	    (index = FindArg((UBYTE *)variables[var].index, Buffer)) < 0) {
	  res->CS_CurChr = 0;
	  csprintf(res, ERR_ILLEGAL_IND, "setvalue", Buffer);
	  *errstrp = res->CS_Buffer;
	  return RETURN_WARN;
	} 
      } else {
	index = 0;
      }

      if (variables[var].type != VAR_FLAG &&
	  *CURRENT(args) == '=')
	args->CS_CurChr++;

      /* If dp is different for a type, 
       * it must be calculated in the case statement 
       */
      dp = (void *)((LONG *)variables[var].value + index);

      switch (variables[var].type) {
      case VAR_FUNC:
	if (variables[var].notify) {
	  if (vlen = (*(var_f)(variables[var].notify))(args, errstrp, res)) 
	    return vlen;
	} else {
	  *errstrp = ERR_ILLEGAL_VAR;
	  return RETURN_ERROR;
	}
	break;
      case VAR_LONG:
	if ((vlen = StrToLong(CURRENT(args), &item)) <= 0) 
	  goto reterr;
	if (variables[var].notify) 
	  if (!(*variables[var].notify)(dp, item)) {
	    *errstrp = ERR_VALUE;
	    return RETURN_WARN;
	  } 
	*(LONG*)dp = item;
	args->CS_CurChr += vlen;
	break;
      case VAR_STRP:
	if (ReadItem(Buffer, BufLen, args) <= 0)
	  goto reterr;
	vlen  = strlen(Buffer) + 1;
	value = bsd_malloc(vlen, M_CFGVAR, M_WAITOK);
	if (!value) {
	  *errstrp = ERR_MEMORY;
	  return RETURN_ERROR;
	}
	strcpy(value, Buffer);
	if (variables[var].notify) 
	  if (!(*variables[var].notify)(dp, (LONG) value)) {
	    bsd_free(value, M_CFGVAR);
	    *errstrp = ERR_VALUE;
	    return RETURN_WARN;
	  }
	if (variables[var].flags & VF_FREE) {
	  bsd_free(*(UBYTE **)dp, M_CFGVAR); 
	}
	*(UBYTE **)dp = value;
	variables[var].flags |= VF_FREE;
	break;
      case VAR_INET:
	/* Currently, nameservice cannot be used */
	if (ReadItem(Buffer, BufLen, args) <= 0)
	  goto reterr;
	if (!__inet_aton(Buffer, (struct in_addr *)dp))
	  goto reterr;
	break;
      case VAR_ENUM:
	if (ReadItem(Buffer, BufLen, args) <= 0)
	  goto reterr;
	/*
	 * Match the item against the template. The value is the index of
	 * the matching keyword, if one is found.
	 */
	if ((vlen = FindArg((STRPTR)variables[var].notify, Buffer)) < 0)
	  goto reterr;
	/*
	 * Set the new value. Note that there is no notify function, since
	 * the notify field was used for the template.
	 */
	*(LONG *)dp = vlen;
	break;
      }
    }
  }

  if (item != ITEM_NOTHING) {
  reterr:      
    *errstrp = ERR_SYNTAX;
    return RETURN_WARN;
  }

#define DONE "Done."
  bcopy(DONE, res->CS_Buffer + res->CS_CurChr, sizeof(DONE));
  res->CS_CurChr += sizeof(DONE);
  res->CS_Buffer[res->CS_CurChr] = '\0';
  return RETURN_OK;
}

/*
 * SET WITH a file
 */
LONG read_sets(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  UBYTE * buf = res->CS_Buffer;

  if (ReadItem(buf, res->CS_Length, args) < 0) {
    *errstrp = ERR_SYNTAX;
    return RETURN_WARN;
  } else {
    return parsefile(buf, errstrp, res);
  }
}

/*
 * Parse a 'WITH' command
 */
LONG 
readfile(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
  *errstrp = "readfile() is currently unimplemented\n";
  return RETURN_WARN;
}

#define CMDLINETEMP "WITH/K,NOO=NOCONFIG/S,DEBUG/S"

#define CL_WITH   0
#define CL_NOCFG  1
#define CL_DEBUG  2		/* Currently default, does nothing */
#define CL_SIZE   3

/*
 * Read command file 
 */
LONG 
parsefile(UBYTE const *name, UBYTE **errstrp, struct CSource *res)
{
  LONG retval = RETURN_OK, ioerr = 0;
  struct CSource arg;
  int line = 0;
  BPTR fh;
  UBYTE *buf;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_config.c) parsefile('%s')\n",name));
#endif

  D(Printf("Loading config file: %s\n",name);)
  buf = AllocMem(CONFIGLINELEN, MEMF_PUBLIC);

  if (buf) {
    arg.CS_Buffer = buf;
    if (fh = Open(name, MODE_OLDFILE)) {
      while (FGets(fh, buf, CONFIGLINELEN)) {
        line++;
        if (*buf == '#')
          continue;
        arg.CS_Length = strlen(buf);
        arg.CS_CurChr = 0;
        retval = setvalue(&arg, errstrp, res);

        if (retval == RETURN_OK)
          continue;
        if (retval != RETURN_WARN) { /* severe error */
          error_request("Fatal configuration error in file %s at line %ld, col %ld\n%s\nAROSTCP will quit", name, line, arg.CS_CurChr, *errstrp);
          break;
        }

	/* Print the error to the "stdout" */
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_config.c) parsefile: %s: line %ld, col %ld: %s",
	       name, line, arg.CS_CurChr, *errstrp));
#endif
	    error_request("Configuration error in file %s at line %ld, col %ld\n%s", name, line, arg.CS_CurChr, *errstrp);
	    retval = RETURN_OK;
      }
      /* Check file error */ 
      ioerr = IoErr();

      Close(fh);
    } else {
      ioerr = IoErr();
    }

    if (ioerr) {
      Fault(ioerr, name, res->CS_Buffer, res->CS_Length);
      *errstrp = res->CS_Buffer;
      retval = RETURN_ERROR;
    }
    if (!fh)
      error_request("Unable to open configuration file\n%s", *errstrp);

    FreeMem(buf, CONFIGLINELEN);
  } else {
    *errstrp = ERR_MEMORY;
    retval = RETURN_FAIL;
  }

  return retval;
}

/* 
 * Read command line arguments and configuration file
 */ 
BOOL
readconfig(void)
{
  UBYTE result[REPLYBUFLEN + 1]; /* for error returns */
  struct CSource res;
  struct RDArgs *rdargs = NULL;
  IPTR args[CL_SIZE] = { 0 };
  UBYTE *errstr;
  LONG error = 0;
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_config.c) readconfig()\n"));
#endif

  res.CS_Buffer = result;      
  res.CS_Length = sizeof(result); 
  res.CS_CurChr = 0;

  /* Parse command line arguments, if any */
  rdargs = ReadArgs(cmd_template, args, NULL);

  if (!rdargs) {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_config.c) readconfig: Bad Args\n"));
#endif
    Printf("Argument error. Template: %s\n", cmd_template);
    return FALSE;
  }

  if (!args[CL_NOCFG]) 
    /* Read default configuration file */
    error = parsefile(config_path, &errstr, &res);

  if (!error && args[CL_WITH])
    /* Read given file */
    error = parsefile((STRPTR)args[CL_WITH], &errstr, &res);
  
  FreeArgs(rdargs);

  if (error)
  {
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_config.c) readconfig: TCP/IP Configuration: %s\n", errstr));
#endif
    Printf("TCP/IP Configuration: %s\n", errstr);
  }

  return ((BOOL)!error);
}

#if 0
/*
 * The order of following keywords is selected to reflect the order of
 * route message numbers in <net/route.h>, so DO NOT CHANGE THE ORDER.
 */
static STRPTR KW_ROUTE_CMDS = 
  "RESET,ADD,DELETE,CHANGE,GET";

static STRPTR ROUTE_TEMPLATE =
  "NET/S,HOST/S,DESTINATION/A,GATEWAY/A,INTERFACE/S,HCNT=HOPCOUNT/K/N";

enum route_template
{ ROUTE_NET, ROUTE_HOST, ROUTE_DESTINATION, ROUTE_GATEWAY, ROUTE_INTERFACE,
  ROUTE_HOPCOUNT, ROUTE_TEMPLATE_SIZE };

#endif /* 0 */

/*
 * Parse route command
 */
LONG 
parseroute(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_config.c) parseroute()\n"));
#endif
#if 1
  *errstrp = "ROUTE not implemented.\n";
  return RETURN_FAIL;
#else
  UBYTE Buffer[KEYWORDLEN];
  LONG  BufLen = sizeof(Buffer);
  struct RDArgs *rdargs;
  IPTR argArray[ROUTE_TEMPLATE_SIZE] = { 0 };
  LONG vlen, item;
  WORD index;
  UBYTE *value;
  void *dp = NULL;		/* pointer to data item */
  int req, flags = 0;
  struct sockaddr_in destination, gateway, netmask;
  int error;

  Buffer[0] = '\0';

  if ((item = ReadItem(Buffer, BufLen, args)) <= 0 
      || (req = FindArg(KW_ROUTE_CMDS, Buffer)) < 0) {
    *errstrp = ERR_SYNTAX;
    return RETURN_FAIL;
  }

  if (req == 0)	{		/* RESET, delete all routes */
    *errstrp = "route reset is currently unimplemented\n";
    return RETURN_FAIL;
  }

  if (req == RTM_CHANGE) {
    *errstrp = "route change is currently unimplemented\n";
    return RETURN_FAIL;
  }
  if (req == RTM_GET) {
    *errstrp = "route get is currently unimplemented\n";
    return RETURN_FAIL;
  }
  
  /*
   * Initialize the RDArgs structure for ReadArgs()
   */
  rdargs = AllocDosObjectTags(DOS_RDARGS, TAG_END);
  if (rdargs == NULL) {
    *errstrp = ERR_MEMORY;
    return RETURN_FAIL;
  }

  rdargs->RDA_Source = *args;
  rdargs->RDA_DAList = NULL;
  rdargs->RDA_Buffer = NULL;
  rdargs->RDA_BufSiz = 0;
  rdargs->RDA_ExtHelp = NULL;
  rdargs->RDA_Flags = 0;

  if (ReadArgs(ROUTE_TEMPLATE, argArray, rdargs) == NULL
      || (argArray[ROUTE_HOST] && argArray[ROUTE_NET])) {
    FreeDosObject(DOS_RDARGS, rdargs);
    *errstrp = ERR_SYNTAX;
    return RETURN_FAIL;
  }
  
  /*
   * Find destination host/network
   */
  /* INCOMPLETE */

  if (!argArray[ROUTE_INTERFACE])
    flags |= RTF_GATEWAY;
  if (argArray[ROUTE_HOST])
    flags |= RTF_HOST;

#if 0
  error = rtrequest(req, &destination, &gateway,
		    (flags & RTF_HOST) ? NULL : &netmask,
		    flags, (struct rtentry **)0);
#endif

  FreeArgs(rdargs);
  FreeDosObject(DOS_RDARGS, rdargs);

  return RETURN_OK;
#endif
}
