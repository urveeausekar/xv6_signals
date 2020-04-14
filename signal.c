#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"


struct proctable{
	struct spinlock lock;
	struct proc proc[NPROC];
};

extern sighandler_t sigreturn;

extern struct proctable ptable;



int def_disposition[32] = {0, TERM, TERM, CORE, CORE, 0, CORE, 0, CORE, TERM, TERM, CORE, TERM, TERM, TERM, TERM, 0, IGNORE, CONT, STOP, STOP, STOP, STOP};


int issig(struct proc *p)
{
	return p->sigpending & (~(p->sigblocked)); 
}



int
ign(void)
{
	return 0;
}




void
coredump(void)
{
	static char *states[] = {
	[UNUSED]    "unused",
	[EMBRYO]    "embryo",
	[SLEEPING]  "sleep ",
	[RUNNABLE]  "runble",
	[RUNNING]   "run   ",
	[ZOMBIE]    "zombie"
	};
	int i;
	struct proc *p = myproc();
	char *state;
	uint pc[10];


	if(p->state != UNUSED){
		
		if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";
		cprintf("%d %s %s", p->pid, state, p->name);
		if(p->state == SLEEPING){
			getcallerpcs((uint*)p->context->ebp+2, pc);
			for(i=0; i<10 && pc[i] != 0; i++)
				cprintf(" %p", pc[i]);
		}
	}
	cprintf("\n");
  
}






void
core(struct proc *p, int * done)
{
	coredump();
	release(&ptable.lock);
	kill(p->pid);
	acquire(&ptable.lock);
	*done = 1;
}

void
term(struct proc *q, int * done)
{
	release(&ptable.lock);
	kill(q->pid);
	acquire(&ptable.lock);
	*done = 1;
	
}



// No, process won't ever be scheduled because it is sleeping,
// so psig never called on a sleeping process
// So, Kill must alter state of process and make it runnable, not cont
void
cont(struct proc *p)
{
		
	if(p->state == SLEEPING)
	{
		p->state = RUNNABLE;
		p->justwoken = 1;
	}
			
		
}

void
stop(struct proc *q, int *done)
{	
	q->state = SLEEPING;
	sched();

	*done = 1;
}


// PAGEBREAK!
// psig handler all signals that can be handled by the kernel
// If there are any user defined handler, it sets up user stack for user handler to run and jumps to user handler

