7900 // init: The initial user-level program
7901 
7902 #include "types.h"
7903 #include "stat.h"
7904 #include "user.h"
7905 #include "fcntl.h"
7906 
7907 char *argv[] = { "sh", 0 };
7908 
7909 int
7910 main(void)
7911 {
7912   int pid, wpid;
7913 
7914   if(open("console", O_RDWR) < 0){
7915     mknod("console", 1, 1);
7916     open("console", O_RDWR);
7917   }
7918   dup(0);  // stdout
7919   dup(0);  // stderr
7920 
7921   for(;;){
7922     printf(1, "init: starting sh\n");
7923     pid = fork();
7924     if(pid < 0){
7925       printf(1, "init: fork failed\n");
7926       exit();
7927     }
7928     if(pid == 0){
7929       exec("sh", argv);
7930       printf(1, "init: exec sh failed\n");
7931       exit();
7932     }
7933     while((wpid=wait()) >= 0 && wpid != pid)
7934       printf(1, "zombie!\n");
7935   }
7936 }
7937 
7938 
7939 
7940 
7941 
7942 
7943 
7944 
7945 
7946 
7947 
7948 
7949 
