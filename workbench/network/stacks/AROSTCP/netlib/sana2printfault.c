/* $Id$
 *
 *      sana2printfault.c - print SANA-II error message
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

#include <devices/sana2.h>
#include <net/sana2errno.h>
#include <proto/dos.h>

#include "conf.h"

/****** sana2.lib/sana2PrintFault *********************************************

    NAME
	Sana2PrintFault - print SANA-II device error messages

    SYNOPSIS
	#include <devices/sana2.h>

	Sana2PrintFault(banner, ios2request)

	void Sana2PrintFault(const char *, struct IOSana2Req *)

    FUNCTION
	The Sana2PrintFault() function finds the error message corresponding
	to the error in the given SANA-II IO request and writes it, followed
	by a newline, to the Output().  If the argument string is non-NULL it
	is preappended to the message string and separated from it by a colon
	and space (`: ').  If string is NULL only the error message string is
	printed.

    SEE ALSO
	Sana2PrintFault()

*******************************************************************************
*/


void 
Sana2PrintFault(const char *banner, struct IOSana2Req *ios2)
{
  register WORD err = ios2->ios2_Req.io_Error;
  register ULONG werr = ios2->ios2_WireError;
  IPTR args[3];
  char * format;

  args[0] = (IPTR)banner; 

  if (err >= sana2io_nerr || -err > io_nerr) {
    args[1] = (IPTR)io_errlist[0];
  } else { 
    if (err < 0) 
      args[1] = (IPTR)io_errlist[-err];
    else 
      args[1] = (IPTR)sana2io_errlist[err];
  }
  if (werr == 0 || werr >= sana2wire_nerr) {
    if (banner != NULL)
      format = "%s: %s\n";
    else
      format = "%s\n";
  } else {
    if (banner != NULL)
      format = "%s: %s (%s)\n";
    else
      format = "%s (%s)\n";
    args[2] = (IPTR)sana2wire_errlist[werr];
  }

  VPrintf(format, banner != NULL ? args : args + 1);
}
