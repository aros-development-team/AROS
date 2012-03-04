/*
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

#include <aros/libcall.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/errno.h>

#include <kern/amiga_includes.h>

#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <api/allocdatabuffer.h>

#include <api/apicalls.h>
#include <api/amiga_kernvars.h>
#include <stdarg.h>

#include <bsdsocket/socketbasetags.h>

/*
 * va_set() macro is used to convert from AmigaOS-style arguments array
 * to standard SVR4 va_list structure. We just fabricate such a structure
 * where reg_save_area is already ended up and next argument will be
 * picked up from overflow_arg_area which is our array. Note that we
 * don't need va_start() and va_end() in this case
 */
#ifdef __PPC__
#define va_set(ap, args)	\
  ap->gpr = 8;			\
  ap->fpr = 8;			\
  ap->overflow_arg_area = args;
#endif

#ifdef __x86_64__
#define va_set(ap, args)	\
  ap->gp_offset = 48;		\
  ap->fp_offset = 304;		\
  ap->overflow_arg_area = args;
#endif

#ifdef __arm__
#define va_set(ap, args)	\
  ap.__ap = args;
#endif

#ifndef va_set
#define va_set(ap, args)	\
  ap = (va_list)args;
#endif

extern const char * const __sys_errlist[];
extern const int __sys_nerr;
extern const char * const h_errlist[];
extern const int h_nerr;
extern const char * const io_errlist[];
extern const short io_nerr;
extern const char * const sana2io_errlist[];
extern const short sana2io_nerr;
extern const char * const sana2wire_errlist[];
extern const short sana2wire_nerr;
extern STRPTR version;
extern struct kernel_var kvars[];

/*LONG Errno(
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH0(LONG, Errno,
   struct SocketBase *, libPtr, 27, UL)
{
  AROS_LIBFUNC_INIT
  return (LONG)readErrnoValue(libPtr);
  AROS_LIBFUNC_EXIT
}

LONG __SetErrnoPtr(VOID *err_p, UBYTE size, struct SocketBase *libPtr)
{
  if (size == 4 || size == 2 || size == 1) {
    if (size & 0x1 || !((IPTR)err_p & 0x1)) {	/* either odd size or address even */
      libPtr->errnoSize = size;
      libPtr->errnoPtr = err_p;
      return 0;
    }
  }

  writeErrnoValue(libPtr, EINVAL);
  return -1;
}

AROS_LH2(LONG, SetErrnoPtr,
   AROS_LHA(VOID *, err_p, A0),
   AROS_LHA(UBYTE, size, D0),
   struct SocketBase *, libPtr, 28, UL)
{
  AROS_LIBFUNC_INIT
  return __SetErrnoPtr(err_p, size, libPtr);
  AROS_LIBFUNC_EXIT
}

/*VOID SAVEDS Syslog(
   REG(d0, ULONG pri),
   REG(a0, const char *fmt),
   REG(a1, va_list ap),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH3(VOID, Syslog,
   AROS_LHA(ULONG, pri, D0),
   AROS_LHA(const char *, fmt, A0),
   AROS_LHA(IPTR *, ap, A1),
   struct SocketBase *, libPtr, 43, UL)
{
  AROS_LIBFUNC_INIT
  int saved_errno;
  char fmt_cpy[1024];
  char tag_cpy[256];
  va_list _ap;
  register char *p = fmt_cpy;
  char *t = NULL;
  char *t1;

  CHECK_TASK_VOID();

  va_set(_ap, ap);

  /* check for invalid bits or no priority set */
  if (!LOG_PRI(pri)) {
    DSYSLOG(KPrintF("Priority is zero\n");)
    return;
  }
  if (pri &~ (LOG_PRIMASK|LOG_FACMASK)) {
    DSYSLOG(KPrintF("Bad bits in priority/facility value 0x%08lx\n", pri);)
    return;
  }
  if (!(LOG_MASK(LOG_PRI(pri)) & libPtr->LogMask)) {
    DSYSLOG(KPrintF("Priority value %lu is masked out\n", LOG_PRI(pri));)
    return;
  }

  saved_errno = readErrnoValue(libPtr);
  if (saved_errno >= __sys_nerr)
    saved_errno = 0;						/* XXX */

  /* set default facility if none specified */
  if ((pri & LOG_FACMASK) == 0)
    pri |= libPtr->LogFacility;

  if (libPtr->LogTag) {
    DSYSLOG(KPrintF("LogTag: %s", libPtr->LogTag);)
    t = tag_cpy;
    t1 = tag_cpy;
    t1 += sprintf(t1, "%s", libPtr->LogTag);
  }
  DSYSLOG(else KPrintF("LogTag: <NULL>");)
  DSYSLOG(KPrintF(", message: %s\n", fmt);)
  if (libPtr->LogStat & LOG_PID) {
    if (!t) {
      t = tag_cpy;
      t1 = tag_cpy;
    }
    sprintf(t1, "[%p]", libPtr->thisTask);
  }

  /* 
   * Build the new format string
   */
