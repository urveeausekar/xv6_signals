//These are the signals defined in the original POSIX.1-1990 standard

#define SIGHUP        1       //Term    Hangup detected on controlling terminal or death of controlling process
#define SIGINT        2       //Term    Interrupt from keyboard
#define SIGQUIT       3       //Core    Quit from keyboard
#define SIGILL        4       //Core    Illegal Instruction
#define SIGABRT       6      // Core    Abort signal from abort(3)
#define SIGFPE        8      // Core    Floating-point exception
#define SIGKILL       9      // Term    Kill signal
#define SIGSEGV      11      // Core    Invalid memory reference
#define SIGPIPE      13       //Term    Broken pipe: write to pipe with no readers; see pipe(7)
#define SIGALRM      14       //Term    Timer signal from alarm(2)
#define SIGTERM      15       //Term    Termination signal
#define SIGUSR1      10   //Term    User-defined signal 1
#define SIGUSR2      12    //Term    User-defined signal 2
#define SIGCHLD      17    //Ign     Child stopped or terminated
#define SIGCONT      18    //Cont    Continue if stopped
#define SIGSTOP      19    //Stop    Stop process
#define SIGTSTP      20    //Stop    Stop typed at terminal
#define SIGTTIN      21    //Stop    Terminal input for background process
#define SIGTTOU      22    //Stop    Terminal output for background process

#define SIG_DFL (void (*)())0
#define SIG_IGN (void (*)())1
#define SIG_USERDEF (void (*)())2
#define SIG_ERR (void (*)())-1

#define DFL 0
#define IGN 1
#define USERDEF 2

#define TERM 1
#define IGNORE 2
#define CORE 3
#define CONT 4
#define STOP 5



#define NUMSIG 32	//maximum number of signals supported
#define CURRNUM 22	//maximum number of signals currently supported

typedef int pid_t;

typedef void (*sighandler_t)(int);
typedef void (*addr_sigret)(void);



typedef struct handlerinfo {
	//int signum
	int disposition;	//either DFL, IGN, or USERDEF
	sighandler_t handler;	//address of the user defined signal handler
}handlerinfo;


