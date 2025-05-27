/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
	
	This code is to help debug crashes early in the kernels startup code
*/

#include <aros/multiboot.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "boot_utils.h"
#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_bootmem.h"
#include "kernel_debug.h"
#include "kernel_mmap.h"
#include "kernel_romtags.h"

extern void boot_crashhandler(uint64_t int_no, uint64_t err_code);

static x86vectgate_t boot_gates[16];

#define VGA_ADDR ((volatile uint16_t*)0xB8000)
#define VGA_COLOR_WHITE_ON_BLUE  0x1F  // 0x1 = white (foreground), 0xF = blue (background)
#define VGA_COLOR_RED_ON_BLACK  0x04  // 0x4 = red (foreground), 0x0 = black (background)
#define VGA_COLOR_RED_BACK  0x4F
#define VGA_COLOR VGA_COLOR_RED_ON_BLACK
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_LIMIT  (VGA_WIDTH * VGA_HEIGHT)

#define STR2(x) #x
#define STR(x) STR2(x)

// Macro for exceptions that DO NOT push error code
#define ISR_NOERR(n) \
__attribute__((naked)) static void isr_stub_##n(void) { \
    __asm__ volatile ( \
        "pushq $0;"           /* fake error code */ \
        "pushq $" #n ";"      /* interrupt number */ \
        "jmp boot_crashhandler;" \
    ); \
}

// Macro for exceptions that DO push error code
#define ISR_HASERR(n) \
__attribute__((naked)) static void isr_stub_##n(void) { \
    __asm__ volatile ( \
        "pushq $" #n ";"      /* interrupt number */ \
        "jmp boot_crashhandler;" \
    ); \
}

// First 16 ISRs
ISR_NOERR(0)   // Divide by zero
ISR_NOERR(1)   // Debug
ISR_NOERR(2)   // NMI
ISR_NOERR(3)   // Breakpoint
ISR_NOERR(4)   // Overflow
ISR_NOERR(5)   // Bound range exceeded
ISR_NOERR(6)   // Invalid opcode
ISR_NOERR(7)   // Device not available
ISR_HASERR(8)  // Double fault
ISR_NOERR(9)   // Coprocessor segment overrun (obsolete)
ISR_HASERR(10) // Invalid TSS
ISR_HASERR(11) // Segment not present
ISR_HASERR(12) // Stack-segment fault
ISR_HASERR(13) // General protection
ISR_HASERR(14) // Page fault
ISR_NOERR(15)  // Reserved

static const char hex_digits[] = "0123456789ABCDEF";
static char buf[21];

static volatile uint16_t *vga_cursor;

static void draw_frame(uint16_t border_char) {
    volatile uint16_t* vga = VGA_ADDR;

    // Top and bottom rows
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga[x] = border_char;  // top row
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = border_char;  // bottom row
    }

    // Left and right columns
    for (int y = 1; y < VGA_HEIGHT - 1; y++) {
        vga[y * VGA_WIDTH] = border_char;               // left column
        vga[y * VGA_WIDTH + VGA_WIDTH - 1] = border_char;  // right column
    }
}

static int print_char(char c, volatile uint16_t** cursor) {
    volatile uint16_t* vga_base = VGA_ADDR;
    uintptr_t pos = *cursor - vga_base;

    if (pos >= VGA_LIMIT) {
        return -1; // fail if cursor is out of bounds
    }

    if (c == '\n') {
        // Move to the beginning of the next line
        pos = ((pos / VGA_WIDTH) + 1) * VGA_WIDTH;
        if (pos >= VGA_LIMIT) {
            return -1; // fail if newline would overflow screen
        }
        *cursor = vga_base + pos + 1;
    } else {
        // Calculate current column (0 to 79)
        size_t col = pos % VGA_WIDTH;

        // Print character
        **cursor = ((uint16_t)VGA_COLOR << 8) | c;
        (*cursor)++;

        pos++;

        // If we just printed at column 79, wrap to the next line
        if (col == VGA_WIDTH - 2) {
            pos = ((pos / VGA_WIDTH) + 1) * VGA_WIDTH;
            if (pos >= VGA_LIMIT) {
                return -1; // fail if wrapping overflows screen
            }
            *cursor = vga_base + pos + 1;
        }
    }

    return 0; // success
}

static int print_str(const char* str, volatile uint16_t** cursor) {
    while (*str) {
        if (print_char(*str++, cursor) != 0) {
            return -1;  // stop printing on failure
        }
    }
    return 0;
}

static void print_uint64(uint64_t val, volatile uint16_t **cursor) {

    char *p = buf + sizeof(buf) - 1;
    *p = '\0';

    if (val == 0) {
        print_char('0', cursor);
        return;
    }

    while (val > 0) {
        *(--p) = '0' + (val % 10);
        val /= 10;
    }

    while (*p)
        print_char(*p++, cursor);
}

static void uint8_as_hex(uint8_t value, char* buf) {
    buf[0] = hex_digits[(value >> 4) & 0xF];
    buf[1] = hex_digits[value & 0xF];
    buf[2] = '\0';
}

