#include "isr.h"
#include "../../drivers/serial.h"

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