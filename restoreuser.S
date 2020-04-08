# To restore user context and jump to where the user process was when it was interrupted.
#
#	int restoreuser(uint edi, uint esi, uint ebp, uint oesp, uint ebx, uint edx, uint ecx, uint eax, uint gs, uint fs, uint es, uint ds, uint eip, uint cs, uint eflag, uint esp, uint ss);
#
# Now, we have the context of the process, saved in the kernel stack, as they have been passed as arguments. 
# Right now we are running on the kernel stack.
# Below ss in the kernel stack, there is also the return address of kernel_sigreturn, which we just ignore.
# Now we pop the registers off the kernel stack and iret.

.globl restoreuser
restoreuser:
	addl $4, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	iret
	
