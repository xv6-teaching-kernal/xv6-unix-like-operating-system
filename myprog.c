#include "types.h"
#include "stat.h"
#include "user.h"
int
main(void)
{
// chdir("home");
printf(1, "Size of a given inode: %d \n",getinodesize("home/ucsc"));
printf(1, "I’m running with ordinary priority\n");
printf(1, "setting priority to %d\n", setpriority(1));
sleep(2);
printf(1, "I’m running with priority 1 \n");
// printf(1, "setting priority to %d\n", setpriority(2));
sleep(2);
// below line should never work
printf(1, "I’m running with priority 2 \n");
exit();
}