static void uint8_to_hex(uint8_t val, char* buf) {
    buf[0] = '0';
    buf[1] = 'x';
	uint8_as_hex(val, &buf[2]);
}

static void uint32_as_hex(uint32_t val, char* buf) {
    for (int i = 0; i < 8; ++i) {
        buf[i] = hex_digits[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[8] = '\0';
}

static void uint32_to_hex(uint32_t val, char* buf) {
    buf[0] = '0';
    buf[1] = 'x';
    uint32_as_hex(val, &buf[2]);
}

static void uint64_as_hex(uint64_t val, char* buf) {
    for (int i = 0; i < 16; ++i) {
        buf[i] = hex_digits[(val >> ((15 - i) * 4)) & 0xF];
    }
    buf[16] = '\0';
}

static void uint64_to_hex(uint64_t val, char* buf) {
    buf[0] = '0';
    buf[1] = 'x';
    uint64_as_hex(val, &buf[2]);
}

static void print_crash_type(uint64_t int_no, volatile uint16_t **cursor) {
	switch (int_no)
	{
	case 0:  print_str("Divide by zero", cursor); break;
	case 1:  print_str("Debug", cursor); break;
	case 2:  print_str("NMI", cursor); break;
	case 3:  print_str("Breakpoint", cursor); break;
	case 4:  print_str("Overflow", cursor); break;
	case 5:  print_str("Bound range exceeded", cursor); break;
	case 6:  print_str("Invalid opcode", cursor); break;
	case 7:  print_str("Device not available", cursor); break;
	case 8:  print_str("Double fault", cursor); break;
	case 9:  print_str("Copro seg overrun", cursor); break;
	case 10: print_str("Invalid TSS", cursor); break;
	case 11: print_str("Seg not present", cursor); break;
	case 12: print_str("Stack-seg fault", cursor); break;
	case 13: print_str("General protection", cursor); break;
	case 14: print_str("Page fault", cursor); break;
	default: print_str("Reserved", cursor); break;
	}
}

static void print_crash_info(uint64_t *stack) {
    uint64_t int_no   = stack[15]; // offset 120 / 8
    uint64_t err_code = stack[16]; // offset 128 / 8
    uint64_t rip      = stack[17]; // offset 136 / 8
	uint16_t frame = (uint16_t)' ';

	do
	{
		vga_cursor = (volatile uint16_t*)VGA_ADDR;
		if ((frame & 0xFF00) == 0)
			frame |= ((uint16_t)VGA_COLOR_RED_BACK << 8);
		else
			frame &=  0xFF;
		draw_frame(frame);
		print_char('\n', &vga_cursor);
		print_str("Int: ", &vga_cursor);
		print_uint64(int_no, &vga_cursor);
		print_str(" (", &vga_cursor);
		print_crash_type(int_no, &vga_cursor);
		print_char(')', &vga_cursor);
		print_char('\n', &vga_cursor);

		print_str("E: ", &vga_cursor);
		print_uint64(err_code, &vga_cursor);
		print_str(", RIP: ", &vga_cursor);
		uint64_to_hex(rip, buf);
		print_str(buf, &vga_cursor);
		print_char('\n', &vga_cursor);
		
		print_str("Flags: ", &vga_cursor);
		uint32_to_hex((stack[19] & 0xFFFFFFFF), buf);
		print_str(buf, &vga_cursor);
		print_char('\n', &vga_cursor);
		print_char('\n', &vga_cursor);

		print_str("RAX: ", &vga_cursor);
		uint64_to_hex(stack[15], buf);
		print_str(buf, &vga_cursor);
		print_str(" RBX: ", &vga_cursor);
		uint64_to_hex(stack[14], buf);
		print_str(buf, &vga_cursor);
		print_str(" RCX: ", &vga_cursor);
		uint64_to_hex(stack[13], buf);
		print_str(buf, &vga_cursor);
		print_str(" RDX: ", &vga_cursor);
		uint64_to_hex(stack[12], buf);
		print_str(buf, &vga_cursor);
		print_char('\n', &vga_cursor);

		print_str("RSI: ", &vga_cursor);
		uint64_to_hex(stack[11], buf);
		print_str(buf, &vga_cursor);
		print_str(" RDI: ", &vga_cursor);
		uint64_to_hex(stack[10], buf);
		print_str(buf, &vga_cursor);
		print_str(" RBP: ", &vga_cursor);
		uint64_to_hex(stack[9], buf);
		print_str(buf, &vga_cursor);
		print_str(" RSP: ", &vga_cursor);
		uint64_to_hex(stack[8], buf);
		print_str(buf, &vga_cursor);
		print_char('\n', &vga_cursor);

		print_str("R08: ", &vga_cursor);
		uint64_to_hex(stack[7], buf);
		print_str(buf, &vga_cursor);
		print_str(" R09: ", &vga_cursor);
		uint64_to_hex(stack[6], buf);
		print_str(buf, &vga_cursor);
		print_str(" R10: ", &vga_cursor);
		uint64_to_hex(stack[5], buf);
		print_str(buf, &vga_cursor);
		print_str(" R11: ", &vga_cursor);
		uint64_to_hex(stack[4], buf);
		print_str(buf, &vga_cursor);
		print_char('\n', &vga_cursor);

		print_str("R12: ", &vga_cursor);
		uint64_to_hex(stack[3], buf);
		print_str(buf, &vga_cursor);
		print_str(" R13: ", &vga_cursor);
		uint64_to_hex(stack[2], buf);
		print_str(buf, &vga_cursor);
		print_str(" R14: ", &vga_cursor);
		uint64_to_hex(stack[1], buf);
		print_str(buf, &vga_cursor);
		print_str(" R15: ", &vga_cursor);
		uint64_to_hex(stack[0], buf);
		print_str(buf, &vga_cursor);
		print_char('\n', &vga_cursor);

		// If invalid opcode (interrupt 6), show 32 bytes at RIP
		if (int_no == 6) {
			print_char('\n', &vga_cursor);
			print_str("Opcode @ ", &vga_cursor);
			uint64_to_hex(rip, buf);
			print_str(buf, &vga_cursor);
			print_char('\n', &vga_cursor);
			uint8_t* ptr = (uint8_t*)rip;
			for (int i = 0; i < 32; ++i) {
				uint8_as_hex(ptr[i], buf);
				print_str(buf, &vga_cursor);
				print_char(' ', &vga_cursor);
			}
			print_char('\n', &vga_cursor);
		}

		volatile uint64_t i;
		for (i = 0; i < 50000000; ++i) {
			__asm__ volatile ("pause");
		}
	}
	while (true);

    __asm__ __volatile__ (
        "hlt\n\t"
		);
}

__attribute__((naked)) void boot_crashhandler(uint64_t int_no, uint64_t err_code) {
    __asm__ __volatile__ (
        "cli\n\t"

        // Save general-purpose registers
        "push %%rax\n\t"
        "push %%rbx\n\t"
        "push %%rcx\n\t"
        "push %%rdx\n\t"
        "push %%rsi\n\t"
        "push %%rdi\n\t"
        "push %%rbp\n\t"
        "push %%r8\n\t"
        "push %%r9\n\t"
        "push %%r10\n\t"
        "push %%r11\n\t"
        "push %%r12\n\t"
        "push %%r13\n\t"
        "push %%r14\n\t"
        "push %%r15\n\t"

        // Pass current stack pointer to C
        "mov %%rsp, %%rdi\n\t"
        "call print_crash_info\n\t"

        // Restore registers
        "pop %%r15\n\t"
        "pop %%r14\n\t"
        "pop %%r13\n\t"
        "pop %%r12\n\t"
        "pop %%r11\n\t"
        "pop %%r10\n\t"
        "pop %%r9\n\t"
        "pop %%r8\n\t"
        "pop %%rbp\n\t"
        "pop %%rdi\n\t"
        "pop %%rsi\n\t"
        "pop %%rdx\n\t"
        "pop %%rcx\n\t"
        "pop %%rbx\n\t"
        "pop %%rax\n\t"

        "sti\n\t"
        "iretq\n\t"
        :
        :
        : "memory"
    );
}

#define SET_GATE(i, stub) \
    boot_gates[i].offset_low = (IPTR)(stub) & 0xFFFF; \
    boot_gates[i].selector = KERNEL_CS; \
    boot_gates[i].ist = 0; \
    boot_gates[i].type = 0x0E; \
    boot_gates[i].dpl = 3; \
    boot_gates[i].p = 1; \
    /* 32-bit vs 64-bit split */ \
    boot_gates[i].offset_mid = ((IPTR)(stub) >> 16) & 0xFFFF; \
    boot_gates[i].offset_high = ((IPTR)(stub) >> 32) & 0xFFFFFFFF;

void core_InitEarlyIDT()
{
    /* set up emergency boot IDT */
    SET_GATE(0,  isr_stub_0);// Divide by zero
    SET_GATE(1,  isr_stub_1);// Debug
    SET_GATE(2,  isr_stub_2);// NMI
    SET_GATE(3,  isr_stub_3); // Breakpoint
    SET_GATE(4,  isr_stub_4);// Overflow
    SET_GATE(5,  isr_stub_5);// Bound range exceeded
    SET_GATE(6,  isr_stub_6);// Invalid opcode
    SET_GATE(7,  isr_stub_7);// Device not available
    SET_GATE(8,  isr_stub_8);// Double fault
    SET_GATE(9,  isr_stub_9);// Coprocessor segment overrun (obsolete)
    SET_GATE(10, isr_stub_10);// Invalid TSS
    SET_GATE(11, isr_stub_11);// Segment not present
    SET_GATE(12, isr_stub_12);// Stack-segment fault
    SET_GATE(13, isr_stub_13);// General protection
    SET_GATE(14, isr_stub_14);// Page fault
    SET_GATE(15, isr_stub_15);// Reserved
    struct segment_selector IDT_sel;
    IDT_sel.size = sizeof(x86vectgate_t) * 16 - 1;
    IDT_sel.base = (unsigned long)boot_gates;
    asm volatile ("lidt %0"::"m"(IDT_sel));
}