/*if (libPtr->LogTag) {
    p += sprintf(p, libPtr->LogTag);
  }
  if (libPtr->LogStat & LOG_PID) {
    p += sprintf(p, "[%lx]", libPtr->thisTask);
  }
  if (libPtr->LogTag) {
    *p++ = ':';
    *p++ = ' ';
  }*/
  /*
   * Substitute error message for %m.
   */
  {
    char ch;
    const char *t2;

    for (t1 = p; ch = *fmt; ++fmt) {
      if (ch == '%') {
	if (fmt[1] == '%') {
	  ++fmt;
	  *t1++ = ch;
	}
	else if (fmt[1] == 'm') {
	  ++fmt;
	  for (t2 = __sys_errlist[saved_errno]; *t1 = *t2++; ++t1)
	    ;
	  continue;
	}
      }
      *t1++ = ch;
    }
    *t1 = '\0';
  }
  vlog(pri, t, fmt_cpy, _ap);
  AROS_LIBFUNC_EXIT
}

/*VOID SAVEDS SetSocketSignals(
   REG(d0, ULONG sigintrmask),
   REG(d1, ULONG sigiomask),
   REG(d2, ULONG sigurgmask),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH3(VOID, SetSocketSignals,
   AROS_LHA(ULONG, sigintrmask, D0),
   AROS_LHA(ULONG, sigiomask, D1),
   AROS_LHA(ULONG, sigurgmask, D2),
   struct SocketBase *, libPtr, 22, UL)
{
  AROS_LIBFUNC_INIT
  CHECK_TASK_VOID();
  DSYSCALLS(log(LOG_DEBUG,"SetSocketSignals(0x%08lx, 0x%08lx, 0x%08lx) called", sigintrmask, sigiomask, sigurgmask);)
  /*
   * The operations below are atomic so no need to protect them
   */
  libPtr->sigIntrMask = sigintrmask;
  libPtr->sigIOMask = sigiomask;
  libPtr->sigUrgMask = sigurgmask;
  AROS_LIBFUNC_EXIT
}

/*LONG getdtablesize(
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH0(LONG, getdtablesize,
   struct SocketBase *, libPtr, 23, UL)
{
  AROS_LIBFUNC_INIT
  DSYSCALLS(log(LOG_DEBUG,"getdtablesize(): returned %ld", libPtr->dTableSize);)
  return (LONG)libPtr->dTableSize;
  AROS_LIBFUNC_EXIT
}

static int getLastSockFd(struct SocketBase *libPtr)
{
  int bit, lastmlong = (libPtr->dTableSize - 1) / NFDBITS;
  ULONG *smaskp, cmask, rmask;

  for (smaskp = (ULONG *)(libPtr->dTable + libPtr->dTableSize + lastmlong);
       lastmlong >= 0; smaskp--, lastmlong--)
    if (*smaskp)
      break;

  if (lastmlong < 0)
    return -1;

  cmask = *smaskp;
  if ((rmask = cmask & 0xFFFF0000)) { bit = 16; cmask = rmask; }
  else bit = 0;
  if ((rmask = cmask & 0xFF00FF00)) { bit += 8; cmask = rmask; }
  if ((rmask = cmask & 0xF0F0F0F0)) { bit += 4; cmask = rmask; }
  if ((rmask = cmask & 0xCCCCCCCC)) { bit += 2; cmask = rmask; }
  if ((rmask = cmask & 0xAAAAAAAA)) bit += 1;

  return lastmlong * 32 + bit;
}

/*
 * Set size of descriptor tab|e
 */
