#include "proc.h"
#include "spinlock.h"
#include "mmu.h"

extern struct proctable ptable;

int issig(struct proc *p)
{
  return p->sigpending; 
}



int inline 
ign()
{
	//do nothing
}

int 
core(struc proc *q)
{
	//procdump();
	//dump core here. find a way
	kill(q->pid);
}

int 
term(struc proc *q)
{
  kill(q->pid);
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
    	}
    	break;
    }
}

int 
stop(struc proc *q)
{
	q->state = SLEEPING;
}




int psig(struct proc *p)
{
  if(p->sigpending & 1 << 9 == 1 << 9)
  {
    p->sigpending = p->sigpending
    kill(p->pid);
  }
  else if(p->sigpending & 1 << 18 == 1 << 18)
    //nope write functions first
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
