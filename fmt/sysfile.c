5500 //
5501 // File-system system calls.
5502 // Mostly argument checking, since we don't trust
5503 // user code, and calls into file.c and fs.c.
5504 //
5505 
5506 #include "types.h"
5507 #include "defs.h"
5508 #include "param.h"
5509 #include "stat.h"
5510 #include "mmu.h"
5511 #include "proc.h"
5512 #include "fs.h"
5513 #include "file.h"
5514 #include "fcntl.h"
5515 
5516 // Fetch the nth word-sized system call argument as a file descriptor
5517 // and return both the descriptor and the corresponding struct file.
5518 static int
5519 argfd(int n, int *pfd, struct file **pf)
5520 {
5521   int fd;
5522   struct file *f;
5523 
5524   if(argint(n, &fd) < 0)
5525     return -1;
5526   if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
5527     return -1;
5528   if(pfd)
5529     *pfd = fd;
5530   if(pf)
5531     *pf = f;
5532   return 0;
5533 }
5534 
5535 // Allocate a file descriptor for the given file.
5536 // Takes over file reference from caller on success.
5537 static int
5538 fdalloc(struct file *f)
5539 {
5540   int fd;
5541 
5542   for(fd = 0; fd < NOFILE; fd++){
5543     if(proc->ofile[fd] == 0){
5544       proc->ofile[fd] = f;
5545       return fd;
5546     }
5547   }
5548   return -1;
5549 }
5550 int
5551 sys_dup(void)
5552 {
5553   struct file *f;
5554   int fd;
5555 
5556   if(argfd(0, 0, &f) < 0)
5557     return -1;
5558   if((fd=fdalloc(f)) < 0)
5559     return -1;
5560   filedup(f);
5561   return fd;
5562 }
5563 
5564 int
5565 sys_read(void)
5566 {
5567   struct file *f;
5568   int n;
5569   char *p;
5570 
5571   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
5572     return -1;
5573   return fileread(f, p, n);
5574 }
5575 
5576 int
5577 sys_write(void)
5578 {
5579   struct file *f;
5580   int n;
5581   char *p;
5582 
5583   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
5584     return -1;
5585   return filewrite(f, p, n);
5586 }
5587 
5588 int
5589 sys_close(void)
5590 {
5591   int fd;
5592   struct file *f;
5593 
5594   if(argfd(0, &fd, &f) < 0)
5595     return -1;
5596   proc->ofile[fd] = 0;
5597   fileclose(f);
5598   return 0;
5599 }
5600 int
5601 sys_fstat(void)
5602 {
5603   struct file *f;
5604   struct stat *st;
5605 
5606   if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
5607     return -1;
5608   return filestat(f, st);
5609 }
5610 
5611 // Create the path new as a link to the same inode as old.
5612 int
5613 sys_link(void)
5614 {
5615   char name[DIRSIZ], *new, *old;
5616   struct inode *dp, *ip;
5617 
5618   if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
5619     return -1;
5620   if((ip = namei(old)) == 0)
5621     return -1;
5622 
5623   begin_trans();
5624 
5625   ilock(ip);
5626   if(ip->type == T_DIR){
5627     iunlockput(ip);
5628     commit_trans();
5629     return -1;
5630   }
5631 
5632   ip->nlink++;
5633   iupdate(ip);
5634   iunlock(ip);
5635 
5636   if((dp = nameiparent(new, name)) == 0)
5637     goto bad;
5638   ilock(dp);
5639   if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
5640     iunlockput(dp);
5641     goto bad;
5642   }
5643   iunlockput(dp);
5644   iput(ip);
5645 
5646   commit_trans();
5647 
5648   return 0;
5649 
5650 bad:
5651   ilock(ip);
5652   ip->nlink--;
5653   iupdate(ip);
5654   iunlockput(ip);
5655   commit_trans();
5656   return -1;
5657 }
5658 
5659 // Is the directory dp empty except for "." and ".." ?
5660 static int
5661 isdirempty(struct inode *dp)
5662 {
5663   int off;
5664   struct dirent de;
5665 
5666   for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
5667     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5668       panic("isdirempty: readi");
5669     if(de.inum != 0)
5670       return 0;
5671   }
5672   return 1;
5673 }
5674 
5675 
5676 
5677 
5678 
5679 
5680 
5681 
5682 
5683 
5684 
5685 
5686 
5687 
5688 
5689 
5690 
5691 
5692 
5693 
5694 
5695 
5696 
5697 
5698 
5699 
5700 int
5701 sys_unlink(void)
5702 {
5703   struct inode *ip, *dp;
5704   struct dirent de;
5705   char name[DIRSIZ], *path;
5706   uint off;
5707 
5708   if(argstr(0, &path) < 0)
5709     return -1;
5710   if((dp = nameiparent(path, name)) == 0)
5711     return -1;
5712 
5713   begin_trans();
5714 
5715   ilock(dp);
5716 
5717   // Cannot unlink "." or "..".
5718   if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
5719     goto bad;
5720 
5721   if((ip = dirlookup(dp, name, &off)) == 0)
5722     goto bad;
5723   ilock(ip);
5724 
5725   if(ip->nlink < 1)
5726     panic("unlink: nlink < 1");
5727   if(ip->type == T_DIR && !isdirempty(ip)){
5728     iunlockput(ip);
5729     goto bad;
5730   }
5731 
5732   memset(&de, 0, sizeof(de));
5733   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5734     panic("unlink: writei");
5735   if(ip->type == T_DIR){
5736     dp->nlink--;
5737     iupdate(dp);
5738   }
5739   iunlockput(dp);
5740 
5741   ip->nlink--;
5742   iupdate(ip);
5743   iunlockput(ip);
5744 
5745   commit_trans();
5746 
5747   return 0;
5748 
5749 
5750 bad:
5751   iunlockput(dp);
5752   commit_trans();
5753   return -1;
5754 }
5755 
5756 static struct inode*
5757 create(char *path, short type, short major, short minor)
5758 {
5759   uint off;
5760   struct inode *ip, *dp;
5761   char name[DIRSIZ];
5762 
5763   if((dp = nameiparent(path, name)) == 0)
5764     return 0;
5765   ilock(dp);
5766 
5767   if((ip = dirlookup(dp, name, &off)) != 0){
5768     iunlockput(dp);
5769     ilock(ip);
5770     if(type == T_FILE && ip->type == T_FILE)
5771       return ip;
5772     iunlockput(ip);
5773     return 0;
5774   }
5775 
5776   if((ip = ialloc(dp->dev, type)) == 0)
5777     panic("create: ialloc");
5778 
5779   ilock(ip);
5780   ip->major = major;
5781   ip->minor = minor;
5782   ip->nlink = 1;
5783   iupdate(ip);
5784 
5785   if(type == T_DIR){  // Create . and .. entries.
5786     dp->nlink++;  // for ".."
5787     iupdate(dp);
5788     // No ip->nlink++ for ".": avoid cyclic ref count.
5789     if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
5790       panic("create dots");
5791   }
5792 
5793   if(dirlink(dp, name, ip->inum) < 0)
5794     panic("create: dirlink");
5795 
5796   iunlockput(dp);
5797 
5798   return ip;
5799 }
5800 int
5801 sys_open(void)
5802 {
5803   char *path;
5804   int fd, omode;
5805   struct file *f;
5806   struct inode *ip;
5807 
5808   if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
5809     return -1;
5810   if(omode & O_CREATE){
5811     begin_trans();
5812     ip = create(path, T_FILE, 0, 0);
5813     commit_trans();
5814     if(ip == 0)
5815       return -1;
5816   } else {
5817     if((ip = namei(path)) == 0)
5818       return -1;
5819     ilock(ip);
5820     if(ip->type == T_DIR && omode != O_RDONLY){
5821       iunlockput(ip);
5822       return -1;
5823     }
5824   }
5825 
5826   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
5827     if(f)
5828       fileclose(f);
5829     iunlockput(ip);
5830     return -1;
5831   }
5832   iunlock(ip);
5833 
5834   f->type = FD_INODE;
5835   f->ip = ip;
5836   f->off = 0;
5837   f->readable = !(omode & O_WRONLY);
5838   f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
5839   return fd;
5840 }
5841 
5842 
5843 
5844 
5845 
5846 
5847 
5848 
5849 
5850 int
5851 sys_mkdir(void)
5852 {
5853   char *path;
5854   struct inode *ip;
5855 
5856   begin_trans();
5857   if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
5858     commit_trans();
5859     return -1;
5860   }
5861   iunlockput(ip);
5862   commit_trans();
5863   return 0;
5864 }
5865 
5866 int
5867 sys_mknod(void)
5868 {
5869   struct inode *ip;
5870   char *path;
5871   int len;
5872   int major, minor;
5873 
5874   begin_trans();
5875   if((len=argstr(0, &path)) < 0 ||
5876      argint(1, &major) < 0 ||
5877      argint(2, &minor) < 0 ||
5878      (ip = create(path, T_DEV, major, minor)) == 0){
5879     commit_trans();
5880     return -1;
5881   }
5882   iunlockput(ip);
5883   commit_trans();
5884   return 0;
5885 }
5886 
5887 
5888 
5889 
5890 
5891 
5892 
5893 
5894 
5895 
5896 
5897 
5898 
5899 
5900 int
5901 sys_chdir(void)
5902 {
5903   char *path;
5904   struct inode *ip;
5905 
5906   if(argstr(0, &path) < 0 || (ip = namei(path)) == 0)
5907     return -1;
5908   ilock(ip);
5909   if(ip->type != T_DIR){
5910     iunlockput(ip);
5911     return -1;
5912   }
5913   iunlock(ip);
5914   iput(proc->cwd);
5915   proc->cwd = ip;
5916   return 0;
5917 }
5918 
5919 int
5920 sys_exec(void)
5921 {
5922   char *path, *argv[MAXARG];
5923   int i;
5924   uint uargv, uarg;
5925 
5926   if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
5927     return -1;
5928   }
5929   memset(argv, 0, sizeof(argv));
5930   for(i=0;; i++){
5931     if(i >= NELEM(argv))
5932       return -1;
5933     if(fetchint(uargv+4*i, (int*)&uarg) < 0)
5934       return -1;
5935     if(uarg == 0){
5936       argv[i] = 0;
5937       break;
5938     }
5939     if(fetchstr(uarg, &argv[i]) < 0)
5940       return -1;
5941   }
5942   return exec(path, argv);
5943 }
5944 
5945 
5946 
5947 
5948 
5949 
5950 int
5951 sys_pipe(void)
5952 {
5953   int *fd;
5954   struct file *rf, *wf;
5955   int fd0, fd1;
5956 
5957   if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
5958     return -1;
5959   if(pipealloc(&rf, &wf) < 0)
5960     return -1;
5961   fd0 = -1;
5962   if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
5963     if(fd0 >= 0)
5964       proc->ofile[fd0] = 0;
5965     fileclose(rf);
5966     fileclose(wf);
5967     return -1;
5968   }
5969   fd[0] = fd0;
5970   fd[1] = fd1;
5971   return 0;
5972 }
5973 
5974 
5975 
5976 
5977 
5978 
5979 
5980 
5981 
5982 
5983 
5984 
5985 
5986 
5987 
5988 
5989 
5990 
5991 
5992 
5993 
5994 
5995 
5996 
5997 
5998 
5999 
