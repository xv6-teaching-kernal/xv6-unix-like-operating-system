3850 struct file {
3851   enum { FD_NONE, FD_PIPE, FD_INODE } type;
3852   int ref; // reference count
3853   char readable;
3854   char writable;
3855   struct pipe *pipe;
3856   struct inode *ip;
3857   uint off;
3858 };
3859 
3860 
3861 // in-memory copy of an inode
3862 struct inode {
3863   uint dev;           // Device number
3864   uint inum;          // Inode number
3865   int ref;            // Reference count
3866   int flags;          // I_BUSY, I_VALID
3867 
3868   short type;         // copy of disk inode
3869   short major;
3870   short minor;
3871   short nlink;
3872   uint size;
3873   uint addrs[NDIRECT+1];
3874 };
3875 #define I_BUSY 0x1
3876 #define I_VALID 0x2
3877 
3878 // table mapping major device number to
3879 // device functions
3880 struct devsw {
3881   int (*read)(struct inode*, char*, int);
3882   int (*write)(struct inode*, char*, int);
3883 };
3884 
3885 extern struct devsw devsw[];
3886 
3887 #define CONSOLE 1
3888 
3889 
3890 
3891 
3892 
3893 
3894 
3895 
3896 
3897 
3898 
3899 
