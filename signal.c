#include "proc.h"
#include "spinlock.h"

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

sighandler_t signal(int signum, sighandler_t handler);


int 
Kill(pid_t pid, int sig)
{
	if(pid == 1)
	{
		cprintf("Error : Operation not allowed\n");
		return -1;
	}
	
	/* if sig is sigkill, immediately set process to zombie
	   first check if the sender has enough pl to send signals,  
	
	*/
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
