/*
 * BK Id: SCCS/s.of1275.c 1.6 05/18/01 15:16:42 cort
 */
/* Open Firmware Client Interface */


#include "of1275.h"


static int (*of_server)(void *) = (int(*)(void*))-1;

void
of_init(void *handler)
{
	of_server = (int(*)(void*))handler;
}


/* 6.3.2.1 Client interface */


int
of_test(const char *name, int *missing)
{
	int result;
	static of_test_service s;
	s.service = "test";
	s.n_args = 1;
	s.n_returns = 1;
	s.name = name;
	result = of_server(&s);
	*missing = s.missing;
	return result;
}


/* 6.3.2.2 Device tree */


int
of_peer(int phandle, int *sibling_phandle)
{
	int result;
	static of_peer_service s;
	s.service = "peer";
	s.n_args = 1;
	s.n_returns = 1;
	s.phandle = phandle;
	result = of_server(&s);
	*sibling_phandle = s.sibling_phandle;
	return result;
}

int
of_child(int phandle, int *child_phandle)
{
	int result;
	static of_child_service s;
	s.service = "child";
	s.n_args = 1;
	s.n_returns = 1;
	s.phandle = phandle;
	result = of_server(&s);
	*child_phandle = s.child_phandle;
	return result;
}

int
of_parent(int phandle, int *parent_phandle)
{
	int result;
	static of_parent_service s;
	s.service = "parent";
	s.n_args = 1;
	s.n_returns = 1;
	s.phandle = phandle;
	result = of_server(&s);
	*parent_phandle = s.parent_phandle;
	return result;
}

int
of_instance_to_package(int ihandle, int *phandle)
{
	int result;
	static of_instance_to_package_service s;
	s.service = "instance-to-package";
	s.n_args = 1;
	s.n_returns = 1;
	s.ihandle = ihandle;
	result = of_server(&s);
	*phandle = s.phandle;
	return result;
}

int
of_getproplen(int phandle, const char *name, int *proplen)
{
	int result;
	static of_getproplen_service s;
	s.service = "getproplen";
	s.n_args = 2;
	s.n_returns = 1;
	s.phandle = phandle;
	s.name = name;
	result = of_server(&s);
	*proplen = s.proplen;
	return result;
}

int
of_getprop(int phandle, const char *name, void *buf, int buflen, int *size)
{
	int result;
	static of_getprop_service s;
	s.service = "getprop";
	s.n_args = 4;
	s.n_returns = 1;
	s.phandle = phandle;
	s.name = name;
	s.buf = buf;
	s.buflen = buflen;
	result = of_server(&s);
	*size = s.size;
	return result;
}

int
of_nextprop(int phandle, const char *previous, void *buf, int *flag)
{
	int result;
	static of_nextprop_service s;
	s.service = "nextprop";
	s.n_args = 3;
	s.n_returns = 1;
	s.phandle = phandle;
	s.previous = previous;
	s.buf = buf;
	result = of_server(&s);
	*flag = s.flag;
	return result;
}

int
of_setprop(int phandle, const char *name, void *buf, int len, int *size)
{
	int result;
	static of_setprop_service s;
	s.service = "setprop";
	s.n_args = 4;
	s.n_returns = 1;
	s.phandle = phandle;
	s.name = name;
	s.buf = buf;
	s.len = len;
	result = of_server(&s);
	*size = s.size;
	return result;
}

int
of_canon(const char *device_specifier, void *buf, int buflen, int *length)
{
	int result;
	static of_canon_service s;
	s.service = "canon";
	s.n_args = 3;
	s.n_returns = 1;
	s.device_specifier = device_specifier;
	s.buf = buf;
	s.buflen = buflen;
	result = of_server(&s);
	*length = s.length;
	return result;
}

int
of_finddevice(const char *device_specifier, int *phandle)
{
	int result;
	static of_finddevice_service s;
	s.service = "finddevice";
	s.n_args = 1;
	s.n_returns = 1;
	s.device_specifier = device_specifier;
	result = of_server(&s);
	*phandle = s.phandle;
	return result;
}

int
of_instance_to_path(int ihandle, void *buf, int buflen, int *length)
{
	int result;
	static of_instance_to_path_service s;
	s.service = "instance-to-path";
	s.n_args = 3;
	s.n_returns = 1;
	s.ihandle = ihandle;
	s.buf = buf;
	s.buflen = buflen;
	result = of_server(&s);
	*length = s.length;
	return result;
}

