3450 #include "types.h"
3451 #include "x86.h"
3452 #include "defs.h"
3453 #include "param.h"
3454 #include "memlayout.h"
3455 #include "mmu.h"
3456 #include "proc.h"
3457 
3458 int
3459 sys_fork(void)
3460 {
3461   return fork();
3462 }
3463 
3464 int
3465 sys_exit(void)
3466 {
3467   exit();
3468   return 0;  // not reached
3469 }
3470 
3471 int
3472 sys_wait(void)
3473 {
3474   return wait();
3475 }
3476 
3477 int
3478 sys_kill(void)
3479 {
3480   int pid;
3481 
3482   if(argint(0, &pid) < 0)
3483     return -1;
3484   return kill(pid);
3485 }
3486 
3487 int
3488 sys_getpid(void)
3489 {
3490   return proc->pid;
3491 }
3492 
3493 
3494 
3495 
3496 
3497 
3498 
3499 
3500 int
3501 sys_sbrk(void)
3502 {
3503   int addr;
3504   int n;
3505 
3506   if(argint(0, &n) < 0)
3507     return -1;
3508   addr = proc->sz;
3509   if(growproc(n) < 0)
3510     return -1;
3511   return addr;
3512 }
3513 
3514 int
3515 sys_sleep(void)
3516 {
3517   int n;
3518   uint ticks0;
3519 
3520   if(argint(0, &n) < 0)
3521     return -1;
3522   acquire(&tickslock);
3523   ticks0 = ticks;
3524   while(ticks - ticks0 < n){
3525     if(proc->killed){
3526       release(&tickslock);
3527       return -1;
3528     }
3529     sleep(&ticks, &tickslock);
3530   }
3531   release(&tickslock);
3532   return 0;
3533 }
3534 
3535 // return how many clock tick interrupts have occurred
3536 // since start.
3537 int
3538 sys_uptime(void)
3539 {
3540   uint xticks;
3541 
3542   acquire(&tickslock);
3543   xticks = ticks;
3544   release(&tickslock);
3545   return xticks;
3546 }
3547 
3548 
3549 
3550 int
3551 sys_getmysize(void)
3552 {
3553   int addr;
3554   addr = proc->sz;
3555   return addr;
3556 }
3557 
3558 uint
3559 sys_getkernelstartaddr(void)
3560 {
3561   return KERNBASE;
3562 }
3563 
3564 uint
3565 sys_getkernelendaddr(void)
3566 {
3567   return KERNBASE+PHYSTOP;
3568 }
3569 
3570 uint
3571 sys_getkernelvariaddr(void)
3572 {
3573   int a=1;
3574   uint send=(uint)&a;
3575   return send;
3576 }
3577 
3578 uint
3579 sys_getsystemcalladdr(void)
3580 {
3581   return (uint)&sys_fork;
3582 }
3583 
3584 int sys_setpriority(void)
3585 {
3586   int priority;
3587   argint(0, &priority);
3588   return priority;
3589 }
3590 
3591 
3592 
3593 
3594 
3595 
3596 
3597 
3598 
3599 
