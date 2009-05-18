/*
 * fec_handler.c
 *
 *  Created on: May 18, 2009
 *      Author: misc
 */

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/memory.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/openfirmware.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <inttypes.h>

#include "fec.h"

void handle_request(struct FECBase *FECBase, struct IOSana2Req *request)
{
	D(bug("[FEC] handle_request\n"));
}