int
of_package_to_path(int phandle, void *buf, int buflen, int *length)
{
	int result;
	static of_package_to_path_service s;
	s.service = "package-to-path";
	s.n_args = 3;
	s.n_returns = 1;
	s.phandle = phandle;
	s.buf = buf;
	s.buflen = buflen;
	result = of_server(&s);
	*length = s.length;
	return result;
}

/* int of_call_method(const char *method, int ihandle, ...); */


/* 6.3.2.3 Device I/O */


int
of_open(const char *device_specifier, int *ihandle)
{
	int result;
	static of_open_service s;
	s.service = "open";
	s.n_args = 1;
	s.n_returns = 1;
	s.device_specifier = device_specifier;
	result = of_server(&s);
	*ihandle = s.ihandle;
	return result;
}

int
of_close(int ihandle)
{
	int result;
	static of_close_service s;
	s.service = "close";
	s.n_args = 1;
	s.n_returns = 0;
	s.ihandle = ihandle;
	result = of_server(&s);
	return result;
}

int
of_read(int ihandle, void *addr, int len, int *actual)
{
	int result;
	static of_read_service s;
	s.service = "read";
	s.n_args = 3;
	s.n_returns = 1;
	s.ihandle = ihandle;
	s.addr = addr;
	s.len = len;
	result = of_server(&s);
	*actual = s.actual;
	return result;
}

int
of_write(int ihandle, void *addr, int len, int *actual)
{
	int result;
	static of_write_service s;
	s.service = "write";
	s.n_args = 3;
	s.n_returns = 1;
	s.ihandle = ihandle;
	s.addr = addr;
	s.len = len;
	result = of_server(&s);
	*actual = s.actual;
	return result;
}

int
of_seek(int ihandle, int pos_hi, int pos_lo, int *status)
{
	int result;
	static of_seek_service s;
	s.service = "seek";
	s.n_args = 3;
	s.n_returns = 1;
	s.ihandle = ihandle;
	s.pos_hi = pos_hi;
	s.pos_lo = pos_lo;
	result = of_server(&s);
	*status = s.status;
	return result;
}


/* 6.3.2.4 Memory */


int
of_claim(void *virt, int size, int align, void **baseaddr)
{
	int result;
	static of_claim_service s;
	s.service = "claim";
	s.n_args = 3;
	s.n_returns = 1;
	s.virt = virt;
	s.size = size;
	s.align = align;
	result = of_server(&s);
	*baseaddr = s.baseaddr;
	return result;
}

int
of_release(void *virt, int size)
{
	int result;
	static of_release_service s;
	s.service = "release";
	s.n_args = 2;
	s.n_returns = 0;
	s.virt = virt;
	s.size = size;
	result = of_server(&s);
	return result;
}


/* 6.3.2.5 Control transfer */


int
of_boot(const char *bootspec)
{
	int result;
	static of_boot_service s;
	s.service = "boot";
	s.n_args = 1;
	s.n_returns = 0;
	s.bootspec = bootspec;
	result = of_server(&s);
	return result;
}

int
of_enter(void)
{
	int result;
	static of_enter_service s;
	s.service = "enter";
	s.n_args = 0;
	s.n_returns = 0;
	result = of_server(&s);
	return result;
}

int
of_exit(void)
{
	int result;
	static of_exit_service s;
	s.service = "exit";
	s.n_args = 0;
	s.n_returns = 0;
	result = of_server(&s);
	return result;
}

/* int of_chain(void *virt, int size, void *entry, void *args, int len); */


/* 6.3.2.6 User interface */


/* int of_interpret(const char *arg, ...); */

int
of_set_callback(void *newfunc, void **oldfunc)
{
	int result;
	static of_set_callback_service s;
	s.service = "set-callback";
	s.n_args = 1;
	s.n_returns = 1;
	s.newfunc = newfunc;
	result = of_server(&s);
	*oldfunc = s.oldfunc;
	return result;
}

int
of_set_symbol_lookup(void *sym_to_value, void *value_to_sym)
{
	int result;
	static of_set_symbol_lookup_service s;
	s.service = "set-symbol-lookup";
	s.n_args = 2;
	s.n_returns = 0;
	s.sym_to_value = sym_to_value;
	s.value_to_sym = s.value_to_sym;
	result = of_server(&s);
	return result;
}


/* 6.3.2.7 Time */


int
of_milliseconds(int *ms)
{
	int result;
	static of_milliseconds_service s;
	s.service = "milliseconds";
	s.n_args = 0;
	s.n_returns = 1;
	result = of_server(&s);
	*ms = s.ms;
	return result;
}
