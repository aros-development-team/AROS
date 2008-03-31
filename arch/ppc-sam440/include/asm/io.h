#ifndef ASM_IO_H
#define ASM_IO_H

#include <inttypes.h>
#include <asm/amcc440.h>

static inline eieio() {
    asm volatile("eieio":::"memory");
}

static inline sync() {
    asm volatile("sync":::"memory");
}

static inline isync() {
    asm volatile("isync":::"memory");
}

static inline uint8_t inb(uint8_t *port) {
    uint8_t ret; asm volatile("lbz%U1%X1 %0,%1; eieio":"=r"(ret):"m"(*port)); return ret;
}

static inline void outb(uint8_t val, char *port) {
    asm volatile("stb%U0%X0 %1,%0; eieio"::"m"(*port),"r"(val));
}

static inline uint16_t inw(uint16_t *port) {
    uint16_t ret; asm volatile("lhz%U1%X1 %0,%1; eieio":"=r"(ret):"m"(*port)); return ret;
}

static inline void outw(uint16_t val, uint16_t *port) {
    asm volatile("sth%U0%X0 %1,%0; eieio"::"m"(*port),"r"(val));
}

static inline uint16_t inw_be(uint16_t *port) {
    return inw(port);
}

static inline void outw_be(uint16_t val, uint16_t *port) {
    outw(val, port);
}

static inline uint16_t inw_le(uint16_t *port) {
    uint16_t ret; asm volatile("lhbrx %0,0,%1; eieio":"=r"(ret):"r"(port),"m"(*port)); return ret;
}

static inline void outw_le(uint16_t val, uint16_t *port) {
    asm volatile("sthbrx %1,0,%2; eieio":"=m"(*port):"r"(val),"r"(port));
}


static inline uint32_t inl(uint32_t *port) {
    uint32_t ret; asm volatile("lwz%U1%X1 %0,%1; eieio":"=r"(ret):"m"(*port)); return ret;
}

static inline void outl(uint32_t val, uint32_t *port) {
    asm volatile("stw%U0%X0 %1,%0; eieio"::"m"(*port),"r"(val));
}

static inline uint32_t inl_be(uint32_t *port) {
    return inl(port);
}

static inline void outl_be(uint32_t val, uint32_t *port) {
    outl(val, port);
}

static inline uint32_t inl_le(uint32_t *port) {
    uint32_t ret; asm volatile("lwbrx %0,0,%1; eieio":"=r"(ret):"r"(port),"m"(*port)); return ret;
}

static inline void outl_le(uint32_t val, uint32_t *port) {
    asm volatile("stwbrx %1,0,%2; eieio":"=m"(*port):"r"(val),"r"(port));
}

static inline void pci_outb(uint8_t val, uint16_t port)
{
    outb(val, (uint8_t *)(port + PCIC0_IO));
}

static inline void pci_outw(uint16_t val, uint16_t port)
{
    outw(val, (uint16_t *)(port + PCIC0_IO));
}

static inline void pci_outw_be(uint16_t val, uint16_t port)
{
    outw_be(val, (uint16_t *)(port + PCIC0_IO));
}

static inline void pci_outw_le(uint16_t val, uint16_t port)
{
    outw_le(val, (uint16_t *)(port + PCIC0_IO));
}

static inline void pci_outl(uint32_t val, uint16_t port)
{
    outl(val, (uint32_t *)(port + PCIC0_IO));
}

static inline void pci_outl_be(uint32_t val, uint16_t port)
{
    outl_be(val, (uint32_t *)(port + PCIC0_IO));
}

static inline void pci_outl_le(uint32_t val, uint16_t port)
{
    outl_le(val, (uint32_t *)(port + PCIC0_IO));
}

static inline uint8_t pci_inb(uint16_t port)
{
    return inb((uint8_t *)(port + PCIC0_IO));
}

static inline uint16_t pci_inw(uint16_t port)
{
    return inw((uint16_t *)(port + PCIC0_IO));
}

static inline uint16_t pci_inw_be(uint16_t port)
{
    return inw_be((uint16_t *)(port + PCIC0_IO));
}

static inline uint16_t pci_inw_le(uint16_t port)
{
    return inw_le((uint16_t *)(port + PCIC0_IO));
}

static inline uint32_t pci_inl(uint16_t port)
{
    return inl((uint32_t *)(port + PCIC0_IO));
}

static inline uint32_t pci_inl_be(uint16_t port)
{
    return inl_be((uint32_t *)(port + PCIC0_IO));
}

static inline uint32_t pci_inl_le(uint16_t port)
{
    return inl_le((uint32_t *)(port + PCIC0_IO));
}


#endif /*ASM_IO_H*/
