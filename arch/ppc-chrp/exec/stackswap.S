#include <aros/ppc/asm.h>
#include <aros/config.h>

                .text
                .align 4
                .globl AROS_SLIB_ENTRY(StackSwap, Exec)
                .type AROS_SLIB_ENTRY(StackSwap, Exec), @function 

#define SysBase %r30
#define SSS     %r29

AROS_SLIB_ENTRY(StackSwap, Exec):
                stwu %r1, -32(%r1)
                
                stw %r3,8(%r1)
                stw %r4,12(%r1)
                stw %r30,16(%r1)
                stw %r29,20(%r1)
                stw %r28,24(%r1)
                
                mr  SSS,%r3
                mr  SysBase,%r4
                
                mflr %r28
                
                lwz %r11,Disable(SysBase)
                mtctr %r11
                mr %r3, SysBase
                bctrl
                
                lwz %r3,ThisTask(SysBase)
                
                lwz %r4,tc_SPLower(%r3)
                lwz %r5,stk_Lower(SSS)
                stw %r4,stk_Lower(SSS)
                stw %r5,tc_SPLower(%r3)
                
                lwz %r4,tc_SPUpper(%r3)
                lwz %r5,stk_Upper(SSS)
                stw %r4,stk_Upper(SSS)
                stw %r5,tc_SPUpper(%r3)

                lwz %r4,stk_Pointer(SSS)
                addi %r11,%r1,32
                stw %r11,stk_Pointer(SSS)
                mr %r1,%r4

                lwz %r11,Enable(SysBase)
                mtctr %r11
                mr %r3,SysBase
                bctrl

                lwz %r3,stk_Pointer(SSS)

                lwz %r30,16-32(%r3)
                lwz %r29,20-32(%r3)

                mtlr %r28
                lwz %r28,24-32(%r3)
                
                blr
