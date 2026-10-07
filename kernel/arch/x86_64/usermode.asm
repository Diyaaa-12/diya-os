bits 64

section .text
global enter_usermode

; void enter_usermode(uint64_t entry, uint64_t user_stack_top)
; rdi = entry point, rsi = user stack top
enter_usermode:
    mov ax, 0x23        ; user data selector (RPL=3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; NOTE: ss is NOT loaded here -- iretq sets SS itself from the
    ; fake frame below. Loading it early would be premature (we're
    ; still in ring 0, loading a DPL=3 selector into ss while CPL=0
    ; would fault immediately).

    push 0x23            ; SS (user data, RPL=3)
    push rsi              ; RSP (user stack top)
    pushfq                ; RFLAGS (copy current flags -- includes IF=1)
    push 0x1B             ; CS (user code, RPL=3)
    push rdi               ; RIP (entry point)

    iretq