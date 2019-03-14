5300 //
5301 // File descriptors
5302 //
5303 
5304 #include "types.h"
5305 #include "defs.h"
5306 #include "param.h"
5307 #include "fs.h"
5308 #include "file.h"
5309 #include "spinlock.h"
5310 
5311 struct devsw devsw[NDEV];
5312 struct {
5313   struct spinlock lock;
5314   struct file file[NFILE];
5315 } ftable;
5316 
5317 void
5318 fileinit(void)
5319 {
5320   initlock(&ftable.lock, "ftable");
5321 }
5322 
5323 // Allocate a file structure.
5324 struct file*
5325 filealloc(void)
5326 {
5327   struct file *f;
5328 
5329   acquire(&ftable.lock);
5330   for(f = ftable.file; f < ftable.file + NFILE; f++){
5331     if(f->ref == 0){
5332       f->ref = 1;
5333       release(&ftable.lock);
5334       return f;
5335     }
5336   }
5337   release(&ftable.lock);
5338   return 0;
5339 }
5340 
5341 
5342 
5343 
5344 
5345 
5346 
5347 
5348 
5349 
5350 // Increment ref count for file f.
5351 struct file*
5352 filedup(struct file *f)
5353 {
5354   acquire(&ftable.lock);
5355   if(f->ref < 1)
5356     panic("filedup");
5357   f->ref++;
5358   release(&ftable.lock);
5359   return f;
5360 }
5361 
5362 // Close file f.  (Decrement ref count, close when reaches 0.)
5363 void
5364 fileclose(struct file *f)
5365 {
5366   struct file ff;
5367 
5368   acquire(&ftable.lock);
5369   if(f->ref < 1)
5370     panic("fileclose");
5371   if(--f->ref > 0){
5372     release(&ftable.lock);
5373     return;
5374   }
5375   ff = *f;
5376   f->ref = 0;
5377   f->type = FD_NONE;
5378   release(&ftable.lock);
5379 
5380   if(ff.type == FD_PIPE)
5381     pipeclose(ff.pipe, ff.writable);
5382   else if(ff.type == FD_INODE){
5383     begin_trans();
5384     iput(ff.ip);
5385     commit_trans();
5386   }
5387 }
5388 
5389 
5390 
5391 
5392 
5393 
5394 
5395 
5396 
5397 
5398 
5399 
5400 // Get metadata about file f.
5401 int
5402 filestat(struct file *f, struct stat *st)
5403 {
5404   if(f->type == FD_INODE){
5405     ilock(f->ip);
5406     stati(f->ip, st);
5407     iunlock(f->ip);
5408     return 0;
5409   }
5410   return -1;
5411 }
5412 
5413 // Read from file f.
5414 int
5415 fileread(struct file *f, char *addr, int n)
5416 {
5417   int r;
5418 
5419   if(f->readable == 0)
5420     return -1;
5421   if(f->type == FD_PIPE)
5422     return piperead(f->pipe, addr, n);
5423   if(f->type == FD_INODE){
5424     ilock(f->ip);
5425     if((r = readi(f->ip, addr, f->off, n)) > 0)
5426       f->off += r;
5427     iunlock(f->ip);
5428     return r;
5429   }
5430   panic("fileread");
5431 }
5432 
5433 
5434 
5435 
5436 
5437 
5438 
5439 
5440 
5441 
5442 
5443 
5444 
5445 
5446 
5447 
5448 
5449 
5450 // Write to file f.
5451 int
5452 filewrite(struct file *f, char *addr, int n)
5453 {
5454   int r;
5455 
5456   if(f->writable == 0)
5457     return -1;
5458   if(f->type == FD_PIPE)
5459     return pipewrite(f->pipe, addr, n);
5460   if(f->type == FD_INODE){
5461     // write a few blocks at a time to avoid exceeding
5462     // the maximum log transaction size, including
5463     // i-node, indirect block, allocation blocks,
5464     // and 2 blocks of slop for non-aligned writes.
5465     // this really belongs lower down, since writei()
5466     // might be writing a device like the console.
5467     int max = ((LOGSIZE-1-1-2) / 2) * 512;
5468     int i = 0;
5469     while(i < n){
5470       int n1 = n - i;
5471       if(n1 > max)
5472         n1 = max;
5473 
5474       begin_trans();
5475       ilock(f->ip);
5476       if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
5477         f->off += r;
5478       iunlock(f->ip);
5479       commit_trans();
5480 
5481       if(r < 0)
5482         break;
5483       if(r != n1)
5484         panic("short filewrite");
5485       i += r;
5486     }
5487     return i == n ? n : -1;
5488   }
5489   panic("filewrite");
5490 }
5491 
5492 
5493 
5494 
5495 
5496 
5497 
5498 
5499 
