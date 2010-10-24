/*
 * debug.h
 *
 *  Created on: Aug 13, 2008
 *      Author: misc
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <of1275.h>

#if DEBUG
#define D(x) (x)
#else
#define D(x) /* */
#endif

#define bug(...) printf(__VA_ARGS__)

#endif /* DEBUG_H_ */
