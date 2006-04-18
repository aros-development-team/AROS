	#define NO_PROTOS
	#include "pp_exec_internal.h"
	

	.text
	.balign 4
	.globl dm_bus_error_handler

dm_bus_error_handler:
	/*
	 * As easy as can be: Leave a note that something
	 * bad happened in address 'ADDRESS_ERROR_MARKER_ADDRESS'
	 */
	move.l	#2,ADDRESS_ERROR_MARKER_ADDRESS
	rte

	.text
	.balign 4
	.globl dm_addr_error_handler

dm_addr_error_handler:
	/*
	 * As easy as can be: Leave a note that something
	 * bad happened in address 'ADDRESS_ERROR_MARKER_ADDRESS'
	 */
	move.l	#3,ADDRESS_ERROR_MARKER_ADDRESS
	rte
