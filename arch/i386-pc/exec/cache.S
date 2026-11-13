/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

                    .balign 32,0x90
                    .globl  Exec_Wbinvd
                    .type   Exec_Wbinvd,@function
                    
Exec_Wbinvd:        wbinvd
                    iret
