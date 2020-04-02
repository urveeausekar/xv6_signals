# To restore user context and jump to where the user process was when it was interrupted.
#
#	int restoreuser(int ss, int esp);
#
# We have saved the context of the process in the user stack. 
# Right now we are running on the kernel stack. But the user esp and ss has been passed to us an an argument.
# So switch stacks to user stack. 
# Now we pop the registers off the user's stack and ret.

.globl restoreuser
restoreuser:
	mov 4(%esp), %ss
	mov 8(%esp), %esp
	
	#stack switched
	
	addl $4, %esp	#argument of signum still on stack
	popl %edi
	popl %esi
	popl %ebp
	addl $4, %esp 	#skip the pushed esp
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax
	popl %gs
	popl %fs
	popl %es
	popl %ds
	addl $4, %esp	#skip ip for now, restore it last.
	popl %cs
	popl %eflag
	movl -12(%esp), eip
	#done.
	
