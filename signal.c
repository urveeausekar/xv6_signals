#include"proc.h"

int issig(struct proc *p)
{
  return p->sigpending; 
}



int ign(){}

int core(){}

int term(){}

int cont(){}

int stop(){}




int psig(struct proc *p)
{
  if(p->sigpending & 1 << 8 == 1 << 8)
    kill(p->pid);
  else if(p->sigpending & 1 << 18 == 1 << 18)
    //nope write functions first
}
