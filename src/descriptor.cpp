#include "main.h"

const unsigned long long gdt[] = {
    0x0000000000000000,
    0x00af9a000000ffff,
    0x00cf93000000ffff};

INTERRUPT_DESCRIPTOR descriptors[256];

void init_gdt()
{
    unsigned long long gdtr[2];
    gdtr[0] = (reinterpret_cast<unsigned long long>(gdt) << 16) | (sizeof(gdt) - 1);
    gdtr[1] = (reinterpret_cast<unsigned long long>(gdt) >> 48);
    load_gdt(gdtr);
    unsigned short cs, ds, ss;
    cs = 1 << 3;
    ds = 2 << 3;
    ss = 2 << 3;
    unsigned short selector = cs;
    unsigned long long dummy;
    asm volatile("pushq %[selector];"
                 "leaq ret_label(%%rip), %[dummy];"
                 "pushq %[dummy];"
                 "lretq;"
                 "ret_label:"
                 : [ dummy ] "=r"(dummy)
                 : [ selector ] "m"(selector));
    write_ss_selector(ss);
    write_ds_selector(ds);
}

void init_idt()
{
    for (unsigned int i = 0; i < 256; ++i)
    {
        set_interrupt_handler(i, default_handler);
    }
    unsigned long long idtr[2];
    idtr[0] = (reinterpret_cast<unsigned long long>(descriptors) << 16) | (sizeof(descriptors) - 1);
    idtr[1] = (reinterpret_cast<unsigned long long>(descriptors) >> 48);
    load_idt(idtr);
}

void set_interrupt_handler(unsigned int idt_no, __attribute__((ms_abi)) void (*handler)())
{
    descriptors[idt_no].offset_low = reinterpret_cast<unsigned long long>(handler);
    descriptors[idt_no].selector = 0x0008;
    descriptors[idt_no].type_attr = 14;
    descriptors[idt_no].type_attr |= 0b10000000;
    descriptors[idt_no].offset_middle = reinterpret_cast<unsigned long long>(handler) >> 16;
    descriptors[idt_no].offset_high = reinterpret_cast<unsigned long long>(handler) >> 32;
}