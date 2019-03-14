4500 // File system implementation.  Five layers:
4501 //   + Blocks: allocator for raw disk blocks.
4502 //   + Log: crash recovery for multi-step updates.
4503 //   + Files: inode allocator, reading, writing, metadata.
4504 //   + Directories: inode with special contents (list of other inodes!)
4505 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
4506 //
4507 // This file contains the low-level file system manipulation
4508 // routines.  The (higher-level) system call implementations
4509 // are in sysfile.c.
4510 
4511 #include "types.h"
4512 #include "defs.h"
4513 #include "param.h"
4514 #include "stat.h"
4515 #include "mmu.h"
4516 #include "proc.h"
4517 #include "spinlock.h"
4518 #include "buf.h"
4519 #include "fs.h"
4520 #include "file.h"
4521 
4522 #define min(a, b) ((a) < (b) ? (a) : (b))
4523 static void itrunc(struct inode*);
4524 
4525 // Read the super block.
4526 void
4527 readsb(int dev, struct superblock *sb)
4528 {
4529   struct buf *bp;
4530 
4531   bp = bread(dev, 1);
4532   memmove(sb, bp->data, sizeof(*sb));
4533   brelse(bp);
4534 }
4535 
4536 // Zero a block.
4537 static void
4538 bzero(int dev, int bno)
4539 {
4540   struct buf *bp;
4541 
4542   bp = bread(dev, bno);
4543   memset(bp->data, 0, BSIZE);
4544   log_write(bp);
4545   brelse(bp);
4546 }
4547 
4548 
4549 
4550 // Blocks.
4551 
4552 // Allocate a zeroed disk block.
4553 static uint
4554 balloc(uint dev)
4555 {
4556   int b, bi, m;
4557   struct buf *bp;
4558   struct superblock sb;
4559 
4560   bp = 0;
4561   readsb(dev, &sb);
4562   for(b = 0; b < sb.size; b += BPB){
4563     bp = bread(dev, BBLOCK(b, sb.ninodes));
4564     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
4565       m = 1 << (bi % 8);
4566       if((bp->data[bi/8] & m) == 0){  // Is block free?
4567         bp->data[bi/8] |= m;  // Mark block in use.
4568         log_write(bp);
4569         brelse(bp);
4570         bzero(dev, b + bi);
4571         return b + bi;
4572       }
4573     }
4574     brelse(bp);
4575   }
4576   panic("balloc: out of blocks");
4577 }
4578 
4579 // Free a disk block.
4580 static void
4581 bfree(int dev, uint b)
4582 {
4583   struct buf *bp;
4584   struct superblock sb;
4585   int bi, m;
4586 
4587   readsb(dev, &sb);
4588   bp = bread(dev, BBLOCK(b, sb.ninodes));
4589   bi = b % BPB;
4590   m = 1 << (bi % 8);
4591   if((bp->data[bi/8] & m) == 0)
4592     panic("freeing free block");
4593   bp->data[bi/8] &= ~m;
4594   log_write(bp);
4595   brelse(bp);
4596 }
4597 
4598 
4599 
4600 // Inodes.
4601 //
4602 // An inode describes a single unnamed file.
4603 // The inode disk structure holds metadata: the file's type,
4604 // its size, the number of links referring to it, and the
4605 // list of blocks holding the file's content.
4606 //
4607 // The inodes are laid out sequentially on disk immediately after
4608 // the superblock. Each inode has a number, indicating its
4609 // position on the disk.
4610 //
4611 // The kernel keeps a cache of in-use inodes in memory
4612 // to provide a place for synchronizing access
4613 // to inodes used by multiple processes. The cached
4614 // inodes include book-keeping information that is
4615 // not stored on disk: ip->ref and ip->flags.
4616 //
4617 // An inode and its in-memory represtative go through a
4618 // sequence of states before they can be used by the
4619 // rest of the file system code.
4620 //
4621 // * Allocation: an inode is allocated if its type (on disk)
4622 //   is non-zero. ialloc() allocates, iput() frees if
4623 //   the link count has fallen to zero.
4624 //
4625 // * Referencing in cache: an entry in the inode cache
4626 //   is free if ip->ref is zero. Otherwise ip->ref tracks
4627 //   the number of in-memory pointers to the entry (open
4628 //   files and current directories). iget() to find or
4629 //   create a cache entry and increment its ref, iput()
4630 //   to decrement ref.
4631 //
4632 // * Valid: the information (type, size, &c) in an inode
4633 //   cache entry is only correct when the I_VALID bit
4634 //   is set in ip->flags. ilock() reads the inode from
4635 //   the disk and sets I_VALID, while iput() clears
4636 //   I_VALID if ip->ref has fallen to zero.
4637 //
4638 // * Locked: file system code may only examine and modify
4639 //   the information in an inode and its content if it
4640 //   has first locked the inode. The I_BUSY flag indicates
4641 //   that the inode is locked. ilock() sets I_BUSY,
4642 //   while iunlock clears it.
4643 //
4644 // Thus a typical sequence is:
4645 //   ip = iget(dev, inum)
4646 //   ilock(ip)
4647 //   ... examine and modify ip->xxx ...
4648 //   iunlock(ip)
4649 //   iput(ip)
4650 //
4651 // ilock() is separate from iget() so that system calls can
4652 // get a long-term reference to an inode (as for an open file)
4653 // and only lock it for short periods (e.g., in read()).
4654 // The separation also helps avoid deadlock and races during
4655 // pathname lookup. iget() increments ip->ref so that the inode
4656 // stays cached and pointers to it remain valid.
4657 //
4658 // Many internal file system functions expect the caller to
4659 // have locked the inodes involved; this lets callers create
4660 // multi-step atomic operations.
4661 
4662 struct {
4663   struct spinlock lock;
4664   struct inode inode[NINODE];
4665 } icache;
4666 
4667 void
4668 iinit(void)
4669 {
4670   initlock(&icache.lock, "icache");
4671 }
4672 
4673 static struct inode* iget(uint dev, uint inum);
4674 
4675 
4676 
4677 
4678 
4679 
4680 
4681 
4682 
4683 
4684 
4685 
4686 
4687 
4688 
4689 
4690 
4691 
4692 
4693 
4694 
4695 
4696 
4697 
4698 
4699 
4700 // Allocate a new inode with the given type on device dev.
4701 // A free inode has a type of zero.
4702 struct inode*
4703 ialloc(uint dev, short type)
4704 {
4705   int inum;
4706   struct buf *bp;
4707   struct dinode *dip;
4708   struct superblock sb;
4709 
4710   readsb(dev, &sb);
4711 
4712   for(inum = 1; inum < sb.ninodes; inum++){
4713     bp = bread(dev, IBLOCK(inum));
4714     dip = (struct dinode*)bp->data + inum%IPB;
4715     if(dip->type == 0){  // a free inode
4716       memset(dip, 0, sizeof(*dip));
4717       dip->type = type;
4718       log_write(bp);   // mark it allocated on the disk
4719       brelse(bp);
4720       return iget(dev, inum);
4721     }
4722     brelse(bp);
4723   }
4724   panic("ialloc: no inodes");
4725 }
4726 
4727 // Copy a modified in-memory inode to disk.
4728 void
4729 iupdate(struct inode *ip)
4730 {
4731   struct buf *bp;
4732   struct dinode *dip;
4733 
4734   bp = bread(ip->dev, IBLOCK(ip->inum));
4735   dip = (struct dinode*)bp->data + ip->inum%IPB;
4736   dip->type = ip->type;
4737   dip->major = ip->major;
4738   dip->minor = ip->minor;
4739   dip->nlink = ip->nlink;
4740   dip->size = ip->size;
4741   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
4742   log_write(bp);
4743   brelse(bp);
4744 }
4745 
4746 
4747 
4748 
4749 
4750 // Find the inode with number inum on device dev
4751 // and return the in-memory copy. Does not lock
4752 // the inode and does not read it from disk.
4753 static struct inode*
4754 iget(uint dev, uint inum)
4755 {
4756   struct inode *ip, *empty;
4757 
4758   acquire(&icache.lock);
4759 
4760   // Is the inode already cached?
4761   empty = 0;
4762   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
4763     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
4764       ip->ref++;
4765       release(&icache.lock);
4766       return ip;
4767     }
4768     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
4769       empty = ip;
4770   }
4771 
4772   // Recycle an inode cache entry.
4773   if(empty == 0)
4774     panic("iget: no inodes");
4775 
4776   ip = empty;
4777   ip->dev = dev;
4778   ip->inum = inum;
4779   ip->ref = 1;
4780   ip->flags = 0;
4781   release(&icache.lock);
4782 
4783   return ip;
4784 }
4785 
4786 // Increment reference count for ip.
4787 // Returns ip to enable ip = idup(ip1) idiom.
4788 struct inode*
4789 idup(struct inode *ip)
4790 {
4791   acquire(&icache.lock);
4792   ip->ref++;
4793   release(&icache.lock);
4794   return ip;
4795 }
4796 
4797 
4798 
4799 
4800 // Lock the given inode.
4801 // Reads the inode from disk if necessary.
4802 void
4803 ilock(struct inode *ip)
4804 {
4805   struct buf *bp;
4806   struct dinode *dip;
4807 
4808   if(ip == 0 || ip->ref < 1)
4809     panic("ilock");
4810 
4811   acquire(&icache.lock);
4812   while(ip->flags & I_BUSY)
4813     sleep(ip, &icache.lock);
4814   ip->flags |= I_BUSY;
4815   release(&icache.lock);
4816 
4817   if(!(ip->flags & I_VALID)){
4818     bp = bread(ip->dev, IBLOCK(ip->inum));
4819     dip = (struct dinode*)bp->data + ip->inum%IPB;
4820     ip->type = dip->type;
4821     ip->major = dip->major;
4822     ip->minor = dip->minor;
4823     ip->nlink = dip->nlink;
4824     ip->size = dip->size;
4825     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
4826     brelse(bp);
4827     ip->flags |= I_VALID;
4828     if(ip->type == 0)
4829       panic("ilock: no type");
4830   }
4831 }
4832 
4833 // Unlock the given inode.
4834 void
4835 iunlock(struct inode *ip)
4836 {
4837   if(ip == 0 || !(ip->flags & I_BUSY) || ip->ref < 1)
4838     panic("iunlock");
4839 
4840   acquire(&icache.lock);
4841   ip->flags &= ~I_BUSY;
4842   wakeup(ip);
4843   release(&icache.lock);
4844 }
4845 
4846 
4847 
4848 
4849 
4850 // Drop a reference to an in-memory inode.
4851 // If that was the last reference, the inode cache entry can
4852 // be recycled.
4853 // If that was the last reference and the inode has no links
4854 // to it, free the inode (and its content) on disk.
4855 void
4856 iput(struct inode *ip)
4857 {
4858   acquire(&icache.lock);
4859   if(ip->ref == 1 && (ip->flags & I_VALID) && ip->nlink == 0){
4860     // inode has no links: truncate and free inode.
4861     if(ip->flags & I_BUSY)
4862       panic("iput busy");
4863     ip->flags |= I_BUSY;
4864     release(&icache.lock);
4865     itrunc(ip);
4866     ip->type = 0;
4867     iupdate(ip);
4868     acquire(&icache.lock);
4869     ip->flags = 0;
4870     wakeup(ip);
4871   }
4872   ip->ref--;
4873   release(&icache.lock);
4874 }
4875 
4876 // Common idiom: unlock, then put.
4877 void
4878 iunlockput(struct inode *ip)
4879 {
4880   iunlock(ip);
4881   iput(ip);
4882 }
4883 
4884 
4885 
4886 
4887 
4888 
4889 
4890 
4891 
4892 
4893 
4894 
4895 
4896 
4897 
4898 
4899 
4900 // Inode content
4901 //
4902 // The content (data) associated with each inode is stored
4903 // in blocks on the disk. The first NDIRECT block numbers
4904 // are listed in ip->addrs[].  The next NINDIRECT blocks are
4905 // listed in block ip->addrs[NDIRECT].
4906 
4907 // Return the disk block address of the nth block in inode ip.
4908 // If there is no such block, bmap allocates one.
4909 static uint
4910 bmap(struct inode *ip, uint bn)
4911 {
4912   uint addr, *a;
4913   struct buf *bp;
4914 
4915   if(bn < NDIRECT){
4916     if((addr = ip->addrs[bn]) == 0)
4917       ip->addrs[bn] = addr = balloc(ip->dev);
4918     return addr;
4919   }
4920   bn -= NDIRECT;
4921 
4922   if(bn < NINDIRECT){
4923     // Load indirect block, allocating if necessary.
4924     if((addr = ip->addrs[NDIRECT]) == 0)
4925       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
4926     bp = bread(ip->dev, addr);
4927     a = (uint*)bp->data;
4928     if((addr = a[bn]) == 0){
4929       a[bn] = addr = balloc(ip->dev);
4930       log_write(bp);
4931     }
4932     brelse(bp);
4933     return addr;
4934   }
4935 
4936   panic("bmap: out of range");
4937 }
4938 
4939 
4940 
4941 
4942 
4943 
4944 
4945 
4946 
4947 
4948 
4949 
4950 // Truncate inode (discard contents).
4951 // Only called when the inode has no links
4952 // to it (no directory entries referring to it)
4953 // and has no in-memory reference to it (is
4954 // not an open file or current directory).
4955 static void
4956 itrunc(struct inode *ip)
4957 {
4958   int i, j;
4959   struct buf *bp;
4960   uint *a;
4961 
4962   for(i = 0; i < NDIRECT; i++){
4963     if(ip->addrs[i]){
4964       bfree(ip->dev, ip->addrs[i]);
4965       ip->addrs[i] = 0;
4966     }
4967   }
4968 
4969   if(ip->addrs[NDIRECT]){
4970     bp = bread(ip->dev, ip->addrs[NDIRECT]);
4971     a = (uint*)bp->data;
4972     for(j = 0; j < NINDIRECT; j++){
4973       if(a[j])
4974         bfree(ip->dev, a[j]);
4975     }
4976     brelse(bp);
4977     bfree(ip->dev, ip->addrs[NDIRECT]);
4978     ip->addrs[NDIRECT] = 0;
4979   }
4980 
4981   ip->size = 0;
4982   iupdate(ip);
4983 }
4984 
4985 // Copy stat information from inode.
4986 void
4987 stati(struct inode *ip, struct stat *st)
4988 {
4989   st->dev = ip->dev;
4990   st->ino = ip->inum;
4991   st->type = ip->type;
4992   st->nlink = ip->nlink;
4993   st->size = ip->size;
4994 }
4995 
4996 
4997 
4998 
4999 
5000 // Read data from inode.
5001 int
5002 readi(struct inode *ip, char *dst, uint off, uint n)
5003 {
5004   uint tot, m;
5005   struct buf *bp;
5006 
5007   if(ip->type == T_DEV){
5008     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
5009       return -1;
5010     return devsw[ip->major].read(ip, dst, n);
5011   }
5012 
5013   if(off > ip->size || off + n < off)
5014     return -1;
5015   if(off + n > ip->size)
5016     n = ip->size - off;
5017 
5018   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
5019     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5020     m = min(n - tot, BSIZE - off%BSIZE);
5021     memmove(dst, bp->data + off%BSIZE, m);
5022     brelse(bp);
5023   }
5024   return n;
5025 }
5026 
5027 
5028 
5029 
5030 
5031 
5032 
5033 
5034 
5035 
5036 
5037 
5038 
5039 
5040 
5041 
5042 
5043 
5044 
5045 
5046 
5047 
5048 
5049 
5050 // Write data to inode.
5051 int
5052 writei(struct inode *ip, char *src, uint off, uint n)
5053 {
5054   uint tot, m;
5055   struct buf *bp;
5056 
5057   if(ip->type == T_DEV){
5058     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
5059       return -1;
5060     return devsw[ip->major].write(ip, src, n);
5061   }
5062 
5063   if(off > ip->size || off + n < off)
5064     return -1;
5065   if(off + n > MAXFILE*BSIZE)
5066     return -1;
5067 
5068   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
5069     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5070     m = min(n - tot, BSIZE - off%BSIZE);
5071     memmove(bp->data + off%BSIZE, src, m);
5072     log_write(bp);
5073     brelse(bp);
5074   }
5075 
5076   if(n > 0 && off > ip->size){
5077     ip->size = off;
5078     iupdate(ip);
5079   }
5080   return n;
5081 }
5082 
5083 
5084 
5085 
5086 
5087 
5088 
5089 
5090 
5091 
5092 
5093 
5094 
5095 
5096 
5097 
5098 
5099 
5100 // Directories
5101 
5102 int
5103 namecmp(const char *s, const char *t)
5104 {
5105   return strncmp(s, t, DIRSIZ);
5106 }
5107 
5108 // Look for a directory entry in a directory.
5109 // If found, set *poff to byte offset of entry.
5110 struct inode*
5111 dirlookup(struct inode *dp, char *name, uint *poff)
5112 {
5113   uint off, inum;
5114   struct dirent de;
5115 
5116   if(dp->type != T_DIR)
5117     panic("dirlookup not DIR");
5118 
5119   for(off = 0; off < dp->size; off += sizeof(de)){
5120     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5121       panic("dirlink read");
5122     if(de.inum == 0)
5123       continue;
5124     if(namecmp(name, de.name) == 0){
5125       // entry matches path element
5126       if(poff)
5127         *poff = off;
5128       inum = de.inum;
5129       return iget(dp->dev, inum);
5130     }
5131   }
5132 
5133   return 0;
5134 }
5135 
5136 
5137 
5138 
5139 
5140 
5141 
5142 
5143 
5144 
5145 
5146 
5147 
5148 
5149 
5150 // Write a new directory entry (name, inum) into the directory dp.
5151 int
5152 dirlink(struct inode *dp, char *name, uint inum)
5153 {
5154   int off;
5155   struct dirent de;
5156   struct inode *ip;
5157 
5158   // Check that name is not present.
5159   if((ip = dirlookup(dp, name, 0)) != 0){
5160     iput(ip);
5161     return -1;
5162   }
5163 
5164   // Look for an empty dirent.
5165   for(off = 0; off < dp->size; off += sizeof(de)){
5166     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5167       panic("dirlink read");
5168     if(de.inum == 0)
5169       break;
5170   }
5171 
5172   strncpy(de.name, name, DIRSIZ);
5173   de.inum = inum;
5174   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5175     panic("dirlink");
5176 
5177   return 0;
5178 }
5179 
5180 
5181 
5182 
5183 
5184 
5185 
5186 
5187 
5188 
5189 
5190 
5191 
5192 
5193 
5194 
5195 
5196 
5197 
5198 
5199 
5200 // Paths
5201 
5202 // Copy the next path element from path into name.
5203 // Return a pointer to the element following the copied one.
5204 // The returned path has no leading slashes,
5205 // so the caller can check *path=='\0' to see if the name is the last one.
5206 // If no name to remove, return 0.
5207 //
5208 // Examples:
5209 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
5210 //   skipelem("///a//bb", name) = "bb", setting name = "a"
5211 //   skipelem("a", name) = "", setting name = "a"
5212 //   skipelem("", name) = skipelem("////", name) = 0
5213 //
5214 static char*
5215 skipelem(char *path, char *name)
5216 {
5217   char *s;
5218   int len;
5219 
5220   while(*path == '/')
5221     path++;
5222   if(*path == 0)
5223     return 0;
5224   s = path;
5225   while(*path != '/' && *path != 0)
5226     path++;
5227   len = path - s;
5228   if(len >= DIRSIZ)
5229     memmove(name, s, DIRSIZ);
5230   else {
5231     memmove(name, s, len);
5232     name[len] = 0;
5233   }
5234   while(*path == '/')
5235     path++;
5236   return path;
5237 }
5238 
5239 
5240 
5241 
5242 
5243 
5244 
5245 
5246 
5247 
5248 
5249 
5250 // Look up and return the inode for a path name.
5251 // If parent != 0, return the inode for the parent and copy the final
5252 // path element into name, which must have room for DIRSIZ bytes.
5253 static struct inode*
5254 namex(char *path, int nameiparent, char *name)
5255 {
5256   struct inode *ip, *next;
5257 
5258   if(*path == '/')
5259     ip = iget(ROOTDEV, ROOTINO);
5260   else
5261     ip = idup(proc->cwd);
5262 
5263   while((path = skipelem(path, name)) != 0){
5264     ilock(ip);
5265     if(ip->type != T_DIR){
5266       iunlockput(ip);
5267       return 0;
5268     }
5269     if(nameiparent && *path == '\0'){
5270       // Stop one level early.
5271       iunlock(ip);
5272       return ip;
5273     }
5274     if((next = dirlookup(ip, name, 0)) == 0){
5275       iunlockput(ip);
5276       return 0;
5277     }
5278     iunlockput(ip);
5279     ip = next;
5280   }
5281   if(nameiparent){
5282     iput(ip);
5283     return 0;
5284   }
5285   return ip;
5286 }
5287 
5288 struct inode*
5289 namei(char *path)
5290 {
5291   char name[DIRSIZ];
5292   return namex(path, 0, name);
5293 }
5294 
5295 struct inode*
5296 nameiparent(char *path, char *name)
5297 {
5298   return namex(path, 1, name);
5299 }
