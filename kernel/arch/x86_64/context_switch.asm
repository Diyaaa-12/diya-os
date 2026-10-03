bits 64

section .text
global context_switch

; void context_switch(uint64_t *old_rsp_ptr, uint64_t new_rsp)
; SysV ABI: rdi = old_rsp_ptr, rsi = new_rsp
context_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp    ; save the CURRENT task's stack pointer into *old_rsp_ptr
    mov rsp, rsi       ; switch onto the NEXT task's stack

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret                ; pops whatever return address sits at the new rsp --
                        ; either a real caller (resuming a yielded task) or
                        ; our crafted fake one (starting a new task)