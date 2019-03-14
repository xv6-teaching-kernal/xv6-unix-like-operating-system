3250 #include "types.h"
3251 #include "defs.h"
3252 #include "param.h"
3253 #include "memlayout.h"
3254 #include "mmu.h"
3255 #include "proc.h"
3256 #include "x86.h"
3257 #include "syscall.h"
3258 
3259 // User code makes a system call with INT T_SYSCALL.
3260 // System call number in %eax.
3261 // Arguments on the stack, from the user call to the C
3262 // library system call function. The saved user %esp points
3263 // to a saved program counter, and then the first argument.
3264 
3265 // Fetch the int at addr from the current process.
3266 int
3267 fetchint(uint addr, int *ip)
3268 {
3269   if(addr >= proc->sz || addr+4 > proc->sz)
3270     return -1;
3271   *ip = *(int*)(addr);
3272   return 0;
3273 }
3274 
3275 // Fetch the nul-terminated string at addr from the current process.
3276 // Doesn't actually copy the string - just sets *pp to point at it.
3277 // Returns length of string, not including nul.
3278 int
3279 fetchstr(uint addr, char **pp)
3280 {
3281   char *s, *ep;
3282 
3283   if(addr >= proc->sz)
3284     return -1;
3285   *pp = (char*)addr;
3286   ep = (char*)proc->sz;
3287   for(s = *pp; s < ep; s++)
3288     if(*s == 0)
3289       return s - *pp;
3290   return -1;
3291 }
3292 
3293 // Fetch the nth 32-bit system call argument.
3294 int
3295 argint(int n, int *ip)
3296 {
3297   return fetchint(proc->tf->esp + 4 + 4*n, ip);
3298 }
3299 
3300 // Fetch the nth word-sized system call argument as a pointer
3301 // to a block of memory of size n bytes.  Check that the pointer
3302 // lies within the process address space.
3303 int
3304 argptr(int n, char **pp, int size)
3305 {
3306   int i;
3307 
3308   if(argint(n, &i) < 0)
3309     return -1;
3310   if((uint)i >= proc->sz || (uint)i+size > proc->sz)
3311     return -1;
3312   *pp = (char*)i;
3313   return 0;
3314 }
3315 
3316 // Fetch the nth word-sized system call argument as a string pointer.
3317 // Check that the pointer is valid and the string is nul-terminated.
3318 // (There is no shared writable memory, so the string can't change
3319 // between this check and being used by the kernel.)
3320 int
3321 argstr(int n, char **pp)
3322 {
3323   int addr;
3324   if(argint(n, &addr) < 0)
3325     return -1;
3326   return fetchstr(addr, pp);
3327 }
3328 
3329 extern int sys_chdir(void);
3330 extern int sys_close(void);
3331 extern int sys_dup(void);
3332 extern int sys_exec(void);
3333 extern int sys_exit(void);
3334 extern int sys_fork(void);
3335 extern int sys_fstat(void);
3336 extern int sys_getpid(void);
3337 extern int sys_kill(void);
3338 extern int sys_link(void);
3339 extern int sys_mkdir(void);
3340 extern int sys_mknod(void);
3341 extern int sys_open(void);
3342 extern int sys_pipe(void);
3343 extern int sys_read(void);
3344 extern int sys_sbrk(void);
3345 extern int sys_sleep(void);
3346 extern int sys_unlink(void);
3347 extern int sys_wait(void);
3348 extern int sys_write(void);
3349 extern int sys_uptime(void);
3350 extern int sys_getmysize(void);
3351 extern int sys_getkernelstartaddr(void);
3352 extern int sys_getkernelendaddr(void);
3353 extern int sys_getkernelvariaddr(void);
3354 extern int sys_getsystemcalladdr(void);
3355 extern int sys_setpriority(void);
3356 
3357 static int (*syscalls[])(void) = {
3358 [SYS_fork]    sys_fork,
3359 [SYS_exit]    sys_exit,
3360 [SYS_wait]    sys_wait,
3361 [SYS_pipe]    sys_pipe,
3362 [SYS_read]    sys_read,
3363 [SYS_kill]    sys_kill,
3364 [SYS_exec]    sys_exec,
3365 [SYS_fstat]   sys_fstat,
3366 [SYS_chdir]   sys_chdir,
3367 [SYS_dup]     sys_dup,
3368 [SYS_getpid]  sys_getpid,
3369 [SYS_sbrk]    sys_sbrk,
3370 [SYS_sleep]   sys_sleep,
3371 [SYS_uptime]  sys_uptime,
3372 [SYS_open]    sys_open,
3373 [SYS_write]   sys_write,
3374 [SYS_mknod]   sys_mknod,
3375 [SYS_unlink]  sys_unlink,
3376 [SYS_link]    sys_link,
3377 [SYS_mkdir]   sys_mkdir,
3378 [SYS_close]   sys_close,
3379 [SYS_getmysize] sys_getmysize,
3380 [SYS_getkernelstartaddr] sys_getkernelstartaddr,
3381 [SYS_getkernelendaddr] sys_getkernelendaddr,
3382 [SYS_getkernelvariaddr] sys_getkernelvariaddr,
3383 [SYS_getsystemcalladdr] sys_getsystemcalladdr,
3384 [SYS_setpriority] sys_setpriority,
3385 };
3386 
3387 
3388 
3389 
3390 
3391 
3392 
3393 
3394 
3395 
3396 
3397 
3398 
3399 
3400 void
3401 syscall(void)
3402 {
3403   int num;
3404 
3405   num = proc->tf->eax;
3406   if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
3407     proc->tf->eax = syscalls[num]();
3408   } else {
3409     cprintf("%d %s: unknown sys call %d\n",
3410             proc->pid, proc->name, num);
3411     proc->tf->eax = -1;
3412   }
3413 }
3414 
3415 
3416 
3417 
3418 
3419 
3420 
3421 
3422 
3423 
3424 
3425 
3426 
3427 
3428 
3429 
3430 
3431 
3432 
3433 
3434 
3435 
3436 
3437 
3438 
3439 
3440 
3441 
3442 
3443 
3444 
3445 
3446 
3447 
3448 
3449 
