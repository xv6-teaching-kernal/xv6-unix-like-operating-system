4100 // Buffer cache.
4101 //
4102 // The buffer cache is a linked list of buf structures holding
4103 // cached copies of disk block contents.  Caching disk blocks
4104 // in memory reduces the number of disk reads and also provides
4105 // a synchronization point for disk blocks used by multiple processes.
4106 //
4107 // Interface:
4108 // * To get a buffer for a particular disk block, call bread.
4109 // * After changing buffer data, call bwrite to write it to disk.
4110 // * When done with the buffer, call brelse.
4111 // * Do not use the buffer after calling brelse.
4112 // * Only one process at a time can use a buffer,
4113 //     so do not keep them longer than necessary.
4114 //
4115 // The implementation uses three state flags internally:
4116 // * B_BUSY: the block has been returned from bread
4117 //     and has not been passed back to brelse.
4118 // * B_VALID: the buffer data has been read from the disk.
4119 // * B_DIRTY: the buffer data has been modified
4120 //     and needs to be written to disk.
4121 
4122 #include "types.h"
4123 #include "defs.h"
4124 #include "param.h"
4125 #include "spinlock.h"
4126 #include "buf.h"
4127 
4128 struct {
4129   struct spinlock lock;
4130   struct buf buf[NBUF];
4131 
4132   // Linked list of all buffers, through prev/next.
4133   // head.next is most recently used.
4134   struct buf head;
4135 } bcache;
4136 
4137 void
4138 binit(void)
4139 {
4140   struct buf *b;
4141 
4142   initlock(&bcache.lock, "bcache");
4143 
4144 
4145 
4146 
4147 
4148 
4149 
4150   // Create linked list of buffers
4151   bcache.head.prev = &bcache.head;
4152   bcache.head.next = &bcache.head;
4153   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
4154     b->next = bcache.head.next;
4155     b->prev = &bcache.head;
4156     b->dev = -1;
4157     bcache.head.next->prev = b;
4158     bcache.head.next = b;
4159   }
4160 }
4161 
4162 // Look through buffer cache for sector on device dev.
4163 // If not found, allocate fresh block.
4164 // In either case, return B_BUSY buffer.
4165 static struct buf*
4166 bget(uint dev, uint sector)
4167 {
4168   struct buf *b;
4169 
4170   acquire(&bcache.lock);
4171 
4172  loop:
4173   // Is the sector already cached?
4174   for(b = bcache.head.next; b != &bcache.head; b = b->next){
4175     if(b->dev == dev && b->sector == sector){
4176       if(!(b->flags & B_BUSY)){
4177         b->flags |= B_BUSY;
4178         release(&bcache.lock);
4179         return b;
4180       }
4181       sleep(b, &bcache.lock);
4182       goto loop;
4183     }
4184   }
4185 
4186   // Not cached; recycle some non-busy and clean buffer.
4187   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
4188     if((b->flags & B_BUSY) == 0 && (b->flags & B_DIRTY) == 0){
4189       b->dev = dev;
4190       b->sector = sector;
4191       b->flags = B_BUSY;
4192       release(&bcache.lock);
4193       return b;
4194     }
4195   }
4196   panic("bget: no buffers");
4197 }
4198 
4199 
4200 // Return a B_BUSY buf with the contents of the indicated disk sector.
4201 struct buf*
4202 bread(uint dev, uint sector)
4203 {
4204   struct buf *b;
4205 
4206   b = bget(dev, sector);
4207   if(!(b->flags & B_VALID))
4208     iderw(b);
4209   return b;
4210 }
4211 
4212 // Write b's contents to disk.  Must be B_BUSY.
4213 void
4214 bwrite(struct buf *b)
4215 {
4216   if((b->flags & B_BUSY) == 0)
4217     panic("bwrite");
4218   b->flags |= B_DIRTY;
4219   iderw(b);
4220 }
4221 
4222 // Release a B_BUSY buffer.
4223 // Move to the head of the MRU list.
4224 void
4225 brelse(struct buf *b)
4226 {
4227   if((b->flags & B_BUSY) == 0)
4228     panic("brelse");
4229 
4230   acquire(&bcache.lock);
4231 
4232   b->next->prev = b->prev;
4233   b->prev->next = b->next;
4234   b->next = bcache.head.next;
4235   b->prev = &bcache.head;
4236   bcache.head.next->prev = b;
4237   bcache.head.next = b;
4238 
4239   b->flags &= ~B_BUSY;
4240   wakeup(b);
4241 
4242   release(&bcache.lock);
4243 }
4244 
4245 
4246 
4247 
4248 
4249 
