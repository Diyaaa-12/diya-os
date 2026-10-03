bits 64

section .text
global gdt_flush

; void gdt_flush(uint64_t gdtp_addr)
; SysV ABI: first argument arrives in RDI
gdt_flush:
    lgdt [rdi]          ; load the GDTR with our table's address/limit

    mov ax, 0x10         ; kernel data selector = entry 2 -> index*8 = 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload CS via a far return: push the target selector and address,
    ; then retfq pops both and jumps -- the standard way to reload CS
    ; in 64-bit mode without a direct far jmp needing special encoding.
    push 0x08             ; kernel code selector = entry 1 -> index*8 = 0x08
    lea rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    ret