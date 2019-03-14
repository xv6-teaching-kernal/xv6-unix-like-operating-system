8600 // Boot loader.
8601 //
8602 // Part of the boot sector, along with bootasm.S, which calls bootmain().
8603 // bootasm.S has put the processor into protected 32-bit mode.
8604 // bootmain() loads an ELF kernel image from the disk starting at
8605 // sector 1 and then jumps to the kernel entry routine.
8606 
8607 #include "types.h"
8608 #include "elf.h"
8609 #include "x86.h"
8610 #include "memlayout.h"
8611 
8612 #define SECTSIZE  512
8613 
8614 void readseg(uchar*, uint, uint);
8615 
8616 void
8617 bootmain(void)
8618 {
8619   struct elfhdr *elf;
8620   struct proghdr *ph, *eph;
8621   void (*entry)(void);
8622   uchar* pa;
8623 
8624   elf = (struct elfhdr*)0x10000;  // scratch space
8625 
8626   // Read 1st page off disk
8627   readseg((uchar*)elf, 4096, 0);
8628 
8629   // Is this an ELF executable?
8630   if(elf->magic != ELF_MAGIC)
8631     return;  // let bootasm.S handle error
8632 
8633   // Load each program segment (ignores ph flags).
8634   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
8635   eph = ph + elf->phnum;
8636   for(; ph < eph; ph++){
8637     pa = (uchar*)ph->paddr;
8638     readseg(pa, ph->filesz, ph->off);
8639     if(ph->memsz > ph->filesz)
8640       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
8641   }
8642 
8643   // Call the entry point from the ELF header.
8644   // Does not return!
8645   entry = (void(*)(void))(elf->entry);
8646   entry();
8647 }
8648 
8649 
8650 void
8651 waitdisk(void)
8652 {
8653   // Wait for disk ready.
8654   while((inb(0x1F7) & 0xC0) != 0x40)
8655     ;
8656 }
8657 
8658 // Read a single sector at offset into dst.
8659 void
8660 readsect(void *dst, uint offset)
8661 {
8662   // Issue command.
8663   waitdisk();
8664   outb(0x1F2, 1);   // count = 1
8665   outb(0x1F3, offset);
8666   outb(0x1F4, offset >> 8);
8667   outb(0x1F5, offset >> 16);
8668   outb(0x1F6, (offset >> 24) | 0xE0);
8669   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
8670 
8671   // Read data.
8672   waitdisk();
8673   insl(0x1F0, dst, SECTSIZE/4);
8674 }
8675 
8676 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
8677 // Might copy more than asked.
8678 void
8679 readseg(uchar* pa, uint count, uint offset)
8680 {
8681   uchar* epa;
8682 
8683   epa = pa + count;
8684 
8685   // Round down to sector boundary.
8686   pa -= offset % SECTSIZE;
8687 
8688   // Translate from bytes to sectors; kernel starts at sector 1.
8689   offset = (offset / SECTSIZE) + 1;
8690 
8691   // If this is too slow, we could read lots of sectors at a time.
8692   // We'd write more to memory than asked, but it doesn't matter --
8693   // we load in increasing order.
8694   for(; pa < epa; pa += SECTSIZE, offset++)
8695     readsect(pa, offset);
8696 }
8697 
8698 
8699 
