# To jump back to the userspace and to the user defined signal handler and start using it 
#
#	void jmptohandler(sighandler_t handlerptr, uint cs, uint eflag, uint esp, uint ss);
#
# The arguments have already been pushed onto the kernel stack by the function call mechanism.
# The pushed arguments make the stack look like way the stack looks just after jumping into kernel
# after an int instruction from user space.
# In addition, the return address has also been pushed i.e the stack is like : 
#
#	----------
#	|   ss   |
#	----------
#	|  esp   |
#	----------
#	| eflag  |
#	----------
#	|   cs   |
#	----------
#	| fnptr  |
#	----------
#	|retaddr |
#  esp->----------


.globl jmptohandler
jmptohandler:
	addl $4, %esp;
	iret;
	
