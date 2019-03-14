7700 // Intel 8250 serial port (UART).
7701 
7702 #include "types.h"
7703 #include "defs.h"
7704 #include "param.h"
7705 #include "traps.h"
7706 #include "spinlock.h"
7707 #include "fs.h"
7708 #include "file.h"
7709 #include "mmu.h"
7710 #include "proc.h"
7711 #include "x86.h"
7712 
7713 #define COM1    0x3f8
7714 
7715 static int uart;    // is there a uart?
7716 
7717 void
7718 uartinit(void)
7719 {
7720   char *p;
7721 
7722   // Turn off the FIFO
7723   outb(COM1+2, 0);
7724 
7725   // 9600 baud, 8 data bits, 1 stop bit, parity off.
7726   outb(COM1+3, 0x80);    // Unlock divisor
7727   outb(COM1+0, 115200/9600);
7728   outb(COM1+1, 0);
7729   outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
7730   outb(COM1+4, 0);
7731   outb(COM1+1, 0x01);    // Enable receive interrupts.
7732 
7733   // If status is 0xFF, no serial port.
7734   if(inb(COM1+5) == 0xFF)
7735     return;
7736   uart = 1;
7737 
7738   // Acknowledge pre-existing interrupt conditions;
7739   // enable interrupts.
7740   inb(COM1+2);
7741   inb(COM1+0);
7742   picenable(IRQ_COM1);
7743   ioapicenable(IRQ_COM1, 0);
7744 
7745   // Announce that we're here.
7746   for(p="xv6...\n"; *p; p++)
7747     uartputc(*p);
7748 }
7749 
7750 void
7751 uartputc(int c)
7752 {
7753   int i;
7754 
7755   if(!uart)
7756     return;
7757   for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
7758     microdelay(10);
7759   outb(COM1+0, c);
7760 }
7761 
7762 static int
7763 uartgetc(void)
7764 {
7765   if(!uart)
7766     return -1;
7767   if(!(inb(COM1+5) & 0x01))
7768     return -1;
7769   return inb(COM1+0);
7770 }
7771 
7772 void
7773 uartintr(void)
7774 {
7775   consoleintr(uartgetc);
7776 }
7777 
7778 
7779 
7780 
7781 
7782 
7783 
7784 
7785 
7786 
7787 
7788 
7789 
7790 
7791 
7792 
7793 
7794 
7795 
7796 
7797 
7798 
7799 
