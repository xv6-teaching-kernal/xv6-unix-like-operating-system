3900 // Simple PIO-based (non-DMA) IDE driver code.
3901 
3902 #include "types.h"
3903 #include "defs.h"
3904 #include "param.h"
3905 #include "memlayout.h"
3906 #include "mmu.h"
3907 #include "proc.h"
3908 #include "x86.h"
3909 #include "traps.h"
3910 #include "spinlock.h"
3911 #include "buf.h"
3912 
3913 #define IDE_BSY       0x80
3914 #define IDE_DRDY      0x40
3915 #define IDE_DF        0x20
3916 #define IDE_ERR       0x01
3917 
3918 #define IDE_CMD_READ  0x20
3919 #define IDE_CMD_WRITE 0x30
3920 
3921 // idequeue points to the buf now being read/written to the disk.
3922 // idequeue->qnext points to the next buf to be processed.
3923 // You must hold idelock while manipulating queue.
3924 
3925 static struct spinlock idelock;
3926 static struct buf *idequeue;
3927 
3928 static int havedisk1;
3929 static void idestart(struct buf*);
3930 
3931 // Wait for IDE disk to become ready.
3932 static int
3933 idewait(int checkerr)
3934 {
3935   int r;
3936 
3937   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
3938     ;
3939   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
3940     return -1;
3941   return 0;
3942 }
3943 
3944 
3945 
3946 
3947 
3948 
3949 
3950 void
3951 ideinit(void)
3952 {
3953   int i;
3954 
3955   initlock(&idelock, "ide");
3956   picenable(IRQ_IDE);
3957   ioapicenable(IRQ_IDE, ncpu - 1);
3958   idewait(0);
3959 
3960   // Check if disk 1 is present
3961   outb(0x1f6, 0xe0 | (1<<4));
3962   for(i=0; i<1000; i++){
3963     if(inb(0x1f7) != 0){
3964       havedisk1 = 1;
3965       break;
3966     }
3967   }
3968 
3969   // Switch back to disk 0.
3970   outb(0x1f6, 0xe0 | (0<<4));
3971 }
3972 
3973 // Start the request for b.  Caller must hold idelock.
3974 static void
3975 idestart(struct buf *b)
3976 {
3977   if(b == 0)
3978     panic("idestart");
3979 
3980   idewait(0);
3981   outb(0x3f6, 0);  // generate interrupt
3982   outb(0x1f2, 1);  // number of sectors
3983   outb(0x1f3, b->sector & 0xff);
3984   outb(0x1f4, (b->sector >> 8) & 0xff);
3985   outb(0x1f5, (b->sector >> 16) & 0xff);
3986   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((b->sector>>24)&0x0f));
3987   if(b->flags & B_DIRTY){
3988     outb(0x1f7, IDE_CMD_WRITE);
3989     outsl(0x1f0, b->data, 512/4);
3990   } else {
3991     outb(0x1f7, IDE_CMD_READ);
3992   }
3993 }
3994 
3995 
3996 
3997 
3998 
3999 
4000 // Interrupt handler.
4001 void
4002 ideintr(void)
4003 {
4004   struct buf *b;
4005 
4006   // First queued buffer is the active request.
4007   acquire(&idelock);
4008   if((b = idequeue) == 0){
4009     release(&idelock);
4010     // cprintf("spurious IDE interrupt\n");
4011     return;
4012   }
4013   idequeue = b->qnext;
4014 
4015   // Read data if needed.
4016   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
4017     insl(0x1f0, b->data, 512/4);
4018 
4019   // Wake process waiting for this buf.
4020   b->flags |= B_VALID;
4021   b->flags &= ~B_DIRTY;
4022   wakeup(b);
4023 
4024   // Start disk on next buf in queue.
4025   if(idequeue != 0)
4026     idestart(idequeue);
4027 
4028   release(&idelock);
4029 }
4030 
4031 
4032 
4033 
4034 
4035 
4036 
4037 
4038 
4039 
4040 
4041 
4042 
4043 
4044 
4045 
4046 
4047 
4048 
4049 
4050 // Sync buf with disk.
4051 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
4052 // Else if B_VALID is not set, read buf from disk, set B_VALID.
4053 void
4054 iderw(struct buf *b)
4055 {
4056   struct buf **pp;
4057 
4058   if(!(b->flags & B_BUSY))
4059     panic("iderw: buf not busy");
4060   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
4061     panic("iderw: nothing to do");
4062   if(b->dev != 0 && !havedisk1)
4063     panic("iderw: ide disk 1 not present");
4064 
4065   acquire(&idelock);  //DOC:acquire-lock
4066 
4067   // Append b to idequeue.
4068   b->qnext = 0;
4069   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
4070     ;
4071   *pp = b;
4072 
4073   // Start disk if necessary.
4074   if(idequeue == b)
4075     idestart(b);
4076 
4077   // Wait for request to finish.
4078   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
4079     sleep(b, &idelock);
4080   }
4081 
4082   release(&idelock);
4083 }
4084 
4085 
4086 
4087 
4088 
4089 
4090 
4091 
4092 
4093 
4094 
4095 
4096 
4097 
4098 
4099 
