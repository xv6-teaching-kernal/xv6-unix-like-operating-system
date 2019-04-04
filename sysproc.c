#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int 
sys_getmysize(void)
{
  int addr;
  addr = proc->sz;
  return addr;
}

uint 
sys_getkernelstartaddr(void)
{
  return KERNBASE;
}

uint 
sys_getkernelendaddr(void)
{
  return KERNBASE+PHYSTOP;
}

uint 
sys_getkernelvariaddr(void)
{
  int a=1;
  uint send=(uint)&a;
  return send;
}

uint 
sys_getsystemcalladdr(void)
{
  return (uint)&sys_fork;
}

int sys_setpriority(void)
{
  int priority;
  // int curren_priority;
  // curren_priority = proc->priority;
  argint(0, &priority);
  proc->priority = priority;
  return proc->priority;
}

int sys_myps(void)
{

  struct proc *proctable;
  struct proc *proct_start;

  int proc_buffer_size ;
  char *proc_buffer;
  char *proc_buffer_start;
  if((argint(0, &proc_buffer_size)<0) || (argptr(1, &proc_buffer,proc_buffer_size)<0)){
    return -1;
  }
  proctable = get_proc_tabel();
  proct_start = proctable;
  
  proc_buffer_start = proc_buffer;

  while( (proc_buffer + proc_buffer_size > proc_buffer_start) && (proctable + sizeof(proc) > proct_start) && proct_start->pid != 0){
    *(uint*)proc_buffer_start = proct_start->sz;
    proc_buffer_start+=sizeof(uint);

    *(pde_t**)proc_buffer_start = proct_start->pgdir;
    proc_buffer_start+=sizeof(pde_t*);

    // *(char**)proc_buffer_start = proct_start->kstack;
    // proc_buffer_start+=sizeof(char*);

    // *(enum procstate*)proc_buffer_start = proct_start->state;
    // proc_buffer_start+=sizeof(enum procstate);

    *(volatile int *)proc_buffer_start = proct_start->pid;
    proc_buffer_start+=sizeof(volatile int);

    *(int *)proc_buffer_start = proct_start->killed;
    proc_buffer_start+=sizeof(int);

    *(void **)proc_buffer_start = proct_start->chan;
    proc_buffer_start+=sizeof(void*);

    // for(int i = 0;i<16;i++){
    //   *(char *)proc_buffer_start = (&proct_start->name + sizeof(char));
    //   proc_buffer_start+=sizeof(char);
    // }

    *(int *)proc_buffer_start = proct_start->priority;
    proc_buffer_start+=sizeof(int);

    proct_start++;
  }


  return 1;

}