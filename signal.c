/*#include "types.h"
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

int def_disposition[32] = {0, TERM, TERM, CORE, CORE, 0, CORE, 0, CORE, TERM, TERM, CORE, TERM, TERM, TERM, TERM, 0, IGN, CONT, STOP, STOP, STOP, STOP};


int issig(struct proc *p)
{
  return p->sigpending & (!p->sigblocked); 
}



int
ign(void)
{
	return 0;
}

void
core(struct proc *p, int * done)
{
	procdump();
	kill(p->pid);
	*done = 1;
}

void
term(struct proc *q, int * done)
{
  kill(q->pid);
  *done = 1;
}

void
cont(struct proc *q)
{
	struct proc *p;
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if(p->pid == q->pid)
		{
			if(p->state == SLEEPING)
			{
				p->state = RUNNABLE;
				p->justwoken = 1;
			}
			break;
		}
}

void
stop(struct proc *q, int *done)
{
  acquire(&ptable.lock);
	
  q->state = SLEEPING;
  sched();
  
  release(&ptable.lock);
  *done = 1;
}


// PAGEBREAK!
// psig handler all signals that can be handled by the kernel
// If there are any user defined handler, it sets up user stack for user handler to run and jumps to user handler

int psig(struct proc *p)
{
  
  if(!p)
  	return -1;
  
  int i;
  cprintf("in psig\n");
  int stopsig[4] = {19, 20, 21, 22};
  int coresig[5] = {SIGQUIT, SIGABRT, SIGILL, SIGFPE, SIGSEGV};
  int termsig[7] = {SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2};
  int termcalled = 0, corecalled = 0, stopcalled = 0;
  
  //first deal with all the terms. No logic in doing other things if process is going to get terminated.
  
  for(i = 0; i < 7; i++){
  	if(((p->sigpending & (1 << termsig[i])) == (1 << termsig[i])) && ((p->sigblocked & (1 << termsig[i])) == 0) && p->allinfo[termsig[i]].disposition == SIG_DFL){
  		if(termcalled == 0)
  			term(p, &termcalled);
  		p->sigpending = p->sigpending & (! (1 << termsig[i]));
  	}
  }
  
  
  //If default action is dumping a core and terminating
  for(i = 0; i < 5; i++){
  	if(((p->sigpending & (1 << coresig[i])) == (1 << coresig[i])) && ((p->sigblocked & (1 << coresig[i])) == 0) && p->allinfo[coresig[i]].disposition == SIG_DFL){
  		if(corecalled == 0)
  			term(p, &corecalled);
  		p->sigpending = p->sigpending & (! (1 << coresig[i]));
  	}
  }
  
  
  
  
  
  //SIGSTOP can't be masked, ignored or handled
  if((p->sigpending & (1 << 19)) == (1 << 19))
  {	
    stop(p, &stopcalled);
    p->sigpending = p->sigpending & (! (1 << 19));
  }
  
  
  if(((p->sigpending & (1 << 17)) == (1 << 17)) && ((p->sigblocked & (1 << 17)) == 0) && p->allinfo[SIGCHLD].disposition == SIG_DFL){
  	ign();
  	p->sigpending = p->sigpending & (! (1 << 17));
  }
  
  
  //1 SIGCONT and 1 other signal that causes a stop cancel each other out
  for(i = 0; i < 4 ; i++){
  	if(((p->sigpending & (1 << stopsig[i])) == (1 << stopsig[i])) && ((p->sigblocked & (1 << stopsig[i])) == 0) && p->allinfo[stopsig[i]].disposition == SIG_DFL && ((p->sigpending & (1 << 18)) == (1 << 18)) && ((p->sigblocked & (1 << 18)) == 0) && p->allinfo[SIGCONT].disposition == SIG_DFL){
  		
  		p->sigpending = p->sigpending & (! (1 << 18));
  		p->sigpending = p->sigpending & (! (1 << stopsig[i]));
  	}
  	else if (((p->sigpending & (1 << stopsig[i])) == (1 << stopsig[i])) && ((p->sigblocked & (1 << stopsig[i])) == 0) && p->allinfo[stopsig[i]].disposition == SIG_DFL){
  		// i.e no sigcont, only a signal which causes a stop
  		if(stopcalled == 0)
  			stop(p, &stopcalled);
  		p->sigpending = p->sigpending & (! (1 << stopsig[i]));
  	}
  }
  
  //If there is only a sigcont
  if(((p->sigpending & (1 << 18)) == (1 << 18)) && ((p->sigblocked & (1 << 18)) == 0) && p->allinfo[SIGCONT].disposition == SIG_DFL){
  	cont(p);
  	p->sigpending = p->sigpending & (! (1 << 18));
  }
  
  
  //Now if signals are userdefined.
	if(p->userdefed == 0){
		cprintf("Out of psig, if no userdefed\n");
		return 1;
	}
	else{
		for(i = 1; i < 32; i++){
			if((p->userdefed & (1 << i)) == (1 << i))
				break;		//i is the signal number that is pending
		}
	// asm volatile("movl $0, %0" : "+m" (lk->locked) : ); example asm usage
	/*

	*(uint *)((p->tf->esp) - 4) = p->tf->eflags;
	*(uint *)((p->tf->esp) - 8) = p->tf->cs;
	*(uint *)((p->tf->esp) - 12) = p->tf->eip;
	*(uint *)((p->tf->esp) - 16) = p->tf->ds;
	*(uint *)((p->tf->esp) - 20) = p->tf->es;
	*(uint *)((p->tf->esp) - 24) = p->tf->fs;
	*(uint *)((p->tf->esp) - 28) = p->tf->gs;
	*(uint *)((p->tf->esp) - 32) = p->tf->eax;
	*(uint *)((p->tf->esp) - 36) = p->tf->ecx;
	*(uint *)((p->tf->esp) - 40) = p->tf->edx;
	*(uint *)((p->tf->esp) - 44) = p->tf->ebx;
	*(uint *)((p->tf->esp) - 48) = p->tf->esp;
	*(uint *)((p->tf->esp) - 52) = p->tf->ebp;
	*(uint *)((p->tf->esp) - 56) = p->tf->esi;
	*(uint *)((p->tf->esp) - 60) = p->tf->edi;
	//*((p->tf->esp) - 64) = p->tf->esi;
	//*((p->tf->esp) - 68) = p->tf->edi;
	//context of kernel saved on user stack

	*(uint *)((p->tf->esp) - 64) = i;
	*(uint *)((p->tf->esp) - 68) = p->addr_sigreturn; //check this


	//Now we push to current stack things so that iret takes us back to level3
	p->tf->esp = p->tf->esp - 68;
	asm volatile("pushl %0" : :"m"(p->tf->ss));
	asm volatile("pushl %0" : :"m"(p->tf->esp));
	asm volatile("pushl %0" : :"m"(p->tf->eflags));
	asm volatile("pushl %0" : :"m"(p->tf->cs));
	asm volatile("pushl %0" : :"m"(p->allinfo[i].handler));
	asm volatile("iret" : : );


	}
	//won't reach here. But still.
	return 0;

 
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
	
	p->userdefed = p->userdefed & (! (1 << signal));
  	p->sigpending = p->sigpending & (! (1 << signal));
  	int esp = p->tf->esp, ss = p->tf->ss;
  	restoreuser(ss, esp);
}




// specifies handlers for signals.
// the returnfn specifies the user address of the systemcall sigreturn, which we will need if we want to execute a user 
// defined handler
// ignores attempt to ignore or set handler for SIGKILL and SIGSTOP

int
signal(addr_sigret returnfn, int signum, sighandler_t handler)
{
	if(signum == 0)
		return -1;
		
	if(signum == SIGKILL || signum == SIGSTOP)
		return 0;
	
	struct proc *p = myproc();
	
	if(!p)
		return -1;
	
	acquire(&ptable.lock);
	
	p->addr_sigreturn = NULL;
	if(handler == (void *)SIG_IGN)
		p->allinfo[signum].disposition = SIG_IGN;
	else if(handler == SIG_DFL)
		p->allinfo[signum].disposition = SIG_DFL;
	else{
		p->allinfo[signum].disposition = SIG_USERDEF;
		p->userdefed = p->sigblocked | (1 << signum);
		p->allinfo[signum].handler = handler;
		p->addr_sigreturn = returnfn;
	}
		
	release(&ptable.lock);
	return 0;
}



//In unix, we check user id, etc, but in xv6 no such distinctions - every process runs as root. 
//Therefore forbid user processes from signalling kernel processes, and forbid everyone from signalling init.
int 
Kill(pid_t pid, int sig)
{
	cprintf("In kill, start\n");
	if(pid == 1 || pid == 0 || pid < -1)
	{
		cprintf("Operation not allowed\n");
		return -1;
	}
	
	int receiver_pl = 10, sender_pl = 10;
	struct proc *p, *curproc = myproc();
	
	if(curproc == 0)
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
    	
	    		//if sig is sigkill, immediately set process to zombie
			//SIGKILL is generated under some unusual conditions where the program cannot possibly continue to run,
			//(even to run a signal handler), so terminate it here instead of waiting for sigkilled process to run.
				
			acquire(&ptable.lock);
			
			if(sig == SIGKILL){
				p->state = ZOMBIE;
				p->killed = 1;
				
				release(&ptable.lock);
				if(minusone)
					continue;
				else
					return 0;
			}
	    	
			if(p->allinfo[sig].disposition != SIG_IGN && sig != 0)
				p->sigpending = p->sigpending | (1 << sig);

			release(&ptable.lock);
			if(minusone == 0)
				break;
    		
    	
		}
	}
	cprintf("out of kill\n");
	cprintf("sigpending is %d\n", p->sigpending);
	return 0;
	
	
}


int raise(int sig)
{
	struct proc *p = myproc();
	
	if(!p)
		return -1;
		
	return Kill(p->pid, sig);
	//FIXME : If the signal causes a handler to be called, raise() will  return  only after the signal handler has returned.

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

uint 
sigsetmask(uint mask)
{
	//It is not possible to block SIGKILL or SIGSTOP.  Attempts to do so are silently ignored
	
	if((mask & (1 << SIGKILL)) || (mask & (1 << SIGSTOP)))
		return 0;
	
	struct proc *p = myproc();
	
	if(!p)
		return -1;
	
	
	acquire(&ptable.lock);
	p->sigblocked = mask;
	release(&ptable.lock);
	
	return mask;
}

*/
