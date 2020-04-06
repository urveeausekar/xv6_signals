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

static int failed = 0;	//keeps track of number of tests failed yet.
int failedprivate;

void sighuphandler(int signum){
	printf(1, "In user defined sighup handler.\n");
}


int testsignal_syscall(int pid){
	printf(1, "Testing systemcall signal\n");
	int pidtwo;
	failedprivate = 0;
	pidtwo = fork();
	if(pidtwo == 0){
		printf(1, "Test1 : Checking SIG_IGN\n");
		signal(SIGHUP, SIG_IGN);
		Kill(getpid(), SIGHUP);
		printf(1, "Passed\n");
		printf(1, "Test2 : Checking user defined handler\n");
		signal(SIGHUP, sighuphandler);
		Kill(getpid(), SIGHUP);
		printf(1, "Passed\n");
		printf(1, "Test3 : Checking SIG_DFL\n");
		signal(SIGHUP, SIG_DFL);
		Kill(getpid(), SIGHUP);
		
	}
	else{
		wait();
		return 1;
	}
	//psig should terminate process just after returning from kill. So child should never reach here

	return 0;
}


int masktest(){
	uint mask;
	int fail = 0;
	mask = ((1 << SIGHUP) | (1 << SIGINT));		//block sighup and sigint (for example)
	printf(1, "testing sigsetmask and siggetmask\n");
	printf(1, "Test1 : checking normal functionality\n");
	sigsetmask(mask);
	if(siggetmask() == mask)
		printf(1, "Passed test1\n");
	else{
		printf(1, "Failed test1\n");
		fail++;
	}
	
	printf(1, "Test2 : non maskable signal - SIGKILL\n");
	mask = 0;
	mask = (1 << SIGKILL);
	sigsetmask(mask);
	if(siggetmask() == mask){
		printf(1, "Failed test2\n");
		fail++;
	}
	else{
		printf(1, "Passed test2\n");
	
	}
	
	printf(1, "Test3 : non maskable signal - SIGSTOP\n");
	mask = 0;
	mask = (1 << SIGSTOP);
	sigsetmask(mask);
	if(siggetmask() == mask){
		printf(1, "Failed test3\n");
		fail++;
	}
	else{
		printf(1, "Passed test3\n");
	
	}
	
	sigsetmask(0);
	return fail;
		
	
}



int testsigkill1(){
	int pid;
	printf(1, "testing SIGKILL\n");
	pid = fork();
	if(pid == 0){
		//printf(1, "in child");
		while(1)
			;
		exit();
		
	}
	else{
		sleep(1);
		Kill(pid, SIGKILL);
		wait();
		printf(1, "Sigkill delivered, test passed.\n");
		return 0;
	}
	return -1;
}


int testsigterm(){
	int pid;
	printf(1, "testing SIGTERM\n");
	pid = fork();
	if(pid == 0){
		while(2000)
			printf(1, "in while\n");
		exit();
	}
	else{
		Kill(pid, SIGTERM);
		wait();
		printf(1, "Sigterm delivered, test passed.\n");
		return 0;
	}
	return -1;
}


int main(){
	
	int ret, pid;
	printf(1, "running testsuite for signals\n\n");
	
	/*if(testsigkill1() == -1){
		printf(1, "testsigkill1 failed\n");
		failed++;
	}
	
	/*if(testsigterm() == -1){
		printf(1, "testsigterm failed\n");
		failed++;
	}*/
	
	
	ret = masktest();
	if(ret != 0){
		printf(1, "masktest failed\n");
		failed = failed + ret;
	}
		
	printf(1, "Testing ignored signal - SIGCHLD\n");
	pid = getpid();
	Kill(pid, SIGCHLD);
	printf(1, "SIGCHLD ignored\n");
	
	if(!testsignal_syscall(pid)){
		printf(1, "syscall signal() test failed\n");
		failed++;
		exit();
	}
		
	
	if(failed == 0)
		printf(1, "All tests passed\n");
	else
		printf(1, "Out of 2, %d tests failed\n", failed);
	
	exit();
}
