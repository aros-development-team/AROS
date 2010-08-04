struct IntrNode
{
    struct MinNode      in_Node;
    void                *in_Handler;
    void                *in_HandlerData;
    void                *in_HandlerData2;
    uint8_t             in_type;
    uint8_t             in_nr;
};

enum intr_types
{
    it_exception = 0xe0,
    it_interrupt = 0xf0
};

#ifdef HAVE_KERNEL_INTERRUPTS_C

/* Architecture-specific interrupt controller functions */

void ictl_enable_irq(uint8_t num)  /* Enable IRQ num  */
void ictl_disable_irq(uint8_t num) /* Disable IRQ num */

#else

#define ictl_enable_irq(irq)
#define ictl_disable_irq(irq)

#endif
