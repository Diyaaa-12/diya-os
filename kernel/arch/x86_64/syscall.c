#include "syscall.h"
#include "../../drivers/serial.h"

/* sys_write: rdi = pointer to a null-terminated string, rsi/rdx unused
 * for now. NOTE -- a real OS would validate that rdi actually points
 * into THIS process's legitimately mapped, user-accessible memory
 * before touching it (an unvalidated pointer lets user code make the
 * kernel read/write arbitrary kernel memory). We skip that validation
 * here, deliberately and explicitly, since Milestone 11's separate
 * per-process address spaces are what make that check meaningful --
 * right now user and kernel still share one address space, so the
 * check would be security theater. This is a documented limitation,
 * not an oversight. */
static int64_t sys_write(const char *str)
{
    serial_write(str);
    return 0;
}

static void sys_exit(void)
{
    serial_write("\n[syscall] sys_exit called -- halting\n");
    for (;;) { __asm__ ("hlt"); }
}

void syscall_dispatch(struct syscall_frame *frame)
{
    switch (frame->rax) {
        case SYS_WRITE:
            frame->rax = (uint64_t)sys_write((const char *)frame->rdi);
            break;
        case SYS_EXIT:
            sys_exit();
            break;
        default:
            frame->rax = (uint64_t)-1; /* unknown syscall */
            break;
    }
}