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
  int i;
  
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
  	
  }
 
 
}







//---------------------------- CODE FOR SYSCALLS ----------------------------------

sighandler_t 
signal(int signum, sighandler_t handler)
{
	if(signum == 0)
		return NULL;
	
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
	
	
}


int raise(int sig)
{
	struct proc *p = myproc();
	Kill(p->pid, sig);
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
