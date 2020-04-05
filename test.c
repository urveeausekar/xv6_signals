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
	printf(1, "Size of integer is : %d\n", sizeof(int));
	printf(1, "Size of function pointer is : %d\n", sizeof(sighandler_t));
	exit();
}