int psig(struct proc *p)
{
  
  
  
	int i;
	int stopsig[4] = {19, 20, 21, 22};
	int coresig[5] = {SIGQUIT, SIGABRT, SIGILL, SIGFPE, SIGSEGV};
	int termsig[7] = {SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2};
	int termcalled = 0, corecalled = 0, stopcalled = 0;
  
  
	if(!p)
		return -1;
  
	acquire(&ptable.lock);
  
	//first deal with all the terms. No logic in doing other things if process is going to get terminated.
  
  
	for(i = 0; i < 7; i++){
		if(((p->sigpending & (1 << termsig[i])) == (1 << termsig[i])) && ((p->sigblocked & (1 << termsig[i])) == 0) && p->allinfo[termsig[i]].disposition == DFL){
			if(termcalled == 0)
				term(p, &termcalled);
			p->sigpending = p->sigpending & (~(1 << termsig[i]));
		}
	}
  
  
	//If default action is dumping a core and terminating
	for(i = 0; i < 5; i++){
		if(((p->sigpending & (1 << coresig[i])) == (1 << coresig[i])) && ((p->sigblocked & (1 << coresig[i])) == 0) && p->allinfo[coresig[i]].disposition == DFL){
			if(corecalled == 0)
				core(p, &corecalled);
			p->sigpending = p->sigpending & (~(1 << coresig[i]));
		}
	}
  
  
  
  
  
	//SIGSTOP can't be masked, ignored or handled
	if((p->sigpending & (1 << 19)) == (1 << 19)){	
		stop(p, &stopcalled);
		p->sigpending = p->sigpending & (~(1 << 19));
	}
  
  
	if(((p->sigpending & (1 << 17)) == (1 << 17)) && ((p->sigblocked & (1 << 17)) == 0) && p->allinfo[SIGCHLD].disposition == DFL){
		ign();
		p->sigpending = p->sigpending & (~(1 << 17));
	}
  
  
	//1 SIGCONT and 1 other signal that causes a stop cancel each other out
	for(i = 0; i < 4 ; i++){
		if(((p->sigpending & (1 << stopsig[i])) == (1 << stopsig[i])) && ((p->sigblocked & (1 << stopsig[i])) == 0) && p->allinfo[stopsig[i]].disposition == DFL && ((p->sigpending & (1 << 18)) == (1 << 18)) && ((p->sigblocked & (1 << 18)) == 0) && p->allinfo[SIGCONT].disposition == DFL){
			
			p->sigpending = p->sigpending & (~(1 << 18));
			p->sigpending = p->sigpending & (~(1 << stopsig[i]));
		}
		else if (((p->sigpending & (1 << stopsig[i])) == (1 << stopsig[i])) && ((p->sigblocked & (1 << stopsig[i])) == 0) && p->allinfo[stopsig[i]].disposition == DFL){
			// i.e no sigcont, only a signal which causes a stop
			if(stopcalled == 0)
				stop(p, &stopcalled);
			p->sigpending = p->sigpending & (~(1 << stopsig[i]));
		}
	}
  
	//If there is only a sigcont
	if(((p->sigpending & (1 << 18)) == (1 << 18)) && ((p->sigblocked & (1 << 18)) == 0) && p->allinfo[SIGCONT].disposition == DFL){
		cont(p);
		p->sigpending = p->sigpending & (~(1 << 18));
	}
  
  
	//Now if deal with signals that have userdefined handlers (if they are pending)
	if(p->userdefed == 0){
		release(&ptable.lock);
		return 1;
	}
	else{
		for(i = 1; i < 32; i++){
			if((p->userdefed & (1 << i)) == (1 << i))
				break;		
			//i is the signal number that is pending
		}


	*(uint *)((p->tf->esp) - 4) = p->tf->ss;
	*(uint *)((p->tf->esp) - 8) = p->tf->esp;
	*(uint *)((p->tf->esp) - 12) = p->tf->eflags;
	*(uint *)((p->tf->esp) - 16) = p->tf->cs;
	*(uint *)((p->tf->esp) - 20) = p->tf->eip;
	*(uint *)((p->tf->esp) - 24) = p->tf->ds;
	*(uint *)((p->tf->esp) - 28) = p->tf->es;
	*(uint *)((p->tf->esp) - 32) = p->tf->fs;
	*(uint *)((p->tf->esp) - 36) = p->tf->gs;
	*(uint *)((p->tf->esp) - 40) = p->tf->eax;
	*(uint *)((p->tf->esp) - 44) = p->tf->ecx;
	*(uint *)((p->tf->esp) - 48) = p->tf->edx;
	*(uint *)((p->tf->esp) - 52) = p->tf->ebx;
	*(uint *)((p->tf->esp) - 56) = p->tf->oesp;
	*(uint *)((p->tf->esp) - 60) = p->tf->ebp;
	*(uint *)((p->tf->esp) - 64) = p->tf->esi;
	*(uint *)((p->tf->esp) - 68) = p->tf->edi;
	//context of kernel saved on user stack

	*(uint *)((p->tf->esp) - 72) = i;
	*(uint *)((p->tf->esp) - 76) = (uint)p->addr_sigreturn;
	p->tf->esp = p->tf->esp - 76;
	
	p->sigpending = p->sigpending & (~(1 << i));
	release(&ptable.lock);
	
	__sync_synchronize();
	
	//jmptohandler jumps back to user space and starts running the user defined handler
	jmptohandler(p->allinfo[i].handler, p->tf->cs, p->tf->eflags, p->tf->esp, p->tf->ss);

	}
	//won't reach here. But for the sake of completeness.
	return -1;

 
}

