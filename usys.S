#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
# 111713007 : syscalls for signal handling

SYSCALL(Kill)
SYSCALL(raise)
SYSCALL(sigsetmask)
SYSCALL(siggetmask)
#SYSCALL(sigreturn)
# doesn't seem to be working like this, trying to expand the macro.

.globl sigreturn;
sigreturn:
	movl $SYS_sigreturn, %eax
	int $T_SYSCALL
	ret

# need to write a special function for signal . Also need to pass address of sigreturn here, without bothering user
.globl signal;
signal:
	subl $4, %esp
	movl 4(%esp), %eax
	movl %eax, (%esp)
	movl $sigreturn, %eax
	movl %eax, 4(%esp)
	movl $SYS_signal, %eax
	int $T_SYSCALL
	ret

