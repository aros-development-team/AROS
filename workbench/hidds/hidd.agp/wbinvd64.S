                    .balign 32,0x90
                    .globl  Wbinvd
                    .type   Wbinvd,@function
                    
Wbinvd:             wbinvd
                    iretq
