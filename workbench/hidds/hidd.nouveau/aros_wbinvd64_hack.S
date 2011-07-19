                    .balign 32,0x90
                    .globl  HACK_Wbinvd
                    .type   HACK_Wbinvd,@function
                    
HACK_Wbinvd:        wbinvd
                    iretq
