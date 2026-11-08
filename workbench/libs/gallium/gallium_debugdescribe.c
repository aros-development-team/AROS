/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reference descriptor for Mesa's pipe_resource_reference().
*/

/* p_state.h first: u_debug_describe.h declares a descriptor for
   pipe_stream_output_target without forward declaring it. */
#include "pipe/p_state.h"
#include "util/u_debug_describe.h"

/*
 * pipe_resource_reference() and pipe_resource_destroy() (util/u_inlines.h)
 * hand the address of this function to pipe_reference_described(), which
 * passes it on to debug_reference(). Mesa builds here with NDEBUG, so
 * MESA_DEBUG is 0, debug_reference() is an empty inline and the descriptor
 * is never called: an optimised build drops the reference to it entirely.
 * An unoptimised one still materialises the address, and the link then
 * needs the symbol - Mesa's own definition lives in u_debug_describe.c,
 * which this library does not build, as that file wants the format tables
 * and with them most of mesa.library.
 *
 * The siblings this header declares are only needed if something here ever
 * calls the inlines that name them.
 */
void debug_describe_resource(char *buf, const struct pipe_resource *ptr)
{
    (void)ptr;

    if (buf)
        buf[0] = '\0';
}
