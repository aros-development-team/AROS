	#define NO_PROTOS
	#include "arm_exec_internal.h"
	

	.text
	.align	2
	.globl	dm_data_abort_handler
	.type	dm_data_abort_handler, function

dm_data_abort_handler:
	/*
	 * As easy as can be: Leave a note that something
	 * bad happened in address 'DATA_ABORT_MARKER_ADDRESS'
	 */
	mov	r8,#DATA_ABORT_MARKER_ADDRESS
	mov	r9,#1
	str	r9,[r8]
	subs	pc,r14,#4

