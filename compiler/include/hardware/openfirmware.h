#ifndef HARDWARE_OPENFIRMWARE_H
#define HARDWARE_OPENFIRMWARE_H

#include <exec/lists.h>

struct OFWNode
{
    struct MinNode  on_node;
    char	   *on_name;
    struct MinList  on_children;
    struct MinList  on_properties;

    uint8_t	    on_storage[];
};

struct OFWProperty
{
    struct MinNode  op_node;
    char 	   *op_name;
    uint32_t	    op_length;
    void	   *op_value;

    uint8_t	    op_storage[];
};

#endif
