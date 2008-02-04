#include <inttypes.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include <strings.h>

#include "kernel_intern.h"

#define STACK_SIZE 4096

void __putc(char c)
{
    while(!(inb(UART0_LSR) & UART_LSR_TEMT));
    
    outb(c, UART0_THR);
}

void __puts(char *str)
{
    while (*str)
    {
        if (*str == '\n')
            __putc('\r');
        __putc(*str++);
    }
}

static void __attribute__((used)) kernel_cstart(struct TagItem *msg)
{
    rkprintf("[KRN] Kernel resource pre-exec init\n");
}

asm(".section .aros.init,\"ax\"\n\t"
    ".globl start\n\t"
    ".type start,@function\n"
    "start:\n\t"
    "lis %r9,tmp_stack_end@ha\n\t"
    "mr %r29,%r3\n\t"
    "lwz %r1,tmp_stack_end@l(%r9)\n\t"
    "bl __clear_bss\n\t"
    "lis %r11,target_address@ha\n\t"
    "mr %r3,%r29\n\t"
    "lwz %r11,target_address@l(%r11)\n\t"
    "lis %r9,stack_end@ha\n\t"
    "mtctr %r11\n\t"
    "lwz %r1,stack_end@l(%r9)\n\t"
    "bctrl\n\t"
    "\n1: b 1b\n\t"
    ".string \"Native/CORE v3 (" __DATE__ ")\""
    "\n\t.text\n\t"
);

static void __attribute__((used)) __clear_bss(struct TagItem *msg) 
{
    struct KernelBSS *bss;
    
    bss =(struct KernelBSS *)krnGetTagData(KRN_KernelBss, 0, msg);
    rkprintf("[KRN] Clearing BSS\n");

    if (bss)
    {
        while (bss->addr && bss->len)
        {
            rkprintf("[KRN]   %p-%p\n", bss->addr, (char*)bss->addr+bss->len-1);
            bzero(bss->addr, bss->len);
            bss++;
        }   
    }
}

static uint32_t __attribute__((used)) tmp_stack[128]={1,};
static const uint32_t *tmp_stack_end __attribute__((used, section(".text"))) = &tmp_stack[120];
static uint32_t stack[STACK_SIZE] __attribute__((used));
static uint32_t stack_super[STACK_SIZE] __attribute__((used));
static const uint32_t *stack_end __attribute__((used, section(".text"))) = &stack[STACK_SIZE-16];
static const void *target_address __attribute__((used, section(".text"))) = (void*)kernel_cstart;

struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr)
{
    if (!(*tagListPtr)) return 0;

    while(1)
    {
        switch((*tagListPtr)->ti_Tag)
        {
            case TAG_MORE:
                if (!((*tagListPtr) = (struct TagItem *)(*tagListPtr)->ti_Data))
                    return NULL;
                continue;
            case TAG_IGNORE:
                break;

            case TAG_END:
                (*tagListPtr) = 0;
                return NULL;

            case TAG_SKIP:
                (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
                continue;

            default:
                return (struct TagItem *)(*tagListPtr)++;

        }

        (*tagListPtr)++;
    }
}

struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList)
{
    struct TagItem *tag;
    const struct TagItem *tagptr = tagList;

    while((tag = krnNextTagItem(&tagptr)))
    {
        if (tag->ti_Tag == tagValue)
            return tag;
    }

    return 0;
}

IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList)
{
    struct TagItem *ti = 0;

    if (tagList && (ti = krnFindTagItem(tagValue, tagList)))
        return ti->ti_Data;

        return defaultVal;
}
