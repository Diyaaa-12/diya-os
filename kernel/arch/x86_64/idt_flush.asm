bits 64

section .text
global idt_flush

; void idt_flush(uint64_t idtp_addr)
idt_flush:
    lidt [rdi]
    ret