// PAGEBREAK!
// After the user handler finishes execution,we must restore context of the user process that was,before it got interrupted
// So, kernel sigreturn calls restoreuser() which restores all registers and jumps back to user process

void
kernel_sigreturn(int signal)
{
	
	struct proc *p = myproc();
	if(!p)
		return;
		
	uint cs, ds, es, ss, fs, gs;
	uint eax, ebx, ecx, edx, esi, edi, esp, ebp, oesp, eip, eflag;
  	
  	esp = p->tf->esp;	//This is current userstack esp, which currently points to signum argument of user handler
  	edi = *(uint *)(esp + 4);
  	esi = *(uint *)(esp + 8);
  	ebp = *(uint *)(esp + 12);
  	oesp = *(uint *)(esp + 16);
  	ebx = *(uint *)(esp + 20);
  	edx = *(uint *)(esp + 24);
  	ecx = *(uint *)(esp + 28);
  	eax = *(uint *)(esp + 32);
  	gs = *(uint *)(esp + 36);
  	fs = *(uint *)(esp + 40);
  	es = *(uint *)(esp + 44);
  	ds = *(uint *)(esp + 48);
  	eip = *(uint *)(esp + 52);
  	cs = *(uint *)(esp + 56);
  	eflag = *(uint *)(esp + 60);
  	ss = *(uint *)(esp + 68);
  	esp = *(uint *)(esp + 64);
  	
  	restoreuser(edi, esi, ebp, oesp, ebx, edx, ecx, eax, gs, fs, es, ds, eip, cs, eflag, esp, ss);
}




// specifies handlers for signals.
// the returnfn specifies the user address of the systemcall sigreturn, 
// which we will need if we want to execute a user defined handler
// ignores attempt to ignore or set handler for SIGKILL and SIGSTOP

sighandler_t
signal(addr_sigret returnfn, int signum, sighandler_t handler)
{
	if(signum <= 0 || signum > CURRNUM)
		return SIG_ERR;
		
	if(signum == SIGKILL || signum == SIGSTOP)
		return SIG_ERR;
	
	
	struct proc *p = myproc();
	
	if(!p)
		return SIG_ERR;
		
	sighandler_t prev;
	if(p->allinfo[signum].disposition == USERDEF)
		prev = p->allinfo[signum].handler;
	else if(p->allinfo[signum].disposition == IGN)
		prev = SIG_IGN;
	else
		prev = SIG_DFL;
	
	acquire(&ptable.lock);

	
	p->addr_sigreturn = NULL;
	if(handler == SIG_IGN){
		if(p->allinfo[signum].disposition == USERDEF)
			p->userdefed = p->userdefed & (~(1 << signum));
		
		p->allinfo[signum].disposition = IGN;

	}
	else if(handler == SIG_DFL){
		if(p->allinfo[signum].disposition == USERDEF)
			p->userdefed = p->userdefed & (~(1 << signum));
			
		p->allinfo[signum].disposition = DFL;
		
	}
	else{
		p->allinfo[signum].disposition = USERDEF;
		p->userdefed = p->userdefed | (1 << signum);
		p->allinfo[signum].handler = handler;
		p->addr_sigreturn = returnfn;
	}
		
	release(&ptable.lock);
	return prev;
}



