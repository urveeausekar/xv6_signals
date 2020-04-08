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


struct proctable ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // for signal handling
  p->sigpending = 0;
  p->justwoken = 0;


  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  int i;
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  //set signal handlers, masks, etc.
  p->sigblocked = 0;
  p->userdefed = 0;
  for(i = 0; i < NUMSIG; i++)
  {
    p->allinfo[i].disposition = DFL;
    //p->allinfo[i].handler = def_disposition[i]; not necessary
  }
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  
  //copy parent's signal dispositions, mask, etc.
  np->sigblocked = curproc->sigblocked;
  np->userdefed = curproc->userdefed;
  for(i = 0; i < NUMSIG; i++)
    np->allinfo[i] = curproc->allinfo[i];
  
  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        //for signals
        p->justwoken = 0;
        p->sigblocked = 0;
        p->sigpending = 0;
        p->userdefed = 0;
        
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
    		
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  
  //Check if any signals pending. If yes, deal with them
	//if(issig(p))
	//	psig(p);
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;
  
  sched();
	
  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      p->justwoken = 1;
      
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
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
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
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
    cprintf("\n");
  }
}

//PAGEBREAK: 
// Code for signal handling


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


//FIXME:
// No, process won't ever be scheduled because it is sleeping, so psig never called on a sleeping process
// So, Kill must alter state of process and make it runnable, not cont.
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
  
  
  
  int i;
  cprintf("in psig\n");
  int stopsig[4] = {19, 20, 21, 22};
  int coresig[5] = {SIGQUIT, SIGABRT, SIGILL, SIGFPE, SIGSEGV};
  int termsig[7] = {SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2};
  int termcalled = 0, corecalled = 0, stopcalled = 0;
  
  
  if(!p)
  	return -1;
  
  
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
  			term(p, &corecalled);
  		p->sigpending = p->sigpending & (~(1 << coresig[i]));
  	}
  }
  
  
  
  
  
  //SIGSTOP can't be masked, ignored or handled
  if((p->sigpending & (1 << 19)) == (1 << 19))
  {	
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
  
  
  //Now if signals are userdefined.
	if(p->userdefed == 0){
		cprintf("Out of psig, if no userdefed\n");
		return 1;
	}
	else{
		//cprintf("About to go to user defined handler\n");
		for(i = 1; i < 32; i++){
			if((p->userdefed & (1 << i)) == (1 << i))
				break;		
			//i is the signal number that is pending
		}
		//cprintf("Found i is %d\n", i);

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

	//Now we push to current stack things so that iret takes us back to level3
	p->tf->esp = p->tf->esp - 76;
	//cprintf("Here \n");
	//cprintf(" in psig, printing: ss = %d, esp = %d, flag = %d, cs = %d, handler ptr = %d\n", p->tf->ss, p->tf->esp, p->tf->eflags,  p->tf->cs, p->allinfo[i].handler);
	
	p->sigpending = p->sigpending & (~(1 << i));
	//FIXME : put a synchronise here?
	jmptohandler(p->allinfo[i].handler, p->tf->cs, p->tf->eflags, p->tf->esp, p->tf->ss);

	}
	//won't reach here. But still.
	return -1;

 
}

// PAGEBREAK!
// After the user handler finishes execution,we must restore context of the user process that was,before it got interrupted
// So, kernel sigreturn calls restoreuser() which restores all registers and jumps back to user process

void
kernel_sigreturn(int signal)
{
	//cprintf("In kernelsigreturn\n");
	struct proc *p = myproc();
	if(!p)
		return;
		
	uint cs, ds, es, ss, fs, gs;
	uint eax, ebx, ecx, edx, esi, edi, esp, ebp, oesp, eip, eflag;
  	
  	esp = p->tf->esp;	//This is current userstack esp, which currently points to sinum argument of user handler
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
	
	//cprintf("In signal, handler is %d,\n", (int)handler);
	
	p->addr_sigreturn = NULL;
	if(handler == SIG_IGN){
		if(p->allinfo[signum].disposition == USERDEF)
			p->userdefed = p->userdefed & (~(1 << signum));
		
		p->allinfo[signum].disposition = IGN;
		//cprintf("In IGN\n");
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
	//cprintf("before signal return , fn ptr sigreturn is %d\n", p->addr_sigreturn);
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
			else if(sig == SIGCHLD && p->allinfo[SIGCHLD].disposition == DFL){
				release(&ptable.lock);
				if(minusone)
					continue;
				else{
					
					//cprintf("Sigchld disposition is %d\n", p->allinfo[sig].disposition);
					//cprintf("out of kill\n");
					//cprintf("sigpending is %d\n", p->sigpending);
					return 0;
					
				}
			}
			else if(sig == SIGCONT && p->allinfo[SIGCONT].disposition == DFL){
				if(p->state == SLEEPING){
					p->state = RUNNABLE;
					p->justwoken = 1;
				}
				release(&ptable.lock);
				if(minusone)
					continue;
				else
					return 0;
				
			}
	    	
			if(p->allinfo[sig].disposition != IGN && sig != 0){
				//cprintf("DISposition is %d\n", p->allinfo[sig].disposition);
				//cprintf("in setting : sigpending is %d\n", p->sigpending);
				p->sigpending = p->sigpending | (1 << sig);
			}

			release(&ptable.lock);
			if(minusone == 0)
				break;
    		
    	
		}
	}
	//cprintf("DISposition is %d\n", p->allinfo[sig].disposition);
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


