
.section .intvecs, "ax"

/* initial, unpatchable vector table */

_reset_vec:
           ldr	pc, reset_handler_address
           ldr	pc, undef_handler_address
           ldr	pc, svc_handler_address
           ldr	pc, prefetch_abort_handler_address
           ldr	pc, data_abort_handler_address
_loop:   b       .
           ldr	pc, irq_handler_address
           ldr	pc, fiq_handler_address

reset_handler_address:		       .word	__vectorhand_reset
undef_handler_address:	       .word	__vectorhand_undef
svc_handler_address:		       .word	__vectorhand_swi
prefetch_abort_handler_address:   .word	__vectorhand_prefetchabort
data_abort_handler_address:        .word	__vectorhand_dataabort
irq_handler_address:		       .word	__vectorhand_irq
fiq_handler_address:		       .word	__vectorhand_fiq
