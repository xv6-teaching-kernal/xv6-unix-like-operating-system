7000 // Intel 8259A programmable interrupt controllers.
7001 
7002 #include "types.h"
7003 #include "x86.h"
7004 #include "traps.h"
7005 
7006 // I/O Addresses of the two programmable interrupt controllers
7007 #define IO_PIC1         0x20    // Master (IRQs 0-7)
7008 #define IO_PIC2         0xA0    // Slave (IRQs 8-15)
7009 
7010 #define IRQ_SLAVE       2       // IRQ at which slave connects to master
7011 
7012 // Current IRQ mask.
7013 // Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
7014 static ushort irqmask = 0xFFFF & ~(1<<IRQ_SLAVE);
7015 
7016 static void
7017 picsetmask(ushort mask)
7018 {
7019   irqmask = mask;
7020   outb(IO_PIC1+1, mask);
7021   outb(IO_PIC2+1, mask >> 8);
7022 }
7023 
7024 void
7025 picenable(int irq)
7026 {
7027   picsetmask(irqmask & ~(1<<irq));
7028 }
7029 
7030 // Initialize the 8259A interrupt controllers.
7031 void
7032 picinit(void)
7033 {
7034   // mask all interrupts
7035   outb(IO_PIC1+1, 0xFF);
7036   outb(IO_PIC2+1, 0xFF);
7037 
7038   // Set up master (8259A-1)
7039 
7040   // ICW1:  0001g0hi
7041   //    g:  0 = edge triggering, 1 = level triggering
7042   //    h:  0 = cascaded PICs, 1 = master only
7043   //    i:  0 = no ICW4, 1 = ICW4 required
7044   outb(IO_PIC1, 0x11);
7045 
7046   // ICW2:  Vector offset
7047   outb(IO_PIC1+1, T_IRQ0);
7048 
7049 
7050   // ICW3:  (master PIC) bit mask of IR lines connected to slaves
7051   //        (slave PIC) 3-bit # of slave's connection to master
7052   outb(IO_PIC1+1, 1<<IRQ_SLAVE);
7053 
7054   // ICW4:  000nbmap
7055   //    n:  1 = special fully nested mode
7056   //    b:  1 = buffered mode
7057   //    m:  0 = slave PIC, 1 = master PIC
7058   //      (ignored when b is 0, as the master/slave role
7059   //      can be hardwired).
7060   //    a:  1 = Automatic EOI mode
7061   //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
7062   outb(IO_PIC1+1, 0x3);
7063 
7064   // Set up slave (8259A-2)
7065   outb(IO_PIC2, 0x11);                  // ICW1
7066   outb(IO_PIC2+1, T_IRQ0 + 8);      // ICW2
7067   outb(IO_PIC2+1, IRQ_SLAVE);           // ICW3
7068   // NB Automatic EOI mode doesn't tend to work on the slave.
7069   // Linux source code says it's "to be investigated".
7070   outb(IO_PIC2+1, 0x3);                 // ICW4
7071 
7072   // OCW3:  0ef01prs
7073   //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
7074   //    p:  0 = no polling, 1 = polling mode
7075   //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
7076   outb(IO_PIC1, 0x68);             // clear specific mask
7077   outb(IO_PIC1, 0x0a);             // read IRR by default
7078 
7079   outb(IO_PIC2, 0x68);             // OCW3
7080   outb(IO_PIC2, 0x0a);             // OCW3
7081 
7082   if(irqmask != 0xFFFF)
7083     picsetmask(irqmask);
7084 }
7085 
7086 
7087 
7088 
7089 
7090 
7091 
7092 
7093 
7094 
7095 
7096 
7097 
7098 
7099 
