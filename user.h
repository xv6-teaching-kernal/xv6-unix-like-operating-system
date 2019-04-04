// #include "ps.h"
struct stat;

#define MAX_PS_PROCS 10

struct procps {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
//   char *kstack;                // Bottom of kernel stack for this process
//   enum procstate state;        // Process state
  volatile int pid;            // Process ID
//   struct proc *parent;         // Parent process
//   struct trapframe *tf;        // Trap frame for current syscall
//   struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
//   struct file *ofile[NOFILE];  // Open files
//   struct inode *cwd;           // Current directory
//   char name[16];               // Process name (debugging)
  int priority;                // Priority of the process
};

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int getmysize(void);
uint getkernelstartaddr(void);
uint getkernelendaddr(void);
uint getkernelvariaddr(void);
uint getsystemcalladdr(void);
int setpriority(int priority);
int getinodesize(char* path);
int myps(unsigned long, struct procps[]);


// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
