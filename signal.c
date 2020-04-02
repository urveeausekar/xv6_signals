#include "proc.h"
#include "spinlock.h"
#include "mmu.h"

extern struct proctable ptable;


void
checkforsignals(struct proc *p)
{
	if(issig(p))
		psig(p);
}



int issig(struct proc *p)
{
  return p->sigpending & (!p->sigblocked); 
}



int inline 
ign()
{
	//do nothing
}

int 
core(struc proc *q, , int * done;)
{
	procdump();
	//FIXME : dump core here. find a way
	kill(q->pid);
	*done = 1;
}

int 
term(struc proc *q, int * done)
{
  kill(q->pid);
  *done = 1;
}

int 
cont(struc proc *q)
{
	struct proc p;
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

int 
stop(struc proc *q, , int * done;)
{
  acquire(&ptable.lock);
	
  q->state = SLEEPING;
  sched();
  
  release(&ptable.lock);
  *done = 1;
}




int psig(struct proc *p)
{
  int i, esp, ss, eax, ebx, ecx, eds, esi, edi, ebp, cs, ds, es, fs, gs, eip, eflag;
  
  int stopsig[4] = {19, 20, 21, 22};
  int coresig[5] = {SIGQUIT, SIGABRT, SIGILL, SIGFPE, SIGSEGV};
  int termsig[7] = {SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2};
  int termcalled = 0, corecalled = 0, stopcalled = 0;
  
  //first deal with all the terms. No logic in doing other things if process is going to get terminated.
  
  for(i = 0; i < 7; i++){
  	if((p->sigpending & (1 << termsig[i]) == (1 << termsig[i])) && (p->sigblocked & (1 << termsig[i]) == 0) && p->allinfo[termsig[i]].disposition == SIG_DFL){
  		if(termcalled == 0)
  			term(p, &termcalled);
  		p->sigpending = p->sigpending & (! (1 << termsig[i]));
  	}
  }
  
  
  //If default action is dumping a core and terminating
  for(i = 0; i < ; i++){
  	if((p->sigpending & (1 << coresig[i]) == (1 << coresig[i])) && (p->sigblocked & (1 << coresig[i]) == 0) && p->allinfo[coresig[i]].disposition == SIG_DFL){
  		if(corecalled == 0)
  			term(p, &corecalled);
  		p->sigpending = p->sigpending & (! (1 << coresig[i]));
  	}
  }
  
  
  
  
  
  //SIGSTOP can't be masked, ignored or handled
  if(p->sigpending & (1 << 19) == (1 << 19))
  {
    stop(p);
    p->sigpending = p->sigpending & (! (1 << 19));
  }
  
  
  if((p->sigpending & (1 << 17) == (1 << 17)) && (p->sigblocked & (1 << 17) == 0) && p->allinfo[SIGCHLD].disposition == SIG_DFL){
  	ign();
  	p->sigpending = p->sigpending & (! (1 << 17));
  }
  
  
  //1 SIGCONT and 1 other signal that causes a stop cancel each other out
  for(i = 0; i < 4 ; i++){
  	if((p->sigpending & (1 << stopsig[i]) == (1 << stopsig[i])) && (p->sigblocked & (1 << stopsig[i]) == 0) && p->allinfo[stopsig[i]].disposition == SIG_DFL && (p->sigpending & (1 << 18) == (1 << 18)) && (p->sigblocked & (1 << 18) == 0) && p->allinfo[SIGCONT].disposition == SIG_DFL){
  		
  		p->sigpending = p->sigpending & (! (1 << 18));
  		p->sigpending = p->sigpending & (! (1 << stopsig[i]));
  	}
  	else if ((p->sigpending & (1 << stopsig[i]) == (1 << stopsig[i])) && (p->sigblocked & (1 << stopsig[i]) == 0) && p->allinfo[stopsig[i]].disposition == SIG_DFL){
  		// i.e no sigcont, only a signal which causes a stop
  		if(stopcalled == 0)
  			stop(p, &stopcalled);
  		p->sigpending = p->sigpending & (! (1 << stopsig[i]));
  	}
  }
  
  //If there is only a sigcont
  if((p->sigpending & (1 << 18) == (1 << 18)) && (p->sigblocked & (1 << 18) == 0) && p->allinfo[SIGCONT].disposition == SIG_DFL){
  	cont(p);
  	p->sigpending = p->sigpending & (! (1 << 18));
  }
  
  
  //Now if signals are userdefined.
	if(p->userdefed == 0)
		return 1;
	else{
		for(i = 1; i < 32; i++){
			if(p->userdefed & (1 << i) == (1 << i)
				break;		//i is the signal number that is pending
		}
	// asm volatile("movl $0, %0" : "+m" (lk->locked) : ); example asm usage
	/*asm volatile ("movl %%esp, %0" : "=r"(esp) : );
	asm volatile ("movl %%ss, %0" : "=r"(ss) : );
	asm volatile ("movl %%cs, %0" : "=r"(cs) : );
	asm volatile ("movl %%ds, %0" : "=r"(ds) : );
	asm volatile ("movl %%es, %0" : "=r"(es) : );
	asm volatile ("movl %%eax, %0" : "=r"(eax) : );
	asm volatile ("movl %%ebx, %0" : "=r"(ebx) : );
	asm volatile ("movl %%ecx, %0" : "=r"(ecx) : );
	asm volatile ("movl %%edx, %0" : "=r"(edx) : );
	asm volatile ("movl %%esi, %0" : "=r"(esi) : );
	asm volatile ("movl %%edi, %0" : "=r"(edi) : );
	asm volatile ("movl %%ebp, %0" : "=r"(ebp) : );
	asm volatile ("movl %%fs, %0" : "=r"(fs) : );
	asm volatile ("movl %%gs, %0" : "=r"(gs) : );
	asm volatile ("movl %%eflag, %0" : "=r"(eflag) : );
	asm volatile ("movl %%eip, %0" : "=r"(eip) : );*/

	*((p->tf->esp) - 4) = p->tf->eflag;
	*((p->tf->esp) - 8) = p->tf->cs;
	*((p->tf->esp) - 12) = p->tf->eip;
	*((p->tf->esp) - 16) = p->tf->ds;
	*((p->tf->esp) - 20) = p->tf->es;
	*((p->tf->esp) - 24) = p->tf->fs;
	*((p->tf->esp) - 28) = p->tf->gs;
	*((p->tf->esp) - 32) = p->tf->eax;
	*((p->tf->esp) - 36) = p->tf->ecx;
	*((p->tf->esp) - 40) = p->tf->edx;
	*((p->tf->esp) - 44) = p->tf->ebx;
	*((p->tf->esp) - 48) = p->tf->esp;
	*((p->tf->esp) - 52) = p->tf->ebp;
	*((p->tf->esp) - 56) = p->tf->esi;
	*((p->tf->esp) - 60) = p->tf->edi;
	//*((p->tf->esp) - 64) = p->tf->esi;
	//*((p->tf->esp) - 68) = p->tf->edi;
	//context of kernel saved on user stack

	*((p->tf->esp) - 64) = i;
	*((p->tf->esp) - 68) = sigreturn; //check this

	asm volatile ("movl %0, %%esp" : :((p->tf->esp) - 68));  //switch to user's stack
	asm volatile ("movw %0, %%ss" : :(p->tf->ss));		//switch to user ss
	asm volatile ("movl %0, %%cs" : : (p->tf->cs));
	/*asm volatile ("movl %%eip, %0" : "=r"(eip) : );
	*((p->tf->esp) - 20) = eip;*/

	asm volatile ("movl %0, %%eip" : : (p->allinfo[i].handler)); //start running handler




	}

 
}


// Code for the syscalls that the user will call.

void
kernel_sigreturn(int signal)
{
	struct proc *p = myproc();
	p->userdefed = p->userdefed & (! (1 << signal));
  	p->sigpending = p->sigpending & (! (1 << signal));
  	int esp = p->tf->esp, ss = p->tf->ss;
  	restoreuser(ss, esp);
}






int
signal(int signum, sighandler_t handler)
{
	if(signum == 0)
		return -1;
	
	struct proc *p = myproc();
	
	acquire(&ptable.lock);
	
	if(handler == SIG_IGN)
		p->allinfo[signum].disposition = SIG_IGN;
	else if(handler == SIG_DFL)
		p->allinfo[signum].disposition = SIG_DFL;
	else{
		p->allinfo[signum].disposition = SIG_USERDEF;
		p->userdefed = p->sigblocked | (1 << signum);
		p->allinfo[signum].handler = handler;
	}
		
	release(&ptable.lock);
	return 0;
}



//In unix, we check user id, etc, but in xv6 no such distinctions - every process runs as root. 
//Therefore forbid user processes from signalling kernel processes, and forbid everyone from signalling init.
int 
Kill(pid_t pid, int sig)
{
	
	if(pid == 1 || pid == 0 || pid < -1)
	{
		cprintf("Operation not allowed\n");
		return -1;
	}
	
	int fd, receiver_pl = 10, sender_pl = 10;
	struct proc *p, *curproc = myproc();
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
					continue
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
					continue
				else
					return 0;
			}
	    	
			if(p->handlerinfo[sig].disposition != SIG_IGN && sig != 0)
				p->sigpending = p->sigpending | (1 << sig);

			release(&ptable.lock);
			if(minusone == 0)
				break;
    		
    	
		}
	}
	return 0;
	
	
}


int raise(int sig)
{
	struct proc *p = myproc();
	return Kill(p->pid, sig);
	//FIXME : If the signal causes a handler to be called, raise() will  return  only after the signal handler has returned.

}


uint 
siggetmask(void)
{
	struct proc *p = myproc();
	
	acquire(&ptable.lock);
	uint mask = p->sigblocked;
	release(&ptable.lock);
	
	return mask;
}

uint 
sigsetmask(uint mask)
{
	struct proc *p = myproc();
	
	acquire(&ptable.lock);
	p->sigblocked = mask;
	release(&ptable.lock);
	
	return mask;
}
