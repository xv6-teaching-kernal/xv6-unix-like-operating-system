#include "types.h"
#include "stat.h"
#include "user.h"
// #include "ps.h"

int main(void) {
    struct procps ps_array[MAX_PS_PROCS];
    struct procps *start;
    myps((unsigned long)MAX_PS_PROCS * sizeof(struct procps),ps_array);
    start = &ps_array[0];
    
        printf (1,"SZ PGDIR PID KILLED CHAN PRIORITY \n");
    while(start != &ps_array[MAX_PS_PROCS-1] && start->pid != 0){
        printf (1,"%d ", start->sz);
        printf (1,"%p ", start->pgdir);
        // printf (1,"%d ", start->state);
        // printf (1,start->kstack);
        printf (1,"%d ", start->pid);
        printf (1,"%d ", start->killed);
        printf (1,"%d ", start->chan);
        // printf(1,start->name);
        printf(1,"%d \n",start->priority);
        start++;
    }
    exit();    
}