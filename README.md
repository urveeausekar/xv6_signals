# Signals_xv6

Project Topic : Implementing signals in xv6.

This project will focus on signals for Inter Process Communication.

Features Added:
	
	The following system calls have been added :
		-int Kill(int pid, int signum)
		-int raise(int signum)
		-int sigsetmask(int mask)
		-uint siggetmask()
		-int signal(int signal, sighandler_t handler)
		
	There is another systemcall implemented , i.e void sigreturn(), 
	but this is a part of the signal trampoline, i.e the code used 
	to jump back into the kernel after the user defined signal handler 
	has run, and to restore original user context. Therefore, the 
	user should never actually call the function sigreturn.


References
	1) xv6 Book revision 11
	2) Uresh Vahalia : Unix Internals
	3) http://man7.org/linux/man-pages/man7/signal.7.html
	4) manual pages for signal(3), raise, kill(2), etc.
	5) http://courses.cms.caltech.edu/cs124/lectures/ (especially lecture 15)
	
	
How to build, run and test the project using qemu:

Requirements: qemu, make

In the folder which contains all the source files, run the following commands:
	1) make
	2) make qemu
	
	This will cause xv6 to boot up and start running. The shell will appear.
	In the shell, type:
	1) signaltest
	
	This will cause the testing code for signals to run.
	



----------------------------------------------------------------------------------
The Original README
----------------------------------------------------------------------------------

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern x86-based multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.828/, which
provides pointers to on-line resources for v6.

xv6 borrows code from the following sources:
    JOS (asm.h, elf.h, mmu.h, bootasm.S, ide.c, console.c, and others)
    Plan 9 (entryother.S, mp.h, mp.c, lapic.c)
    FreeBSD (ioapic.c)
    NetBSD (console.c)

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by Silas
Boyd-Wickizer, Anton Burtsev, Cody Cutler, Mike CAT, Tej Chajed, eyalz800,
Nelson Elhage, Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, Peter
Froehlich, Yakir Goaron,Shivam Handa, Bryan Henry, Jim Huang, Alexander
Kapshuk, Anders Kaseorg, kehao95, Wolfgang Keller, Eddie Kohler, Austin
Liew, Imbar Marinescu, Yandong Mao, Matan Shabtay, Hitoshi Mitake, Carmi
Merimovich, Mark Morrissey, mtasm, Joel Nider, Greg Price, Ayan Shafqat,
Eldar Sehayek, Yongming Shen, Cam Tenny, tyfkda, Rafael Ubal, Warren
Toomey, Stephen Tu, Pablo Ventura, Xi Wang, Keiichi Watanabe, Nicolas
Wolovick, wxdao, Grant Wu, Jindong Zhang, Icenowy Zheng, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2018 Frans Kaashoek, Robert Morris, and Russ Cox.

ERROR REPORTS

We switched our focus to xv6 on RISC-V; see the mit-pdos/xv6-riscv.git
repository on github.com.

BUILDING AND RUNNING XV6

To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
"make". On non-x86 or non-ELF machines (like OS X, even on x86), you
will need to install a cross-compiler gcc suite capable of producing
x86 ELF binaries (see https://pdos.csail.mit.edu/6.828/).
Then run "make TOOLPREFIX=i386-jos-elf-". Now install the QEMU PC
simulator and run "make qemu".
