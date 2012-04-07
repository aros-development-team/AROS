/*
 *  Copyright © 2004-2012 The AROS Developmemt Team. All rights reserved.
 *  $Id$
 *
 *  C99 & POSIX.1-2008 header file limits.h
 */

#ifndef _LIMITS_H_
#define _LIMITS_H_ 1


/* C99 */
#include <aros/cpu.h> /* For __WORDSIZE */

/* Sizes of integer types <limits.h> */

/*
 *  Define ANSI <limits.h> for standard 32-bit words.
 *  We're assuming
 *  8-bit 'char',
 *  16-bit 'short int', and
 *  32-bit 'int' and 'long int'.
 */

/* FIXME: Are all these values 64 bit OK ? */

/* Number of bits for smallest object that is not a bit-field (byte) */
#define CHAR_BIT	8
/* minimum value for an object of type signed char */
#define SCHAR_MIN	(-128)
/* maximum value for an object of type signed char */
#define SCHAR_MAX	127
/* maximum value for an object of type unsigned char */
#define UCHAR_MAX	255
/* minimum value for an object of type char */
#define CHAR_MIN	SCHAR_MIN
/* maximum value for an object of type char */
#define CHAR_MAX	SCHAR_MAX
/* maximum number of bytes in a multibyte character, for any supported locale */
#define	MB_LEN_MAX	1	/* At the moment only "C" locale supported */
/* minimum value for an object of type short int */
#define SHRT_MIN	(-32768)
/* maximum value for an object of type short int */
#define SHRT_MAX	32767
/* maximum value for an object of type unsigned short int */
#define USHRT_MAX	65535
/* minimum value for an object of type int */
#define INT_MIN		(-INT_MAX - 1)
/* maximum value for an object of type int */
#define INT_MAX		2147483647
/* maximum value for an object of type unsigned int */
#define UINT_MAX	4294967295U
/* minimum value for an object of type long int */
#define LONG_MIN	(-LONG_MAX - 1L)
/* maximum value for an object of type long int */
#if __WORDSIZE == 64
#   define LONG_MAX	9223372036854775807L
#else
#   define LONG_MAX	2147483647L
#endif
/* maximum value for an object of type unsigned long int */
#if __WORDSIZE == 64
#   define ULONG_MAX    18446744073709551615UL
#else
#   define ULONG_MAX    4294967295UL
#endif
/* minimum value for an object of type long long int */
#define LLONG_MIN	(-0x7fffffffffffffffLL - 1)
/* maximum value for an object of type long long int */
#define LLONG_MAX	0x7fffffffffffffffLL
/* maximum value for an object of type unsigned long long int */
#define ULLONG_MAX	0xffffffffffffffffULL


/* POSIX.1-2008 */

/* FIXME: Check validity and ABI implications of defined constants here */

/*
 * The POSIX limits are quite complex. You can actually omit half these
 * definitions if you can't determine them at compile time. You are
 * supposed to use sysconf() to query the actual values.
 */

/* Runtime Invariant Values (Possibly Indeterminate) */
#define AIO_LISTIO_MAX                          _POSIX_AIO_LISTIO_MAX
#define AIO_MAX                                 _POSIX_AIO_MAX
#define AIO_PRIO_DELTA_MAX                      0
#define ARG_MAX                                 40960
#define ATEXIT_MAX                              32
#define CHILD_MAX                               _POSIX_CHILD_MAX
#define DELAYTIMER_MAX                          _POSIX_DELAYTIMER_MAX
#define HOST_NAME_MAX                           _POSIX_HOST_NAME_MAX
#define IOV_MAX                                 _XOPEN_IOV_MAX
#define LOGIN_NAME_MAX                          _POSIX_LOGIN_NAME_MAX
#define MQ_OPEN_MAX                             _POSIX_MQ_OPEN_MAX
#define MQ_PRIO_MAX                             _POSIX_MQ_PRIO_MAX
#define OPEN_MAX                                _POSIX_OPEN_MAX
#define PAGESIZE                                4096
#define PAGE_SIZE                               PAGESIZE
#define PTHREAD_DESTRUCTOR_ITERATIONS           _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#define PTHREAD_KEYS_MAX                        _POSIX_THREAD_KEYS_MAX
#define PTHREAD_STACK_MIN                       0
#define PTHREAD_THREADS_MAX                     _POSIX_THREAD_THREADS_MAX
#define RE_DUP_MAX                              _POSIX2_RE_DUP_MAX
#define RTSIG_MAX                               _POSIX_RTSIG_MAX
#define SEM_NSEMS_MAX                           _POSIX_SEM_NSEMS_MAX
#define SEM_VALUE_MAX                           _POSIX_SEM_VALUE_MAX
#define SIGQUEUE_MAX                            _POSIX_SIGQUEUE_MAX
#define SS_REPL_MAX                             _POSIX_SS_REPL_MAX
#define STREAM_MAX                              _POSIX_STREAM_MAX
#define SYMLOOP_MAX                             _POSIX_SYMLOOP_MAX
#define TIMER_MAX                               _POSIX_TIMER_MAX
#define TRACE_EVENT_NAME_MAX                    _POSIX_TRACE_EVENT_NAME_MAX
#define TRACE_NAME_MAX                          _POSIX_TRACE_NAME_MAX
#define TRACE_SYS_MAX                           _POSIX_TRACE_SYS_MAX
#define TRACE_USER_EVENT_MAX                    _POSIX_TRACE_USER_EVENT_MAX
#define TTY_NAME_MAX                            _POSIX_TTY_NAME_MAX
#define TZNAME_MAX                              _POSIX_TZNAME_MAX

