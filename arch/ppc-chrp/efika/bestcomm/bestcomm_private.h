/*
 * bestcomm_private.h
 *
 *  Created on: Mar 15, 2009
 *      Author: misc
 */

#ifndef BESTCOMM_PRIVATE_H_
#define BESTCOMM_PRIVATE_H_

#include <exec/nodes.h>
#include <asm/mpc5200b.h>
#include <inttypes.h>

struct BestCommBase {
	struct Node		bc_Node;
	bestcomm_t		*bc_BestComm;
	uint8_t			*bc_SRAM;
	uint32_t		*bc_SRAMFree;
	uint32_t		bc_SRAMSize;
	uint8_t			*bc_MBAR;
};

typedef struct {
	intptr_t	addr;
	intptr_t	size;
} reg_t;

#endif /* BESTCOMM_PRIVATE_H_ */