// In unix, we check user id, etc, but in xv6 no such distinctions 
// - every process runs as root. 
// Therefore forbid user processes from signalling kernel processes,
// and forbid everyone from signalling init.
int 
Kill(pid_t pid, int sig)
{

	if(sig <= 0 || sig > CURRNUM)
		return -1;
	
	if(pid == 1 || pid == 0 || pid < -1)
	{
		cprintf("Operation not allowed\n");
		return 0;
	}
	
	int receiver_pl = 10, sender_pl = 10;
	struct proc *p;
	struct proc *curproc = myproc();
	
	if(!curproc)
		return -1;
	
	int minusone = 0;
	
	if(pid == -1)
		minusone = 1;
	
	//search for pid in table
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    
		if(p->pid == 1)
    			continue;
    	
		if(minusone || p->pid == pid){
    		//check privilege level
			receiver_pl = p->tf->cs & DPL_USER;
			sender_pl = curproc->tf->cs & DPL_USER;
			if(receiver_pl != 3 && sender_pl == 3){
				if(minusone)
					continue;
				else
					return 0;
			}
    	
	    		// if sig is sigkill, immediately set process to zombie
			// SIGKILL is generated under some unusual conditions where
			// the program cannot possibly continue to run,
			//(even to run a signal handler), so terminate it here 
			// instead of waiting for sigkilled process to run.
				
			acquire(&ptable.lock);
			
			if(sig == SIGKILL){
	
				release(&ptable.lock);
				kill(p->pid);
				
				if(minusone)
					continue;
				else
					return 0;
			}
			else if(sig == SIGCHLD && p->allinfo[SIGCHLD].disposition == DFL){
				release(&ptable.lock);
				if(minusone)
					continue;
				else{
					return 0;
					
				}
			}
			else if(sig == SIGCONT){
				
				// sigcont must be dealt with here,
				// otherwise the process will never be scheduled
				
				if(p->state == SLEEPING){
					p->state = RUNNABLE;
					p->justwoken = 1;
				}
				if(p->allinfo[SIGCONT].disposition != DFL && p->allinfo[SIGCONT].disposition != IGN){
					p->sigpending = p->sigpending | (1 << sig);
				}
				release(&ptable.lock);
				if(minusone)
					continue;
				else
					return 0;
				
			}
	    	
			if(p->allinfo[sig].disposition != IGN && sig != 0){
				p->sigpending = p->sigpending | (1 << sig);
			}

			release(&ptable.lock);
			if(minusone == 0)
				break;
    		
    	
		}
	}
	
	return 0;
	
	
}

//If the signal causes a handler to be called, raise() will return 
//to user space only after the signal handler has returned.

int raise(int sig)
{
	struct proc *p = myproc();
	
	if(!p)
		return -1;
		
	return Kill(p->pid, sig);
	

}

// PAGEBREAK!
// returns current mask

uint 
siggetmask(void)
{
	struct proc *p = myproc();
	if(!p)
		return -1;
	
	acquire(&ptable.lock);
	uint mask = p->sigblocked;
	release(&ptable.lock);
	
	return mask;
}

// It is not possible to block SIGKILL or SIGSTOP.  
// Attempts to do so are silently ignored
// Returns current mask
uint 
sigsetmask(uint mask)
{
	

	if((mask & (1 << SIGKILL)) || (mask & (1 << SIGSTOP)) || (mask & (1 << SIGCONT)))
		return 0;
	if(mask & 1)
		return 0;
	
	struct proc *p = myproc();
	
	if(!p)
		return -1;
	
	
	acquire(&ptable.lock);
	p->sigblocked = mask;
	release(&ptable.lock);
	return mask;
}

// To block 1 particular signal. Easy for user
// Both sigblock and sigunblock return current mask
// Except when argument is SIGKILL or SIGSTOP or SIGCONT. Then they return 0.
// If the value of sig is invalid, -1 is returned.

uint
sigblock(int sig)
{
	if(sig <= 0 || sig > CURRNUM)
		return -1;
		
	if(sig == SIGKILL || sig == SIGSTOP || sig == SIGCONT)
		return 0;
	uint mask = siggetmask();
	mask = mask | (1 << sig);
	return (sigsetmask(mask));
}


uint 
sigunblock(int sig)
{
	if(sig <= 0 || sig > CURRNUM)
		return -1;
		
	if(sig == SIGKILL || sig == SIGSTOP)
		return 0;
	uint mask = siggetmask();
	mask = mask & (~(1 << sig));
	return (sigsetmask(mask));
}
