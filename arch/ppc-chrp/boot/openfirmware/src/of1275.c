/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <of1275.h>
#include <stdarg.h>
#include <support.h>

static int32_t (*ofw_call)(void *args);

void *stdin;
void *stdout;
void *stderr;

void ofw_init(void *ofw)
{
	void *handle;

	ofw_call = ofw;

	handle = ofw_find_device("/chosen");

	if (handle)
	{
		ofw_get_prop(handle, "stdin", &stdin, sizeof(stdin));
		ofw_get_prop(handle, "stdout", &stdout, sizeof(stdout));
		stderr = stdout;
	}
}

int32_t ofw_test(const char *name)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		const char	*name;
		int32_t	missing;
	}
	a = {
			"test", 1, 1,
			name,
			-1
	};

	ofw_call(&a);

	return a.missing;
}

void * ofw_peer(void * phandle)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	phandle;

		void *	sibling_phandle;
	}
	args = {
			"peer", 1, 1,
			phandle,
			(void *)-1
	};

	ofw_call(&args);

	return args.sibling_phandle;
}

void * ofw_child(void * phandle)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	phandle;

		void *	child_phandle;
	}
	args = {
			"child", 1, 1,
			phandle,
			(void *)-1
	};

	ofw_call(&args);

	return args.child_phandle;
}

void * ofw_parent(void * phandle)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	phandle;

		void *	parent_phandle;
	}
	args = {
			"parent", 1, 1,
			phandle,
			(void *)-1
	};

	ofw_call(&args);

	return args.parent_phandle;
}

int32_t ofw_get_prop_len(void * phandle, const char *name)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	phandle;
		const char	*name;

		int32_t	proplen;
	}
	args = {
			"getproplen", 2, 1,
			phandle, name,
			-1
	};

	ofw_call(&args);

	return args.proplen;
}

int32_t ofw_get_prop(void * phandle, const char *name, void * buf, uint32_t buflen)
{
	struct args
	{
		char *service;
		int32_t nargs;
		int32_t nret;

		void * phandle;
		const char *name;
		void * buf;
		uint32_t buflen;

		int32_t size;
	}
	args = {
			"getprop", 4, 1,
			phandle, name, buf, buflen,
			-1
	};

	ofw_call(&args);

	return args.size;
}

int32_t ofw_next_prop(void * phandle, const char *previous, void * buf)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	phandle;
		const char	*previous;
		void *	buf;

		int32_t	flag;
	}
	args = {
			"nextprop", 3, 1,
			phandle, previous, buf,
			-1
	};

	ofw_call(&args);

	return args.flag;
}

int32_t ofw_set_prop(void * phandle, const char *name, void * buf, uint32_t buflen)
{
	struct args
	{
		char *service;
		int32_t nargs;
		int32_t nret;

		void * phandle;
		const char *name;
		void * buf;
		uint32_t buflen;

		int32_t size;
	}
	args = {
			"setprop", 4, 1,
			phandle, name, buf, buflen,
			-1
	};

	ofw_call(&args);

	return args.size;
}

void * ofw_find_device(char *dev)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		char	*dev;


		void *	phandle;
	}
	args = {
			"finddevice", 1, 1,
			dev,
			(void *)-1
	};

	ofw_call(&args);

	return args.phandle;
}

void * ofw_open(const char *dev)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		const char	*dev;

		void *	ihandle;
	}
	args = {
			"open", 1, 1,
			dev,
			(void *)-1
	};

	ofw_call(&args);

	return args.ihandle;
}

void ofw_close(void * ihandle)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	ihandle;
	}
	args = {
			"close", 1, 0,
			ihandle
	};

	ofw_call(&args);
}

int32_t ofw_read(void * ihandle, void * addr, uint32_t len)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	ihandle;
		void *	addr;
		uint32_t	len;

		int32_t	actual;
	}
	args = {
			"read", 3, 1,
			ihandle, addr, len,
			-1
	};

	if (ofw_call(&args))
		return -1;
	else
		return args.actual;
}

int32_t ofw_write(void * ihandle, void * addr, uint32_t len)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	ihandle;
		void *	addr;
		uint32_t	len;

		int32_t	actual;
	}
	args = {
			"write", 3, 1,
			ihandle, addr, len,
			-1
	};

	if (ofw_call(&args))
		return -1;
	else
		return args.actual;
}

int32_t ofw_seek(void * ihandle, uint32_t pos_hi, uint32_t pos_lo)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	ihandle;
		uint32_t	pos_hi;
		uint32_t	pos_lo;

		int32_t	status;
	}
	args = {
			"seek", 3, 1,
			ihandle, pos_hi, pos_lo,
			-1
	};

	ofw_call(&args);

	return args.status;
}

typedef struct __pool {
	struct __pool 		*next;
	char 		*first_free;
	uint32_t	length;
	uint32_t	free;
} pool_t;

pool_t *mempool = NULL;

void * __claim(uint32_t size)
{
	void *ptr = NULL;
	pool_t *p;

	if (!mempool) {
		mempool = ofw_claim(0, 4096, 4096);
		if (mempool != (void*)-1)
		{
			mempool->first_free = (char*)mempool + sizeof(pool_t);
			mempool->length = 4096;
			mempool->free = 4096 - sizeof(pool_t);
			mempool->next = NULL;
		}
	}

	if (size >= (4096 - sizeof(pool_t)))
	{
		ptr = ofw_claim(0, size, 4096);
		if (ptr == (void*)-1)
			ptr = NULL;
	}
	else
	{
		p = mempool;

		do
		{
			if (p->free >= size)
				break;

			p = p->next;
		} while(p);

		if (!p)
		{
			p = ofw_claim(0, 4096, 4096);
			if (p != (void*)-1)
			{
				p->next = mempool;
				p->first_free = (char*)p + sizeof(pool_t);
				p->length = 4096;
				p->free = 4096 - sizeof(pool_t);
				mempool = p;
			}
			else p = NULL;
		}

		if (p)
		{
			ptr = p->first_free;
			p->first_free += size;
			p->free -= size;
		}
	}

	return ptr;
}

