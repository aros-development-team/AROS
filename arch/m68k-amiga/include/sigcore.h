#ifndef _SIGCORE_H
#define _SIGCORE_H

/*
 * Only exec_init needs this file.
 */

#   define PREPARE_INITIAL_FRAME(cc, sp, startpc) \
	do { \
		ctx->pc = (ULONG)startpc; \
	} while (0)


#endif /* _SIGCORE_H */