/* Pathname Variable Values */
#define FILESIZEBITS                            32
#define LINK_MAX                                _POSIX_LINK_MAX
#define MAX_CANON                               _POSIX_MAX_CANON
#define MAX_INPUT                               _POSIX_MAX_INPUT
#define NAME_MAX                                _XOPEN_NAME_MAX
#define PATH_MAX                                _XOPEN_PATH_MAX
#define PIPE_BUF                                _POSIX_PIPE_BUF
#define POSIX_ALLOC_SIZE_MIN                    // FIXME
#define POSIX_REC_INCR_XFER_SIZE                // FIXME
#define POSIX_REC_MAX_XFER_SIZE                 // FIXME
#define POSIX_REC_MIN_XFER_SIZE                 // FIXME
#define POSIX_REC_XFER_ALIGN                    // FIXME
#define SYMLINK_MAX                             _POSIX_SYMLINK_MAX

/* Runtime Increasable Values */
#define BC_BASE_MAX                             _POSIX2_BC_BASE_MAX
#define BC_DIM_MAX                              _POSIX2_BC_DIM_MAX
#define BC_SCALE_MAX                            _POSIX2_BC_SCALE_MAX
#define BC_STRING_MAX                           _POSIX2_BC_STRING_MAX
#define CHARCLASS_NAME_MAX                      _POSIX2_CHARCLASS_NAME_MAX
#define COLL_WEIGHTS_MAX                        _POSIX2_COLL_WEIGHTS_MAX
#define EXPR_NEST_MAX                           _POSIX2_EXPR_NEST_MAX
#define LINE_MAX                                _POSIX2_LINE_MAX
#define NGROUPS_MAX                             _POSIX_NGROUPS_MAX
#define RE_DUP_MAX                              _POSIX2_RE_DUP_MAX

/* Maximum Values */
/* NOTIMPL _POSIX_CLOCKRES_MIN */

/* Minimum Values */
#define _POSIX_AIO_LISTIO_MAX                   2
#define _POSIX_AIO_MAX                          1
#define _POSIX_ARG_MAX                          4096
#define _POSIX_CHILD_MAX                        25
#define _POSIX_DELAYTIMER_MAX                   32
#define _POSIX_HOST_NAME_MAX                    255
#define _POSIX_LINK_MAX                         8
#define _POSIX_LOGIN_NAME_MAX                   9
#define _POSIX_MAX_CANON                        255
#define _POSIX_MAX_INPUT                        255
#define _POSIX_MQ_OPEN_MAX                      8
#define _POSIX_MQ_PRIO_MAX                      32
#define _POSIX_NAME_MAX                         32
#define _POSIX_NGROUPS_MAX                      16
#define _POSIX_OPEN_MAX                         20
#define _POSIX_PATH_MAX                         256
#define _POSIX_PIPE_BUF                         512
#define _POSIX_RE_DUP_MAX                       255
#define _POSIX_RTSIG_MAX                        8
#define _POSIX_SEM_NSEMS_MAX                    255
#define _POSIX_SEM_VALUE_MAX                    32767
#define _POSIX_SIGQUEUE_MAX                     32
#define _POSIX_SSIZE_MAX                        32767
#define _POSIX_SS_REPL_MAX                      4
#define _POSIX_STREAM_MAX                       8
#define _POSIX_SYMLINK_MAX                      255
#define _POSIX_SYMLOOP_MAX                      8
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS     4
#define _POSIX_THREAD_KEYS_MAX                  128
#define _POSIX_TIMER_MAX                        32
#define _POSIX_TRACE_EVENT_NAME_MAX             30
#define _POSIX_TRACE_NAME_MAX                   8
#define _POSIX_TRACE_SYS_MAX                    8
#define _POSIX_TRACE_USER_EVENT_MAX             32
#define _POSIX_TTY_NAME_MAX                     9
#define _POSIX_TZNAME_MAX                       6
#define _POSIX2_BC_BASE_MAX                     99
#define _POSIX2_BC_DIM_MAX                      2048
#define _POSIX2_BC_SCALE_MAX                    99
#define _POSIX2_BC_STRING_MAX                   1000
#define _POSIX2_CHARCLASS_NAME_MAX              14
#define _POSIX2_COLL_WEIGHTS_MAX                2
#define _POSIX2_EXPR_NEST_MAX                   32
#define _POSIX2_LINE_MAX                        2048
#define _POSIX2_RE_DUP_MAX                      255
#define _XOPEN_IOV_MAX                          16
#define _XOPEN_NAME_MAX                         255
#define _XOPEN_PATH_MAX                         1024

/* Numerical Limits */
#define LONG_BIT                                __WORDSIZE
#define SSIZE_MAX                               _POSIX_SSIZE_MAX
#define WORD_BIT                                32

/* Other Invariant Values */
#define NL_ARGMAX                               9
#define NL_LANGMAX                              14
#define NL_MSGMAX                               32767
#define NL_SETMAX                               255
#define NL_TEXTMAX                              _POSIX2_LINE_MAX
#define NZERO                                   0

#endif /* _LIMITS_H_ */
