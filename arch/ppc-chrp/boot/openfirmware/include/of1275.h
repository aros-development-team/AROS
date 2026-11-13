/*
 * of1275.h
 *
 *  Created on: Aug 12, 2008
 *      Author: misc
 */

#ifndef OF1275_H_
#define OF1275_H_

#include <inttypes.h>
#include <support.h>

typedef struct {
	node_t 	on_node;
	char 	*on_name;
	list_t	on_children;
	list_t	on_properties;

	uint8_t	on_storage[];
} ofw_node_t;

typedef struct {
	node_t		op_node;
	char 		*op_name;
	uint32_t	op_length;
	void		*op_value;

	uint8_t		op_storage[];
} ofw_property_t;

void ofw_init(void *ofw);
int32_t ofw_test(const char *name);
void *	ofw_peer(void *phandle);
void *	ofw_child(void *phandle);
void *	ofw_parent(void *phandle);
int32_t ofw_get_prop_len(void * phandle, const char *name);
int32_t ofw_get_prop(void * phandle, const char *name, void * buf, uint32_t buflen);
int32_t ofw_next_prop(void * phandle, const char *previous, void * buf);
int32_t ofw_set_prop(void * phandle, const char *name, void * buf, uint32_t buflen);
void *ofw_find_device(char *dev);
void *ofw_open(const char *dev);
void ofw_close(void * ihandle);
int32_t ofw_read(void * ihandle, void * addr, uint32_t len);
int32_t ofw_write(void * ihandle, void * addr, uint32_t len);
int32_t ofw_seek(void * ihandle, uint32_t pos_hi, uint32_t pos_lo);
void *ofw_claim(void * virt, uint32_t size, uint32_t align);
void ofw_release(void * virt, uint32_t size);
void * __claim(uint32_t size);

int32_t ofw_interpret(const char *cmd);

int32_t ofw_load(void *ihandle, void * addr, int32_t *size);
int32_t ofw_instantiate_rtas(void * ihandle, void * rtas_base, void **rtas_entry);

int32_t ofw_rtas_token(const char *service);

ofw_node_t *ofw_scan_tree();

char *ofw_GetString(void *handle, const char *prop);

#endif /* OF1275_H_ */
