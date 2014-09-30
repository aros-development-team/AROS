/*
    Copyright © 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RTAS_PRIVATE_H_
#define RTAS_PRIVATE_H_

#include <exec/nodes.h>
#include <inttypes.h>

typedef struct {
	uint32_t	token;
	uint32_t	nargs;
	uint32_t	nret;
	uint32_t	args[16];
	uint32_t	*rets;
} rtas_args_t;

struct RTASBase {
	struct Node	rtas_Node;
	void 		*rtas_base;
	int 		(*rtas_entry)(rtas_args_t *args, void *base);
	rtas_args_t	rtas_args;
	void		*KernelBase;
	void		*OpenFirmwareBase;
};

#endif /* RTAS_PRIVATE_H_ */
