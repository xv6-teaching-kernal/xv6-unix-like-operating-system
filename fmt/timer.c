7650 // Intel 8253/8254/82C54 Programmable Interval Timer (PIT).
7651 // Only used on uniprocessors;
7652 // SMP machines use the local APIC timer.
7653 
7654 #include "types.h"
7655 #include "defs.h"
7656 #include "traps.h"
7657 #include "x86.h"
7658 
7659 #define IO_TIMER1       0x040           // 8253 Timer #1
7660 
7661 // Frequency of all three count-down timers;
7662 // (TIMER_FREQ/freq) is the appropriate count
7663 // to generate a frequency of freq Hz.
7664 
7665 #define TIMER_FREQ      1193182
7666 #define TIMER_DIV(x)    ((TIMER_FREQ+(x)/2)/(x))
7667 
7668 #define TIMER_MODE      (IO_TIMER1 + 3) // timer mode port
7669 #define TIMER_SEL0      0x00    // select counter 0
7670 #define TIMER_RATEGEN   0x04    // mode 2, rate generator
7671 #define TIMER_16BIT     0x30    // r/w counter 16 bits, LSB first
7672 
7673 void
7674 timerinit(void)
7675 {
7676   // Interrupt 100 times/sec.
7677   outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
7678   outb(IO_TIMER1, TIMER_DIV(100) % 256);
7679   outb(IO_TIMER1, TIMER_DIV(100) / 256);
7680   picenable(IRQ_TIMER);
7681 }
7682 
7683 
7684 
7685 
7686 
7687 
7688 
7689 
7690 
7691 
7692 
7693 
7694 
7695 
7696 
7697 
7698 
7699 
