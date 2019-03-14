4250 #include "types.h"
4251 #include "defs.h"
4252 #include "param.h"
4253 #include "spinlock.h"
4254 #include "fs.h"
4255 #include "buf.h"
4256 
4257 // Simple logging. Each system call that might write the file system
4258 // should be surrounded with begin_trans() and commit_trans() calls.
4259 //
4260 // The log holds at most one transaction at a time. Commit forces
4261 // the log (with commit record) to disk, then installs the affected
4262 // blocks to disk, then erases the log. begin_trans() ensures that
4263 // only one system call can be in a transaction; others must wait.
4264 //
4265 // Allowing only one transaction at a time means that the file
4266 // system code doesn't have to worry about the possibility of
4267 // one transaction reading a block that another one has modified,
4268 // for example an i-node block.
4269 //
4270 // Read-only system calls don't need to use transactions, though
4271 // this means that they may observe uncommitted data. I-node and
4272 // buffer locks prevent read-only calls from seeing inconsistent data.
4273 //
4274 // The log is a physical re-do log containing disk blocks.
4275 // The on-disk log format:
4276 //   header block, containing sector #s for block A, B, C, ...
4277 //   block A
4278 //   block B
4279 //   block C
4280 //   ...
4281 // Log appends are synchronous.
4282 
4283 // Contents of the header block, used for both the on-disk header block
4284 // and to keep track in memory of logged sector #s before commit.
4285 struct logheader {
4286   int n;
4287   int sector[LOGSIZE];
4288 };
4289 
4290 struct log {
4291   struct spinlock lock;
4292   int start;
4293   int size;
4294   int busy; // a transaction is active
4295   int dev;
4296   struct logheader lh;
4297 };
4298 
4299 
4300 struct log log;
4301 
4302 static void recover_from_log(void);
4303 
4304 void
4305 initlog(void)
4306 {
4307   if (sizeof(struct logheader) >= BSIZE)
4308     panic("initlog: too big logheader");
4309 
4310   struct superblock sb;
4311   initlock(&log.lock, "log");
4312   readsb(ROOTDEV, &sb);
4313   log.start = sb.size - sb.nlog;
4314   log.size = sb.nlog;
4315   log.dev = ROOTDEV;
4316   recover_from_log();
4317 }
4318 
4319 // Copy committed blocks from log to their home location
4320 static void
4321 install_trans(void)
4322 {
4323   int tail;
4324 
4325   for (tail = 0; tail < log.lh.n; tail++) {
4326     struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
4327     struct buf *dbuf = bread(log.dev, log.lh.sector[tail]); // read dst
4328     memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
4329     bwrite(dbuf);  // write dst to disk
4330     brelse(lbuf);
4331     brelse(dbuf);
4332   }
4333 }
4334 
4335 // Read the log header from disk into the in-memory log header
4336 static void
4337 read_head(void)
4338 {
4339   struct buf *buf = bread(log.dev, log.start);
4340   struct logheader *lh = (struct logheader *) (buf->data);
4341   int i;
4342   log.lh.n = lh->n;
4343   for (i = 0; i < log.lh.n; i++) {
4344     log.lh.sector[i] = lh->sector[i];
4345   }
4346   brelse(buf);
4347 }
4348 
4349 
4350 // Write in-memory log header to disk.
4351 // This is the true point at which the
4352 // current transaction commits.
4353 static void
4354 write_head(void)
4355 {
4356   struct buf *buf = bread(log.dev, log.start);
4357   struct logheader *hb = (struct logheader *) (buf->data);
4358   int i;
4359   hb->n = log.lh.n;
4360   for (i = 0; i < log.lh.n; i++) {
4361     hb->sector[i] = log.lh.sector[i];
4362   }
4363   bwrite(buf);
4364   brelse(buf);
4365 }
4366 
4367 static void
4368 recover_from_log(void)
4369 {
4370   read_head();
4371   install_trans(); // if committed, copy from log to disk
4372   log.lh.n = 0;
4373   write_head(); // clear the log
4374 }
4375 
4376 void
4377 begin_trans(void)
4378 {
4379   acquire(&log.lock);
4380   while (log.busy) {
4381     sleep(&log, &log.lock);
4382   }
4383   log.busy = 1;
4384   release(&log.lock);
4385 }
4386 
4387 
4388 
4389 
4390 
4391 
4392 
4393 
4394 
4395 
4396 
4397 
4398 
4399 
4400 void
4401 commit_trans(void)
4402 {
4403   if (log.lh.n > 0) {
4404     write_head();    // Write header to disk -- the real commit
4405     install_trans(); // Now install writes to home locations
4406     log.lh.n = 0;
4407     write_head();    // Erase the transaction from the log
4408   }
4409 
4410   acquire(&log.lock);
4411   log.busy = 0;
4412   wakeup(&log);
4413   release(&log.lock);
4414 }
4415 
4416 // Caller has modified b->data and is done with the buffer.
4417 // Append the block to the log and record the block number,
4418 // but don't write the log header (which would commit the write).
4419 // log_write() replaces bwrite(); a typical use is:
4420 //   bp = bread(...)
4421 //   modify bp->data[]
4422 //   log_write(bp)
4423 //   brelse(bp)
4424 void
4425 log_write(struct buf *b)
4426 {
4427   int i;
4428 
4429   if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
4430     panic("too big a transaction");
4431   if (!log.busy)
4432     panic("write outside of trans");
4433 
4434   for (i = 0; i < log.lh.n; i++) {
4435     if (log.lh.sector[i] == b->sector)   // log absorbtion?
4436       break;
4437   }
4438   log.lh.sector[i] = b->sector;
4439   struct buf *lbuf = bread(b->dev, log.start+i+1);
4440   memmove(lbuf->data, b->data, BSIZE);
4441   bwrite(lbuf);
4442   brelse(lbuf);
4443   if (i == log.lh.n)
4444     log.lh.n++;
4445   b->flags |= B_DIRTY; // XXX prevent eviction
4446 }
4447 
4448 
4449 
4450 // Blank page.
4451 
4452 
4453 
4454 
4455 
4456 
4457 
4458 
4459 
4460 
4461 
4462 
4463 
4464 
4465 
4466 
4467 
4468 
4469 
4470 
4471 
4472 
4473 
4474 
4475 
4476 
4477 
4478 
4479 
4480 
4481 
4482 
4483 
4484 
4485 
4486 
4487 
4488 
4489 
4490 
4491 
4492 
4493 
4494 
4495 
4496 
4497 
4498 
4499 
