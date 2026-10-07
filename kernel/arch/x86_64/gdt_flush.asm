bits 64

section .text
global gdt_flush
global tss_flush

; void gdt_flush(uint64_t gdtp_addr)
gdt_flush:
    lgdt [rdi]

    mov ax, 0x10         ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    push 0x08             ; kernel code selector
    lea rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    ret

; void tss_flush(void)
tss_flush:
    mov ax, 0x28           ; TSS selector
    ltr ax                 ; Load Task Register -- tells CPU where the TSS is
    ret