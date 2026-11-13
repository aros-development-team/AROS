/*
 * of_intern.h
 *
 *  Created on: Oct 11, 2008
 *      Author: misc
 */

#ifndef OF_INTERN_H_
#define OF_INTERN_H_

#include <exec/nodes.h>
#include <exec/lists.h>
#include <inttypes.h>

typedef struct {
	struct MinNode	on_node;
	char 			*on_name;
	struct MinList	on_children;
	struct MinList	on_properties;

	uint8_t	on_storage[];
} of_node_t;

typedef struct {
	struct MinNode	op_node;
	char 			*op_name;
	uint32_t		op_length;
	void			*op_value;

	uint8_t			op_storage[];
} of_property_t;

struct OpenFirmwareBase {
    struct Node         of_Node;
    of_node_t			*of_Root;
};

#endif /* OF_INTERN_H_ */
