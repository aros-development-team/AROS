#define _IMPLEMENTATION_

asm ("begin:");

#include "vesa.h"
#define ABS(x) (((x) >= 0) ? (x) : -(x))

asm (".long getControllerInfo");
asm (".long getModeInfo");
asm (".long findMode");
asm (".long setVbeMode");
asm (".long paletteWidth");
asm (".long controllerinfo");
asm (".long modeinfo");

short getControllerInfo(void)
{
    short retval;
    asm volatile("call go16 \n\t.code16 \n\t"
                "movw $0x4f00, %%ax\n\t"
                "int $0x10\n\t"
                "movw %%ax, %0\n\t"
                "DATA32 call go32\n\t.code32\n\t":"=b"(retval):"D"(&controllerinfo):"eax","ecx","cc");
    return retval;
}

/*
 * In VBE 1.1 information about standard modes was optional,
 * so we use a hardcoded table here (we rely on this information)
 */
struct vesa11Info vesa11Modes[] =
{
    {640,  400,  8, VMEM_PACKED},
    {640,  480,  8, VMEM_PACKED},
    {800,  600,  4, VMEM_PLANAR},
    {800,  600,  8, VMEM_PACKED},
    {1024, 768,  4, VMEM_PLANAR},
    {1024, 768,  8, VMEM_PACKED},
    {1280, 1024, 4, VMEM_PLANAR},
    {1280, 1024, 8, VMEM_PACKED}
};

short getModeInfo(long mode)
{
    short retval;
    long i;
    char *ptr = (char *)&modeinfo;

    for (i = 0; i < sizeof(modeinfo); i++)
	*ptr++ = 0;

    asm volatile("call go16 \n\t.code16 \n\t"
                "movw $0x4f01, %%ax\n\t"
                "int $0x10\n\t"
                "movw %%ax, %0\n\t"
                "DATA32 call go32\n\t.code32\n\t":"=b"(retval):"c"(mode),"D"(&modeinfo):"eax","cc");

    /*
     * VBE 1.1 does not provide resolution data for display modes.
     * In this case we patch the structure using hardcoded table.
     */
    if ((controllerinfo.version < 0x0102) && (mode > 0x0FF) && (mode < 0x108))
    {
	i = mode - VBE_MODE_STANDARD;

	modeinfo.x_resolution   = vesa11Modes[i].x_resolution;
	modeinfo.y_resolution   = vesa11Modes[i].y_resolution;
	modeinfo.bits_per_pixel = vesa11Modes[i].bits_per_pixel;
	modeinfo.memory_model   = vesa11Modes[i].memory_model;
    }
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

short paletteWidth(long req, unsigned char* width)
{
    short retval;
    unsigned char reswidth;
    
    asm volatile("call go16\n\t.code16\n\t"
		"movw $0x4f08, %%ax\n\t"
		"int $0x10\n\t"
		"movb %%bh, %1\n\t"
		"movw %%ax, %0\n\t"
		"DATA32 call go32\n\t.code32\n\t":"=b"(retval),"=c"(reswidth):"0"(req):"eax","cc");
    *width = reswidth;
    return retval;
}

short findMode(int x, int y, int d)
{
    unsigned long match, bestmatch = ABS(640*480 - x*y);
    unsigned long matchd, bestmatchd = 15 >= d ? 15 - d : (d - 15) * 2;
    unsigned short bestmode = 0x110;
    unsigned short mode_attrs;

    if (getControllerInfo() == VBE_RC_SUPPORTED)
    {
        unsigned short *modes = GET_FAR_PTR(controllerinfo.video_mode);
        int i;

	/*
	 * For VBE 2.0+ we can have linear framebuffer support, so we
	 * will look only for modes that support it.
	 */
	if (controllerinfo.version < 0x0200)
	    mode_attrs = VM_GRAPHICS|VM_SUPPORTED;
	else
	    mode_attrs = VM_GRAPHICS|VM_SUPPORTED|VM_LINEAR_FB;

	for (i=0; modes[i] != 0xffff; ++i)
        {
            /* Skip all not supported modes */
            if (getModeInfo(modes[i])!= VBE_RC_SUPPORTED) continue;
            /* Skip all modes that do not have requested attributes */
            if ((modeinfo.mode_attributes & mode_attrs) != mode_attrs) continue;
            /* We support only LUT or RGB modes */
            if ((modeinfo.memory_model != VMEM_RGB) && (modeinfo.memory_model != VMEM_PACKED))
		continue;
	    /* LUT modes must have VGA registers available, for palette changing */
	    if ((modeinfo.memory_model == VMEM_PACKED) && (modeinfo.mode_attributes & VM_NO_VGA_HW))
		continue;

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
    return bestmode;
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
        "               movl %ecx, (%esp)\n"
        "               xorl %eax, %eax\n"
        "               ret\n"
        "\n"
        "               .code32\n\t.globl go16\n\t.type go16,@function\n"
        "go16:          lgdt GDT_reg\n"
        "               movl %esp, stack32\n"
        "               movl (%esp), %eax\n"
        "               movl %eax, begin + 0xff8\n"
        "               movl $begin + 0xff8, %esp\n"
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

unsigned long           stack32;
struct vbe_controller   controllerinfo;
struct vbe_mode         modeinfo;
