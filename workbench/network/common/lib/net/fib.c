/* $Id$
 *
 *      fib.c - common fib buffer for stat() and chown()
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 */

/* DOS 3.0 and MuFS extensions to file info block */
#include "fibex.h"

struct FileInfoBlock __dostat_fib[1];

