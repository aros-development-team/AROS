/* These are the bit definitions of the SysFlags and AttnResched flags.
    They are listed here more as somewhere to list them.
*/

#define SFB_SoftInt         5   /* There is a software interrupt */
#define SFF_SoftInt         (1L<<5)
#define SFB_QuantumOver	   13
#define SFF_QuantumOver     (1L<<13)

#define ARB_AttnSwitch      7   /* Delayed Switch() pending */
#define ARF_AttnSwitch      (1L<<7)
#define ARB_AttnDispatch   15   /* Delayed Dispatch() pending */
#define ARF_AttnDispatch    (1L<<15)