static LONG
setdtablesize(struct SocketBase * libPtr, UWORD size)
{
  
  LONG oldsize = (LONG)libPtr->dTableSize;
  LONG copysize;
  struct socket ** dTable;
  int olddmasksize, copydmasksize, dmasksize;
  
  if (size < oldsize) {
    int i;

    if ((i = getLastSockFd(libPtr)) > size)
      size = i + 1;
    copysize = size;
  }
  else
    copysize = oldsize;

  olddmasksize	= (oldsize - 1) /  NFDBITS + 1;
  copydmasksize	= (copysize - 1) / NFDBITS + 1;
  dmasksize 	= (size - 1) /	   NFDBITS + 1;
  
  if ((dTable = AllocMem(size * sizeof (struct socket *) +
			 dmasksize * sizeof (fd_mask),
			 MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    return oldsize;

  aligned_bcopy(libPtr->dTable, dTable, copysize * sizeof (struct socket *));
  aligned_bcopy(libPtr->dTable + oldsize, dTable + size, 
		copydmasksize * sizeof (fd_mask));
  
  FreeMem(libPtr->dTable, oldsize * sizeof (struct socket *) +
	  olddmasksize * sizeof (fd_mask));
  
  libPtr->dTable = dTable;
  libPtr->dTableSize = size;
  
  return size;
}

#ifdef __GNUC__
/* GNU C 4.x and higher really don't like to
 * have the l-value type-punned
 */
#define IPTR_ASSIGN(s,v) s = (__typeof__(s))(v)
#define LONG_ASSIGN(s,v) IPTR_ASSIGN(s, v)
#else
#define IPTR_ASSIGN(s, v) *(IPTR *)(s) = v
#define LONG_ASSIGN(s, v) *(LONG *)(s) = v
#endif

#define CASE_IPTR(code, baseField)\
 case (code << SBTB_CODE): /* get */ \
  *tagData = (IPTR)libPtr->baseField;\
  break;\
 case (code << SBTB_CODE) | SBTF_SET: /* set */\
  IPTR_ASSIGN(libPtr->baseField, *(tagData));\
  break

#define CASE_LONG(code, baseField)\
 case (code << SBTB_CODE): /* get */ \
  *tagData = (LONG)libPtr->baseField;\
  break;\
 case (code << SBTB_CODE) | SBTF_SET: /* set */\
  LONG_ASSIGN(libPtr->baseField, *(tagData));\
  break

#define CASE_WORD(code, baseField)\
 case (code << SBTB_CODE): /* get */ \
  *tagData = (ULONG)libPtr->baseField;\
  break;\
 case (code << SBTB_CODE) | SBTF_SET: /* set */\
  *(UWORD *)&libPtr->baseField = (UWORD)*tagData;\
  break

#define CASE_BYTE(code, baseField)\
 case (code << SBTB_CODE): /* get */ \
  *tagData = (ULONG)libPtr->baseField;\
  break;\
 case (code << SBTB_CODE) | SBTF_SET: /* set */\
  *(UBYTE *)&libPtr->baseField = (UBYTE)*tagData;\
  break

#define CASE_FLAG(code, flag)\
  case (code << SBTB_CODE): /* get */ \
    *tagData = ((ULONG)libPtr->flags & (flag)) != 0;\
    break;\
  case (code << SBTB_CODE) | SBTF_SET: /* set */\
    if (*tagData) \
      *(UBYTE *)&libPtr->flags |= (flag);\
    else \
      *(UBYTE *)&libPtr->flags &= ~(flag);\
    break

/****** bsdsocket.library/SocketBaseTagList ***********************************

    NAME
         SocketBaseTagList - Set/Get SocketBase attributes.

    SYNOPSIS
         #include <amitcp/socketbasetags.h>

         ULONG SocketBaseTagList(struct TagItem *);

         error = SocketBaseTagList(taglist)
         D0                        A0

         error = SocketBaseTags(ULONG tag, ...);

    FUNCTION
        Set or get a list of (mostly) SocketBase instance dependent attributes
        from the AmiTCP.

    INPUTS
        These functions expect as their argument a standard tag list, one or
        several array of struct TagItem as defined in the header file
        <utility/tagitem.h>. The structure contains two fields: ti_Tag and
        ti_Data.  The ti_Tag field contains tag code, which determines what
        the SocketBaseTagList() should do with its argument, the ti_Data
        field.

        The include file <amitcp/socketbasetags.h> defines macros for base tag
        code values.  Base tag code macros begin with `SBTC_' (as Socket Base
        Tag Code).  The base tag value defines what data item the tag item
        refers.

        The tag code contains other information besides the referred data
        item.  It controls, whether the SocketBaseTagList() should set or get
        the appropriate parameter, and whether the argument of the tag in
        question is passed by value or by reference.  

        The include file <amitcp/socketbasetags.h> defines the following
        macros, which are used to construct the ti_Tag values from the base
        tag codes:

             SBTM_GETREF(code) - get by reference
             SBTM_GETVAL(code) - get by value
             SBTM_SETREF(code) - set by reference
             SBTM_SETVAL(code) - set by value

        If the actual data is stored directly into the ti_Data field, you
        should use the 'by value' macros, SBTM_GETVAL() or SBTM_SETVAL().
        However, if the ti_Data field contains a pointer to actual data, you
        should use the 'by reference' macros, SBTM_GETREF() or SBTM_SETREF().
        In either case the actual data should always be a LONG aligned to even
        address.

        According the used tag naming scheme a tag which has "PTR" suffix
        takes an pointer as its argument.  Don't mix the pointer arguments
        with 'by reference' argument passing.  It is possible to pass a
        pointer by reference (in which case the ti_Data is a pointer to the
        actual pointer).

        The list of all defined base tag codes is as follows:

             SBTC_BREAKMASK       Tag data contains the INTR signal mask.  If
                                  the calling task receives a signal in the
                                  INTR mask, the AmiTCP interrupts current
                                  function calls and returns with the error
                                  code EINTR.  The INTR mask defaults to the
                                  CTRL-C signal (SIGBREAKF_C, bit 12).

             SBTC_DTABLESIZE      Socket Descriptor Table size. This
                                  defaults to 64.

             SBTC_ERRNO           The errno value. The values are defined in
                                  <sys/errno.h>.

             SBTC_ERRNOBYTEPTR
             SBTC_ERRNOWORDPTR
             SBTC_ERRNOLONGPTR
             SBTC_ERRNOPTR(size)  Set (only) the pointer to the errno
                                  variable defined by the program.  AmiTCP
                                  defines a value for this by default, but
                                  the application must set the pointer (and
                                  the size of the errno) with one of these
                                  tags, if it wishes to access the errno
                                  variable directly.

                                  The SBTC_ERRNOPTR(size) is a macro, which
                                  expands to one of the other (BYTE, WORD or
                                  LONG) tag codes, meaning that only 1, 2
                                  and 4 are legal size values.

                                  The netlib autoinit.c sets the errno
                                  pointer for the application, if the
                                  application is linked with it.

             SBTC_ERRNOSTRPTR     Returns an error string pointer describing
                                  the errno value given on input. You can not
                                  set the error message, only get is allowed.

                                  On call the ti_Data must contain the error
                                  code number.  On return the ti_Data is
                                  assigned to the string pointer.  (*ti_Data,
                                  if passed by reference).  See the file
                                  <sys/errno.h> for symbolic definitions for
                                  the errno codes.

             SBTC_FDCALLBACK      A callback function pointer for coordination
                                  of file descriptor usage between AmiTCP and
                                  link-library.  By default no callback is
                                  called and the value of this pointer is
                                  NULL.  The prototype for the callback
                                  function is:

                                  int error = fdCallback(int fd, int action);
                                      D0                     D0      D1

                                  where

                                  error -  0 for success or one of the error
                                           codes in <sys/errno.h> in case of
                                           error. The AmiTCP API function
                                           that calls the callback usually
                                           returns the 'error' back to the
                                           caller without any further
                                           modification.

                                  fd -     file descriptor number to take
                                           'action' on.

                                  action - one of the following actions
                                           (defined in
                                           <amitcp/socketbasetags.h>):

                                           FDCB_FREE -  mark the 'fd' as
                                                        unused on the link
                                                        library structure. If
                                                        'fd' represents a
                                                        file handled by the
                                                        link library, the
                                                        error (ENOTSOCK)
                                                        should be returned.

                                           FDCB_ALLOC - mark the 'fd'
                                                        allocated as a
                                                        socket.

                                           FDCB_CHECK - check if the 'fd' is
                                                        free. If an error is
                                                        returned, the 'fd' is
                                                        marked as used in the
                                                        AmiTCP/IP structures.

                                  The AmiTCP/IP calls the callback every time
                                  a socket descriptor is allocated or freed.
                                  AmiTCP/IP uses the FDCB_CHECK before actual
                                  allocation to check that it agrees with the
                                  link library on the next free descriptor
                                  number.  Thus the link library doesn't need
                                  to tell the AmiTCP if it creates a new file
                                  handle in open(), for example.

                                  See file _chkufb.c on the net.lib sources
                                  for an example implementation of the
                                  callback function for the SAS/C.

             SBTC_HERRNO          The name resolver error code value. Get
                                  this to find out why the gethostbyname()
                                  or gethostbyaddr() failed. The values are
                                  defined in <netdb.h>

             SBTC_HERRNOSTRPTR    Returns host error string for error number
                                  in tag data.  Host error is set on
                                  unsuccesful gethostbyname() and
                                  gethostbyaddr() calls. See the file
                                  <netdb.h> for the symbolic definitions for
                                  the herrno valus.

                                  Notes for the SBTC_ERRNOSTRPTR apply also
                                  to this tag code.

             SBTC_IOERRNOSTRPTR   Returns an error string for standard
                                  AmigaOS I/O error number as defined in the
                                  header file <exec/errors.h>.  Note that the
                                  error number taken by this tag code is
                                  positive, so the error codes must be
                                  negated (to be positive).  The positive
                                  error codes depend on the particular IO
                                  device, the standard Sana-II error codes
                                  can be retrieved by the tag code
                                  SBTC_S2ERRNOSTRPTR.

                                  Notes for the SBTC_ERRNOSTRPTR apply also
                                  to this tag code.

             SBTC_LOGFACILITY     Facility code for the syslog messages as
                                  defined in the header file <sys/syslog.h>.
                                  Defaults to LOG_USER.

             SBTC_LOGMASK         Sets the filter mask of the syslog
                                  messages.  By default the mask is 0xff,
                                  meaning that all messages are passed to the
                                  log system.

             SBTC_LOGSTAT         Syslog options defined in <sys/syslog.h>.

             SBTC_LOGTAGPTR       A pointer to a string which is used by
                                  syslog() to mark individual syslog
                                  messages. This defaults to NULL, but is
                                  set to the name of the calling program by
                                  the autoinit code in netlib:autoinit.c.
                                  This is for compatibility with pre-3.0
                                  programs.

             SBTC_S2ERRNOSTRPTR   Returns an error string for a Sana-II
                                  specific I/O error code as defined in the
                                  header file <devices/sana2.h>.

                                  Notes for the SBTC_ERRNOSTRPTR apply also
                                  to this tag code.

             SBTC_S2WERRNOSTRPTR  Returns an error string for a Sana-II Wire
                                  Error code as defined in the header file
                                  <devices/sana2.h>.

                                  Notes for the SBTC_ERRNOSTRPTR apply also
                                  to this tag code.

             SBTC_SIGIOMASK       The calling task is sent the signals
                                  specified by mask in tag data when
                                  asynhronous I/O is to be notified. The
                                  default value is zero, ie. no signal is
                                  sent.

             SBTC_SIGURGMASK      The calling task is sent the signals
                                  specified by mask in tag data when urgent
                                  data for the TCP arrives. The default value
                                  is zero, ie. no signal is sent.

    RESULT 
        Returns 0 on success, and a (positive) index of the failing tag on
        error.  Note that the value 1 means _first_ TagItem, 2 the second one,
        and so on.  The return value is NOT a C-language index, which are 0
        based.

    EXAMPLES
        To be written, see net.lib sources for various examples.

    NOTES

    BUGS
        None known.

    SEE ALSO
        <netinclude:amitcp/socketbasetags.h>, <include:utility/tagitem.h>

*****************************************************************************
*
*/
#ifdef notyet
/*
             SBTC_COMPAT43        Tag data is handled as boolean.  If it is
                                  true, AmiTCP/IP uses 4.3BSD compatible
                                  sockaddr structure for this application.

                                  The unreleased AS225r2 uses also 4.3BSD-
                                  compatible sockaddr structures.

*/
#endif

/*ULONG SAVEDS SocketBaseTagList(
   REG(a0, struct TagItem *tags),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH1(ULONG, SocketBaseTagList,
   AROS_LHA(struct TagItem *, tags, A0),
   struct SocketBase *, libPtr, 49, UL)
{
  AROS_LIBFUNC_INIT
  ULONG errIndex = 1;
  IPTR  tag;
  IPTR  *tagData;
  short tmp;
  UWORD utmp;

  static const char * const strErr = "Errlist lookup error";

  CHECK_TASK();

  while((tag = tags->ti_Tag) != TAG_END) {
    if (tag & TAG_USER) {		/* TAG_USER is the sign bit */
      /* get pointer to the actual data */
      tagData = ((UWORD)tag & SBTF_REF) ?
	(IPTR *)tags->ti_Data : &tags->ti_Data;

#ifdef DEBUG_EVENTS
     if (((UWORD)tag & ~(SBTF_REF)) == ((SBTC_SIGEVENTMASK << SBTB_CODE) | SBTF_SET))
	log(LOG_DEBUG,"SBTC_SIGEVENTMASK set to 0x%08lx", *tagData);
#endif

      switch ((UWORD)tag & ~SBTF_REF) {

      CASE_LONG( SBTC_BREAKMASK,  sigIntrMask );

      CASE_LONG( SBTC_SIGIOMASK,  sigIOMask );

      CASE_LONG( SBTC_SIGURGMASK, sigUrgMask );

      CASE_LONG( SBTC_SIGEVENTMASK, sigEventMask );

      case (SBTC_ERRNO << SBTB_CODE): /* get */ 
	*tagData = (IPTR)readErrnoValue(libPtr);
	break;
      case (SBTC_ERRNO << SBTB_CODE) | SBTF_SET: /* set */
        writeErrnoValue(libPtr, *tagData);
	break;

      case (SBTC_HERRNO << SBTB_CODE): /* get */ 
	*tagData = (IPTR)*libPtr->hErrnoPtr;
	break;
      case (SBTC_HERRNO << SBTB_CODE) | SBTF_SET: /* set */
        *libPtr->hErrnoPtr = (IPTR) *tagData;
	break;

      case (SBTC_DTABLESIZE << SBTB_CODE): /* get */
	*tagData = (IPTR)libPtr->dTableSize;
	break;
      case (SBTC_DTABLESIZE << SBTB_CODE) | SBTF_SET: /* set */
	if ((tmp = (WORD)*tagData) > 0)
	  setdtablesize(libPtr, tmp);
	break;

      CASE_IPTR( SBTC_FDCALLBACK,   fdCallback );

      CASE_BYTE( SBTC_LOGSTAT,      LogStat );

      CASE_IPTR( SBTC_LOGTAGPTR,    LogTag );
      
      case (SBTC_LOGFACILITY << SBTB_CODE): /* get */
	*tagData = (ULONG)libPtr->LogFacility;
	break;
      case (SBTC_LOGFACILITY << SBTB_CODE) | SBTF_SET: /* set */
	if ((utmp = (UWORD)*tagData) != 0 && (utmp &~ LOG_FACMASK) == 0)
	  libPtr->LogFacility = utmp;
	break;

      case (SBTC_LOGMASK << SBTB_CODE): /* get */
	*tagData = (ULONG)libPtr->LogMask;
	break;
      case (SBTC_LOGMASK << SBTB_CODE) | SBTF_SET: /* set */
	if ((utmp = (UWORD)*tagData) != 0)
	  libPtr->LogMask = (UBYTE)utmp;
	break;

      case SBTC_ERRNOSTRPTR << SBTB_CODE:
	/* get index */
	utmp = (UWORD)*tagData;
	/* return string pointer */
	*tagData = (IPTR)((utmp >= __sys_nerr) ?
			   strErr : __sys_errlist[utmp]);
	break;
      case SBTC_HERRNOSTRPTR << SBTB_CODE:
	/* get index */
	utmp = (UWORD)*tagData;
	/* return string pointer */
	*tagData = (IPTR)((utmp >= h_nerr) ?
			   strErr : h_errlist[utmp]);
	break;
      case SBTC_IOERRNOSTRPTR << SBTB_CODE:
	/* get index */
	utmp = (UWORD)*tagData;
	/* return string pointer */
	*tagData = (IPTR)((utmp >= io_nerr) ? 
			   strErr : io_errlist[utmp]);
	break;
      case SBTC_S2ERRNOSTRPTR << SBTB_CODE:
	/* get index */
	utmp = (UWORD)*tagData;
	/* return string pointer */
	*tagData = (IPTR)((utmp >= sana2io_nerr) ?
			   strErr : sana2io_errlist[utmp]);
	break;
      case SBTC_S2WERRNOSTRPTR << SBTB_CODE:
	/* get index */
	utmp = (UWORD)*tagData;
	/* return string pointer */
	*tagData = (IPTR)((utmp >= sana2wire_nerr) ?
			   strErr : sana2wire_errlist[utmp]);
	break;

      case (SBTC_ERRNOBYTEPTR << SBTB_CODE) | SBTF_SET: /* set */
	if (__SetErrnoPtr((VOID *)*tagData, 1, libPtr) < 0)
	  return errIndex;
        break;
      case (SBTC_ERRNOWORDPTR << SBTB_CODE) | SBTF_SET: /* set */
	if (__SetErrnoPtr((VOID *)*tagData, 2, libPtr) < 0)
	  return errIndex;
        break;
      case (SBTC_ERRNOLONGPTR << SBTB_CODE) | SBTF_SET: /* set */
	if (__SetErrnoPtr((VOID *)*tagData, 4, libPtr) < 0)
	  return errIndex;
        break;

      CASE_IPTR( SBTC_HERRNOLONGPTR, hErrnoPtr );
      
      case (SBTC_RELEASESTRPTR << SBTB_CODE): /* get */
	*tagData = (IPTR)&version[6];
	break;

#ifdef notyet
      CASE_FLAG( SBTC_COMPAT43, SBFB_COMPAT43 );
#endif

      default:
	return errIndex;
      }
   }
    else {			/* TAG_USER not set */
      switch(tags->ti_Tag) {
      case TAG_IGNORE:
	break;
      case TAG_MORE:
	tags = (struct TagItem *)tags->ti_Data;
	errIndex++;
	continue;
      case TAG_SKIP:
	tags++; errIndex++;
	break;
      default:
        return errIndex;	/* fail */
      }
    }
    
    tags++; errIndex++;
  }
  return 0;
  AROS_LIBFUNC_EXIT
}

