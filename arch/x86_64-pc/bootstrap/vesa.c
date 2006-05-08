#define _IMPLEMENTATION_

#include "vesa.h"
#define ABS(x) (((x) >= 0) ? (x) : -(x))

asm (".long getControllerInfo");
asm (".long getModeInfo");
asm (".long findMode");
asm (".long setVbeMode");
asm (".long controllerinfo");
asm (".long modeinfo");

short getControllerInfo(struct vbe_controller *info)
{
    short retval;
    asm volatile("call go16 \n\t.code16 \n\t"
                "movw $0x4f00, %%ax\n\t"
                "int $0x10\n\t"
                "movw %%ax, %0\n\t"
                "DATA32 call go32\n\t.code32\n\t":"=b"(retval):"D"(info):"eax","ecx","cc");
    return retval;
}

short getModeInfo(long mode, struct vbe_mode *info)
{
    short retval;
    asm volatile("call go16 \n\t.code16 \n\t"
                "movw $0x4f01, %%ax\n\t"
                "int $0x10\n\t"
                "movw %%ax, %0\n\t"
                "DATA32 call go32\n\t.code32\n\t":"=b"(retval):"c"(mode),"D"(info):"eax","cc");
    return retval;
}

short setVbeMode(long mode)
{
    short retval;
    asm volatile("call go16 \n\t.code16 \n\t"
                "movw $0x4f02, %%ax\n\t"
                "int $0x10\n\t"
                "movw %%ax, %0\n\t"
                "DATA32 call go32\n\t.code32\n\t":"=b"(retval):"0"(mode & 0xf7ff):"eax","ecx","cc");
    return retval;
}

short findMode(int x, int y, int d)
{
    long match, bestmatch = ABS(640*480 - x*y);
    long matchd, bestmatchd = 15 >= d ? 15 - d : (d - 15) * 2;
    short bestmode = 0x110;
    
    if (getControllerInfo(&controllerinfo) == 0x4f)
    {
        unsigned short *modes = (unsigned short *)controllerinfo.video_mode;
        int i;
                
        for (i=0; modes[i] != 0xffff; ++i)
        {
            if (getModeInfo(modes[i], &modeinfo)!= 0x4f) continue;
            if ((modeinfo.mode_attributes & 0x90) != 0x90) continue;
            if (modeinfo.memory_model != 6) continue;

            if (modeinfo.x_resolution == x &&
                modeinfo.y_resolution == y &&
                modeinfo.bits_per_pixel == d)
                return modes[i];
            
            match = ABS(modeinfo.x_resolution*modeinfo.y_resolution - x*y);
            matchd = modeinfo.bits_per_pixel >= d ? modeinfo.bits_per_pixel-d: (d-modeinfo.bits_per_pixel)*2;
            if (match < bestmatch || (match == bestmatch && matchd < bestmatchd))
            {
                bestmode = modes[i];
                bestmatch = match;
                bestmatchd = matchd;
            }
        }
    }
    return bestmatchd;
}

asm(
"               .code16\n\t.globl go32\n\t.type go32,@function\n"
"go32:          DATA32 ADDR32 lgdt GDT_reg\n"
"               movl %cr0, %eax\n"
"               bts $0, %eax\n"
"               movl %eax, %cr0\n"
"               ljmp $0x08, $1f\n"
"               .code32\n"
"1:             movw $0x10, %ax\n"
"               movw %ax, %ds\n"
"               movw %ax, %es\n"
"               movw %ax, %fs\n"
"               movw %ax, %gs\n"
"               movw %ax, %ss\n"
"               movl (%esp), %ecx\n"
"               movl stack32, %eax\n"
"               movl %eax, %esp\n"
"               movl %eax, %ebp\n"
"               movl %ecx, (%esp)\n"
"               xorl %eax, %eax\n"
"               ret\n"
"\n"
"               .code32\n\t.globl go16\n\t.type go16,@function\n"
"go16:          lgdt GDT_reg\n"
"               movl %esp, stack32\n"
"               movl (%esp), %eax\n"
"               movl %eax, stack16+2044\n"
"               movl $stack16+2044, %esp\n"
"               movl %esp, %ebp\n"
"               movw $0x20, %ax\n"
"               movw %ax, %ds\n"
"               movw %ax, %es\n"
"               movw %ax, %fs\n"
"               movw %ax, %gs\n"
"               movw %ax, %ss\n"
"               ljmp $0x18, $1f\n\t.code16\n"
"1:\n"
"               movl %cr0, %eax\n"
"               btc $0, %eax\n"
"               movl %eax, %cr0\n"
"               DATA32 ljmp $0x00, $1f\n"
"1:\n"
"               xorl %eax,%eax\n"
"               movw %ax, %ds\n"
"               movw %ax, %es\n"
"               movw %ax, %fs\n"
"               movw %ax, %gs\n"
"               movw %ax, %ss\n"
"               DATA32 ret\n"
".code32");

const unsigned long long GDT_Table[] = {
    0x0000000000000000ULL,
    0x00cf9a000000ffffULL,     /* Code32 */
    0x00cf92000000ffffULL,     /* Data32 */
    0x00009e000000ffffULL,     /* Code16 */
    0x000092000000ffffULL      /* Data16 */
};

const struct
{
    unsigned short l1 __attribute__((packed));
    const void *l3 __attribute__((packed));
}
GDT_reg = {sizeof(GDT_Table)-1, GDT_Table};

unsigned long stack32 __attribute__((section(".data")));
char stack16[2048] __attribute__((section(".data")));
struct vbe_controller   controllerinfo __attribute__((section(".data")));
struct vbe_mode         modeinfo __attribute__((section(".data")));
