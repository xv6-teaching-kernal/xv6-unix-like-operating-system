6500 // Multiprocessor support
6501 // Search memory for MP description structures.
6502 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
6503 
6504 #include "types.h"
6505 #include "defs.h"
6506 #include "param.h"
6507 #include "memlayout.h"
6508 #include "mp.h"
6509 #include "x86.h"
6510 #include "mmu.h"
6511 #include "proc.h"
6512 
6513 struct cpu cpus[NCPU];
6514 static struct cpu *bcpu;
6515 int ismp;
6516 int ncpu;
6517 uchar ioapicid;
6518 
6519 int
6520 mpbcpu(void)
6521 {
6522   return bcpu-cpus;
6523 }
6524 
6525 static uchar
6526 sum(uchar *addr, int len)
6527 {
6528   int i, sum;
6529 
6530   sum = 0;
6531   for(i=0; i<len; i++)
6532     sum += addr[i];
6533   return sum;
6534 }
6535 
6536 // Look for an MP structure in the len bytes at addr.
6537 static struct mp*
6538 mpsearch1(uint a, int len)
6539 {
6540   uchar *e, *p, *addr;
6541 
6542   addr = p2v(a);
6543   e = addr+len;
6544   for(p = addr; p < e; p += sizeof(struct mp))
6545     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
6546       return (struct mp*)p;
6547   return 0;
6548 }
6549 
6550 // Search for the MP Floating Pointer Structure, which according to the
6551 // spec is in one of the following three locations:
6552 // 1) in the first KB of the EBDA;
6553 // 2) in the last KB of system base memory;
6554 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
6555 static struct mp*
6556 mpsearch(void)
6557 {
6558   uchar *bda;
6559   uint p;
6560   struct mp *mp;
6561 
6562   bda = (uchar *) P2V(0x400);
6563   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
6564     if((mp = mpsearch1(p, 1024)))
6565       return mp;
6566   } else {
6567     p = ((bda[0x14]<<8)|bda[0x13])*1024;
6568     if((mp = mpsearch1(p-1024, 1024)))
6569       return mp;
6570   }
6571   return mpsearch1(0xF0000, 0x10000);
6572 }
6573 
6574 // Search for an MP configuration table.  For now,
6575 // don't accept the default configurations (physaddr == 0).
6576 // Check for correct signature, calculate the checksum and,
6577 // if correct, check the version.
6578 // To do: check extended table checksum.
6579 static struct mpconf*
6580 mpconfig(struct mp **pmp)
6581 {
6582   struct mpconf *conf;
6583   struct mp *mp;
6584 
6585   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
6586     return 0;
6587   conf = (struct mpconf*) p2v((uint) mp->physaddr);
6588   if(memcmp(conf, "PCMP", 4) != 0)
6589     return 0;
6590   if(conf->version != 1 && conf->version != 4)
6591     return 0;
6592   if(sum((uchar*)conf, conf->length) != 0)
6593     return 0;
6594   *pmp = mp;
6595   return conf;
6596 }
6597 
6598 
6599 
6600 void
6601 mpinit(void)
6602 {
6603   uchar *p, *e;
6604   struct mp *mp;
6605   struct mpconf *conf;
6606   struct mpproc *proc;
6607   struct mpioapic *ioapic;
6608 
6609   bcpu = &cpus[0];
6610   if((conf = mpconfig(&mp)) == 0)
6611     return;
6612   ismp = 1;
6613   lapic = (uint*)conf->lapicaddr;
6614   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
6615     switch(*p){
6616     case MPPROC:
6617       proc = (struct mpproc*)p;
6618       if(ncpu != proc->apicid){
6619         cprintf("mpinit: ncpu=%d apicid=%d\n", ncpu, proc->apicid);
6620         ismp = 0;
6621       }
6622       if(proc->flags & MPBOOT)
6623         bcpu = &cpus[ncpu];
6624       cpus[ncpu].id = ncpu;
6625       ncpu++;
6626       p += sizeof(struct mpproc);
6627       continue;
6628     case MPIOAPIC:
6629       ioapic = (struct mpioapic*)p;
6630       ioapicid = ioapic->apicno;
6631       p += sizeof(struct mpioapic);
6632       continue;
6633     case MPBUS:
6634     case MPIOINTR:
6635     case MPLINTR:
6636       p += 8;
6637       continue;
6638     default:
6639       cprintf("mpinit: unknown config type %x\n", *p);
6640       ismp = 0;
6641     }
6642   }
6643   if(!ismp){
6644     // Didn't like what we found; fall back to no MP.
6645     ncpu = 1;
6646     lapic = 0;
6647     ioapicid = 0;
6648     return;
6649   }
6650   if(mp->imcrp){
6651     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
6652     // But it would on real hardware.
6653     outb(0x22, 0x70);   // Select IMCR
6654     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
6655   }
6656 }
6657 
6658 
6659 
6660 
6661 
6662 
6663 
6664 
6665 
6666 
6667 
6668 
6669 
6670 
6671 
6672 
6673 
6674 
6675 
6676 
6677 
6678 
6679 
6680 
6681 
6682 
6683 
6684 
6685 
6686 
6687 
6688 
6689 
6690 
6691 
6692 
6693 
6694 
6695 
6696 
6697 
6698 
6699 
