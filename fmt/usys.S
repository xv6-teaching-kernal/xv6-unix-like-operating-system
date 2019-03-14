7850 #include "syscall.h"
7851 #include "traps.h"
7852 
7853 #define SYSCALL(name) \
7854   .globl name; \
7855   name: \
7856     movl $SYS_ ## name, %eax; \
7857     int $T_SYSCALL; \
7858     ret
7859 
7860 SYSCALL(fork)
7861 SYSCALL(exit)
7862 SYSCALL(wait)
7863 SYSCALL(pipe)
7864 SYSCALL(read)
7865 SYSCALL(write)
7866 SYSCALL(close)
7867 SYSCALL(kill)
7868 SYSCALL(exec)
7869 SYSCALL(open)
7870 SYSCALL(mknod)
7871 SYSCALL(unlink)
7872 SYSCALL(fstat)
7873 SYSCALL(link)
7874 SYSCALL(mkdir)
7875 SYSCALL(chdir)
7876 SYSCALL(dup)
7877 SYSCALL(getpid)
7878 SYSCALL(sbrk)
7879 SYSCALL(sleep)
7880 SYSCALL(uptime)
7881 SYSCALL(getmysize)
7882 SYSCALL(getkernelstartaddr)
7883 SYSCALL(getkernelendaddr)
7884 SYSCALL(getkernelvariaddr)
7885 SYSCALL(getsystemcalladdr)
7886 SYSCALL(setpriority)
7887 
7888 
7889 
7890 
7891 
7892 
7893 
7894 
7895 
7896 
7897 
7898 
7899 
