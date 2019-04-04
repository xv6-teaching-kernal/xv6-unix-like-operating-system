#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
char *argv[] = { "sh", 0 };
int
main(void)
{
    int pid;
    printf(1, "Background service started... \n");
    pid = fork();
    if(pid == 0){
        exec("sh", argv);
    }
    while(1){
        sleep(5);
        printf(1, ".");
    }
}