#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"
#include "signal.h"



int main(){
	int pid = fork();
	int i = 0;
	if(pid == 0){
		sleep(1);
		while(i < 200){
			printf(1, "%d ", i);
			i++;
		}
		exit();
	}
	else{
		sleep(2);
		printf(1, "\nPausing process with pid %d\n", pid);
		Kill(pid, SIGTSTP);
		sleep(1);
		Kill(pid, SIGCONT);
		printf(1, "\nResuming process with pid %d\n", pid);
		wait();
	}
	exit();
}