void * ofw_claim(void * virt, uint32_t size, uint32_t align)
{
	struct
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	virt;
		uint32_t	size;
		uint32_t	align;

		void *	baseaddr;
	}
	args = {
			"claim", 3, 1,
			virt, size, align,
			(void *)-1
	};

	ofw_call(&args);

	return args.baseaddr;
}

void ofw_release(void * virt, uint32_t size)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		void *	virt;
		uint32_t	size;
	}
	args = {
			"release", 2, 0,
			virt, size
	};

	ofw_call(&args);
}

int32_t ofw_instantiate_rtas(void * ihandle, void * rtas_base, void **rtas_entry)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		char	*method;
		void *	ihandle;
		void *rtas_base;

		int32_t	callback;
		void *rtas_entry;
	}
	args = {
			"call-method", 3, 2,
			"instantiate-rtas", ihandle, rtas_base,
			-1, (void*)-1
	};

	ofw_call(&args);
	*rtas_entry = args.rtas_entry;
	return args.callback;
}

int32_t ofw_load(void * ihandle, void * addr, int32_t *size)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		char	*method;
		void *	ihandle;
		void *addr;

		int32_t	callback;
		int32_t	size;
	}
	args = {
			"call-method", 3, 2,
			"load", ihandle, addr,
			-1, -1
	};

	ofw_call(&args);
	*size = args.size;
	return args.callback;
}

int32_t ofw_interpret(const char *cmd)
{
	struct args
	{
		char	*service;
		int32_t	nargs;
		int32_t	nret;

		const char	*cmd;

		int32_t retval;
	}
	args = {
			"interpret", 1, 1,
			cmd,
			-1
	};

	ofw_call(&args);

	return args.retval;
}

int32_t ofw_rtas_token(const char *service)
{
	void *handle = ofw_find_device("/rtas");
	int32_t token;

	if (handle)
	{
		ofw_get_prop(handle, service, &token, sizeof(token));
	}

	return token;
}



int ofw_get_properties(void *phandle, ofw_node_t *node)
{
	char property[33];
	char *previous = NULL;
	ofw_property_t *prop = NULL;
	int retval = 0;

	while(ofw_next_prop(phandle, previous, property) > 0)
	{
		int name_len = strlen(property) + 1;
		int prop_len = ofw_get_prop_len(phandle, property);

//		printf("  property '%s'(%d) length %d\n", property, name_len, prop_len);

		prop = __claim(sizeof(ofw_property_t) + name_len + prop_len);
		if (!prop)
		{
			retval = -1;
			break;
		}

		prop->op_name = (char *)&prop->op_storage[0];
		prop->op_value = &prop->op_storage[name_len];

		memcpy(prop->op_name, property, name_len);
		prop->op_length = prop_len;
		ofw_get_prop(phandle, property, prop->op_value, prop_len);

		add_tail(&node->on_properties, &prop->op_node);
		previous = prop->op_name;
	}

	return retval;
}

int ofw_build_tree(void *phandle, ofw_node_t *node, int depth)
{
	int retval = 0;
	void *hnd;
	ofw_node_t *child = NULL;
	int i;

	for (i=0; i < depth; i++)
		printf("  ");

//	printf("ofw_build_tree(%p, %p('%s'), %d)\n", phandle, node, node->on_name, depth);

	/* Get properties of the node */
	ofw_get_properties(phandle, node);

	hnd = ofw_child(phandle);
	if (hnd) do
	{
		int name_len = ofw_get_prop_len(hnd, "name");

		if (name_len > 0)
			name_len++;

		child = __claim(sizeof(ofw_node_t) + name_len);

		if (!child)
		{
			retval = -1;
			break;
		}

		if (name_len > 1)
		{
			child->on_name = (char*)child->on_storage;
			ofw_get_prop(hnd, "name", child->on_name, name_len-1);
		}
		else
			child->on_name = NULL;

		new_list(&child->on_children);
		new_list(&child->on_properties);

		add_tail(&node->on_children, &child->on_node);

		retval = ofw_build_tree(hnd, child, depth + 1);

		if (retval)
			break;

		hnd = ofw_peer(hnd);
	} while(hnd);

	return retval;
}

ofw_node_t *ofw_scan_tree()
{
	void *p_root = ofw_peer(NULL);
	ofw_node_t *root = __claim(sizeof(ofw_node_t) + 2);

	if (root)
	{
		memcpy(root->on_storage, "/", 2);
		root->on_name = (char*)root->on_storage;
		new_list(&root->on_children);
		new_list(&root->on_properties);

		ofw_build_tree(p_root, root, 0);
	}

	return root;
}

/* Copy a string property into allocated memory */
char *ofw_GetString(void *handle, const char *prop)
{
    int32_t len = ofw_get_prop_len(handle, prop);
    char *str = ofw_claim(NULL, len + 1, 4);

    ofw_get_prop(handle, prop, str, 255);
    str[len] = 0;

    return str;
}
