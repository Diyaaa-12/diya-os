#include "isr.h"
#include "../../drivers/serial.h"
#include "../../mm/heap.h"

static const char *exception_names[32] = {
    "Divide Error", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
    "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present",
    "Stack-Segment Fault", "General Protection Fault", "Page Fault", "Reserved",
    "x87 Floating Point", "Alignment Check", "Machine Check", "SIMD Floating Point",
    "Virtualization", "Control Protection", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection", "VMM Communication", "Security", "Reserved"
};

void isr_handler(struct interrupt_frame *frame)
{
    if (frame->vector_num == 14) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

        if (heap_handle_page_fault(cr2)) {
            return; /* fixed -- resume the faulting instruction via iretq */
        }

        /* Not a heap fault -- fall through to the fatal path below,
         * but print CR2 since we already have it. */
        serial_write("\n--- EXCEPTION ---\n");
        serial_write(exception_names[frame->vector_num]);
        serial_write("\nvector=0x");
        serial_write_hex(frame->vector_num);
        serial_write(" error_code=0x");
        serial_write_hex(frame->error_code);
        serial_write(" rip=0x");
        serial_write_hex(frame->rip);
        serial_write(" faulting_addr(CR2)=0x");
        serial_write_hex(cr2);
        serial_write("\n");

        for (;;) {
            __asm__ ("hlt");
        }
    }

    serial_write("\n--- EXCEPTION ---\n");
    serial_write(exception_names[frame->vector_num]);
    serial_write("\nvector=0x");
    serial_write_hex(frame->vector_num);
    serial_write(" error_code=0x");
    serial_write_hex(frame->error_code);
    serial_write(" rip=0x");
    serial_write_hex(frame->rip);
    serial_write("\n");

    for (;;) {
        __asm__ ("hlt");
    }
}