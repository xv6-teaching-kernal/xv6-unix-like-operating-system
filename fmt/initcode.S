7800 # Initial process execs /init.
7801 
7802 #include "syscall.h"
7803 #include "traps.h"
7804 
7805 
7806 # exec(init, argv)
7807 .globl start
7808 start:
7809   pushl $argv
7810   pushl $init
7811   pushl $0  // where caller pc would be
7812   movl $SYS_exec, %eax
7813   int $T_SYSCALL
7814 
7815 # for(;;) exit();
7816 exit:
7817   movl $SYS_exit, %eax
7818   int $T_SYSCALL
7819   jmp exit
7820 
7821 # char init[] = "/init\0";
7822 init:
7823   .string "/init\0"
7824 
7825 # char *argv[] = { init, 0 };
7826 .p2align 2
7827 argv:
7828   .long init
7829   .long 0
7830 
7831 
7832 
7833 
7834 
7835 
7836 
7837 
7838 
7839 
7840 
7841 
7842 
7843 
7844 
7845 
7846 
7847 
7848 
7849 
