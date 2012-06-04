#ifndef HCCOMMON_H
#define HCCOMMON_H

/*
 *----------------------------------------------------------------------------
 *             Common includes for USB Host Controllers
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 *
 */

/* Macros to swap bit defines, constants & variables */
#define SWB(x) ((x+16) & 31)
#define SWC(x) ((x>>16)|((x & 0xffff)<<16))
#define SWW(x) ((x>>16)|(x<<16))
#define L2U(x) (x<<16)
#define U2L(x) (x>>16)

#define PID_IN              0x69
#define PID_OUT             0xe1
#define PID_SETUP           0x2d

#endif /* HCCOMMON_H */
