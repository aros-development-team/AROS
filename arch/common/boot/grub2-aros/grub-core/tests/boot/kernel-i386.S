#define ASM_FILE	1
#ifdef TARGET_MULTIBOOT2
#include <multiboot2.h>
#elif defined (TARGET_MULTIBOOT)
#include <multiboot.h>
#endif

#define SHUTDOWN_PORT 0x8900

	.text
	/* Align 32 bits boundary.  */
	.align	8

#ifdef TARGET_MULTIBOOT2
	/* Multiboot header.  */
multiboot_header:
	/* magic */
	.long	MULTIBOOT2_HEADER_MAGIC
	/* ISA: i386 */
	.long   MULTIBOOT_ARCHITECTURE_I386
	/* Header length.  */
	.long	multiboot_header_end - multiboot_header
	/* checksum */
	.long	-(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header))
	.short MULTIBOOT_HEADER_TAG_END
	.short 0
	.long 8
multiboot_header_end:
#elif defined (TARGET_MULTIBOOT)
	/* Multiboot header.  */
multiboot_header:
	/* magic */
	.long	MULTIBOOT_HEADER_MAGIC
	/* flags */
	.long	0
	/* checksum */
	.long	-MULTIBOOT_HEADER_MAGIC
#endif
	
	.global start
portmsg:
	xorl %eax, %eax
1:	
	movb 0(%esi), %al
	test %eax, %eax
	jz 1f
	outb %al, %dx
	incl %esi
	jmp 1b
1:
	ret

serialmsg:	
1:	
	movb 0(%esi), %bl
	testb %bl, %bl
	jz 1f
	movw $0x3fd, %dx
2:	
	inb %dx, %al
	testb $0x20, %al
	jz   2b

	movw $0x3f8, %dx
	movb %bl, %al
	outb %al, %dx
	incl %esi
	jmp 1b
1:
	ret

	.globl _start
_start:
	lea message, %esi
	call serialmsg
	lea shutdown, %esi
	movw $SHUTDOWN_PORT, %dx
	call portmsg

1:
	hlt
	jmp 1b

shutdown:
	.ascii "Shutdown"
	.byte 0
message:
	.ascii "Boot Test Passed Successfully\n" SUCCESSFUL_BOOT_STRING "\n"
	.byte 0
