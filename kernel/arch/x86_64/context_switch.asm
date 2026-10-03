bits 64

section .text
global context_switch
global task_trampoline

; void context_switch(uint64_t *old_rsp_ptr, uint64_t new_rsp)
context_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp
    mov rsp, rsi

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret

; Landing point for a task's VERY FIRST run. rbx holds the real entry
; point (placed there by task_create -- see the fake-stack layout).
; Interrupts may be disabled here (if this task's first run happens to
; be triggered from inside the timer interrupt handler), so we must
; explicitly re-enable them before the task starts running -- otherwise
; the whole system silently loses preemption forever from this point on.
task_trampoline:
    sti
    jmp rbx