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
	printf(1, "In user defined handler.\n");
}

void sigsegv_handler(int signum){
	printf(1, "Current process received an artificial sigsegv due to a raise call\n");
}

void sigint_handler(int signum){
	printf(1, "Signal delivered, currently running user-defined handler\n");
}


void sigusr1_handler(int signum){
	printf(1, "Currently in user defined handler. About to call systemcall sleep(2)\n");
	sleep(2);
	printf(1, "Back in user defined handler. Context restored properly\n");
}

int Killtest(){
	int pid = getpid();
	int var = 0;
	
	printf(1, "\nTesting systemcall Kill()\n\n");
	printf(1, "Test 1 : Testing a signal that is ignored by default - SIGCHLD\n");
	
	Kill(pid, SIGCHLD);
	printf(1, "Passed\n");
	printf(1, "Test 2 : Testing a signal that causes a process to terminate by  default\n");
	
	pid = fork();
	if(pid == 0){
		raise(SIGINT);
		return 1;
	}
	else{
		wait();
		printf(1, "Passed\n");
	}
	
	printf(1, "Test 3 : Testing a signal that causes a process to dump core by default\n");
	pid = fork();
	if(pid == 0){
		raise(SIGQUIT);
		return 1;
	}
	else{
		wait();
		printf(1, "Passed\n");
	}
	
	printf(1, "Test 4 : Testing SIGCONT and a signal which causes a process to stop\n");
	pid = fork();
	if(pid == 0){
		int i = 0;
		while(i < 50)
			i++;
		exit();
		
	}
	else{
		
		Kill(pid, SIGTSTP);
		printf(1, "Stopped process with pid %d\n", pid);
		printf(1, "Resuming process with pid %d\n", pid);
		Kill(pid, SIGCONT);
		Kill(pid, SIGKILL);
		wait();
		printf(1, "Passed\n");
	}
	
	printf(1, "Test 5 : Trying to kill init\n");
	Kill(1, SIGKILL);
	printf(1, "Passed\n");
	
	printf(1, "Test 6 : Mask a signal, and see if it is delivered\n");
	pid = fork();
	if(pid == 0){
	
		int mask = siggetmask();
		sigsetmask(mask | (1 << SIGINT));
		signal(SIGINT, sigint_handler);
		Kill(getpid(), SIGINT);
		printf(1, "Passed\n");
		printf(1, "Test 7 : Unmask previously masked signal, and see if it is delivered\n");
		sigsetmask(mask);
		printf(1, "Passed\n");
		exit();
	}
	else{
		wait();
		
	}
	
	printf(1, "Test 8 : Giving Kill invalid values\n");
	if(Kill(getpid(), 0) == -1)
		printf(1, "Passed\n");
	
	return 0;
}



int raisetest(){
	printf(1, "\nTesting raise\n\n");
	signal(SIGSEGV, sigsegv_handler);
	raise(SIGSEGV);
	signal(SIGSEGV, SIG_DFL);
	printf(1, "Passed\n");
	return 0;
	
}

int testsignal_syscall(int pid){
	printf(1, "\nTesting systemcall signal\n\n");
	int pidtwo;
	failedprivate = 0;
	pidtwo = fork();
	if(pidtwo == 0){
		printf(1, "Test 1 : Checking SIG_IGN\n");
		signal(SIGHUP, SIG_IGN);
		Kill(getpid(), SIGHUP);
		printf(1, "Passed\n");
		printf(1, "Test 2 : Checking user defined handler\n");
		//printf(1, " In signaltest, Address of function sigreturn is %d\n", sigreturn);
		//printf(1, "Address of handler is %d\n", sighuphandler);
		
		signal(SIGHUP, sighuphandler);
		Kill(getpid(), SIGHUP);
		printf(1, "Passed\n");
		printf(1, "Test 3 : Checking SIG_DFL\n");
		signal(SIGHUP, SIG_DFL);
		Kill(getpid(), SIGHUP);
		//psig should terminate process just after returning from kill. So child should never reach here
		return 0;
		
	}
	else{
		wait();
		printf(1, "Passed\n");
		
	}
	printf(1, "Test 4 : Trying to ignore SIGKILL\n");
	if(signal(SIGKILL, SIG_IGN) == SIG_ERR)
		printf(1, "Passed\n");
		
	printf(1, "Test 5 : Trying to handle SIGKILL\n");
	if(signal(SIGKILL, sighuphandler) == SIG_ERR)
		printf(1, "Passed\n");
	
	
	printf(1, "Test 6 : Trying to ignore SIGSTOP\n");
	if(signal(SIGSTOP, SIG_IGN) == SIG_ERR)
		printf(1, "Passed\n");
		
	printf(1, "Test 7 : Trying to handle SIGSTOP\n");
	if(signal(SIGSTOP, sighuphandler) == SIG_ERR)
		printf(1, "Passed\n");
	
	printf(1, "Test 8 : User defined handler calling system calls\n");
	signal(SIGUSR1, sigusr1_handler);
	Kill(getpid(), SIGUSR1);
	printf(1, "Passed\n");
	signal(SIGUSR1, SIG_DFL);
		
	return 1;
	
}


int masktest(){
	uint mask;
	int fail = 0;
	mask = ((1 << SIGHUP) | (1 << SIGINT));		//block sighup and sigint (for example)
	printf(1, "\ntesting sigsetmask and siggetmask\n\n");
	printf(1, "Test1 : checking normal functionality\n");
	sigsetmask(mask);
	if(siggetmask() == mask)
		printf(1, "Passed test1\n");
	else{
		printf(1, "Failed test1\n");
		fail++;
	}
	
	printf(1, "Test 2 : non maskable signal - SIGKILL\n");
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
	
	printf(1, "Test 3 : non maskable signal - SIGSTOP\n");
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
	printf(1, "\ntesting SIGKILL\n\n");
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
	printf(1, "\ntesting SIGTERM\n\n");
	pid = fork();
	if(pid == 0){
		while(2)
			;
		exit();
	}
	else{
		sleep(1);
		Kill(pid, SIGTERM);
		wait();
		printf(1, "Sigterm delivered, test passed.\n");
		return 0;
	}
	return -1;
}


int main(){
	
	int ret, pid = getpid();
	printf(1, "RUNNING TESTSUITE FOR SIGNALS\n\n");
	
	if(testsigkill1() == -1){
		printf(1, "testsigkill1 failed\n");
		failed++;
	}
	
	if(testsigterm() == -1){
		printf(1, "testsigterm failed\n");
		failed++;
	}
	
	
	ret = masktest();
	if(ret != 0){
		printf(1, "masktest failed\n");
		failed = failed + ret;
	}
		
	ret = Killtest();
	if(ret != 0){
		printf(1, "Killtest failed\n");
		failed = failed + ret;
	}
	
	if(!testsignal_syscall(pid)){
		printf(1, "syscall signal() test failed\n");
		failed++;
		exit();
	}
	
	if(raisetest()){
		printf(1, "syscall raise test failed\n");
		failed++;
	}	
	
	if(failed == 0)
		printf(1, "\nAll tests passed\n");
	else
		printf(1, "Out of 2, %d tests failed\n", failed);
	
	exit();
}
