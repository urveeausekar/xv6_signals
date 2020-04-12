#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_sigsetmask(void)
{
	int mask;
	if(argint(0, &mask) < 0)
    		return -1;
  	return sigsetmask(mask);
}

int
sys_siggetmask(void)
{
	return siggetmask();
}


int
sys_raise(void)
{
	int pid;
	if(argint(0, &pid) < 0)
    		return -1;
  	return raise(pid);
}


int
sys_Kill(void)
{
	int pid, signal;
	if(argint(0, &pid) < 0)
		return -1;
	if(argint(1, &signal) < 0)
		return -1;
	return Kill(pid, signal);
}

//FIXME:check if this cast okay
sighandler_t
sys_signal(void)
{
	int signum;
	int ptr1, sigretfn1;
	sighandler_t ptr;
	addr_sigret sigretfn;
	
	if(argint(0, &sigretfn1) < 0)
		return SIG_ERR;
	if(argint(1, &signum) < 0)
		return SIG_ERR;
	if(argint(2, &ptr1) < 0)
		return SIG_ERR;
		
	//cprintf("in syssignal sigretfn is %d\n", sigretfn1);
	//cprintf("in syssignal signum is %d\n", signum);
	//cprintf("in syssignal handler is %d\n", ptr1);
	ptr = (sighandler_t)ptr1;
	sigretfn = (addr_sigret)sigretfn1;
	return signal(sigretfn, signum, ptr);
	
}

int
sys_sigreturn(void)
{
	int signum;
	//We have not come to sigreturn through a call instruction, but we have edited stack ourselves. So ip is not pushed.
	//Therefore, the user esp pointed directly to the argument instead of to the ip
	//Therefore we modify argint.
	fetchint((myproc()->tf->esp), &signum);
	kernel_sigreturn(signum);
	//will never return. If it does return, something has gone wrong, so return -1.
	return -1;
}

int
sys_sigblock(void)
{
	int signum;
	if(argint(0, &signum) < 0)
    		return -1;
    		
    	return sigblock(signum);
}

int
sys_sigunblock(void)
{
	int signum;
	if(argint(0, &signum) < 0)
    		return -1;
    		
    	return sigunblock(signum);
}
