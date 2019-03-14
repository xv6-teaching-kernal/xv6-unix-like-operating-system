6700 // The local APIC manages internal (non-I/O) interrupts.
6701 // See Chapter 8 & Appendix C of Intel processor manual volume 3.
6702 
6703 #include "types.h"
6704 #include "defs.h"
6705 #include "memlayout.h"
6706 #include "traps.h"
6707 #include "mmu.h"
6708 #include "x86.h"
6709 
6710 // Local APIC registers, divided by 4 for use as uint[] indices.
6711 #define ID      (0x0020/4)   // ID
6712 #define VER     (0x0030/4)   // Version
6713 #define TPR     (0x0080/4)   // Task Priority
6714 #define EOI     (0x00B0/4)   // EOI
6715 #define SVR     (0x00F0/4)   // Spurious Interrupt Vector
6716   #define ENABLE     0x00000100   // Unit Enable
6717 #define ESR     (0x0280/4)   // Error Status
6718 #define ICRLO   (0x0300/4)   // Interrupt Command
6719   #define INIT       0x00000500   // INIT/RESET
6720   #define STARTUP    0x00000600   // Startup IPI
6721   #define DELIVS     0x00001000   // Delivery status
6722   #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
6723   #define DEASSERT   0x00000000
6724   #define LEVEL      0x00008000   // Level triggered
6725   #define BCAST      0x00080000   // Send to all APICs, including self.
6726   #define BUSY       0x00001000
6727   #define FIXED      0x00000000
6728 #define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
6729 #define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
6730   #define X1         0x0000000B   // divide counts by 1
6731   #define PERIODIC   0x00020000   // Periodic
6732 #define PCINT   (0x0340/4)   // Performance Counter LVT
6733 #define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
6734 #define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
6735 #define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
6736   #define MASKED     0x00010000   // Interrupt masked
6737 #define TICR    (0x0380/4)   // Timer Initial Count
6738 #define TCCR    (0x0390/4)   // Timer Current Count
6739 #define TDCR    (0x03E0/4)   // Timer Divide Configuration
6740 
6741 volatile uint *lapic;  // Initialized in mp.c
6742 
6743 static void
6744 lapicw(int index, int value)
6745 {
6746   lapic[index] = value;
6747   lapic[ID];  // wait for write to finish, by reading
6748 }
6749 
6750 void
6751 lapicinit(void)
6752 {
6753   if(!lapic)
6754     return;
6755 
6756   // Enable local APIC; set spurious interrupt vector.
6757   lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
6758 
6759   // The timer repeatedly counts down at bus frequency
6760   // from lapic[TICR] and then issues an interrupt.
6761   // If xv6 cared more about precise timekeeping,
6762   // TICR would be calibrated using an external time source.
6763   lapicw(TDCR, X1);
6764   lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
6765   lapicw(TICR, 10000000);
6766 
6767   // Disable logical interrupt lines.
6768   lapicw(LINT0, MASKED);
6769   lapicw(LINT1, MASKED);
6770 
6771   // Disable performance counter overflow interrupts
6772   // on machines that provide that interrupt entry.
6773   if(((lapic[VER]>>16) & 0xFF) >= 4)
6774     lapicw(PCINT, MASKED);
6775 
6776   // Map error interrupt to IRQ_ERROR.
6777   lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
6778 
6779   // Clear error status register (requires back-to-back writes).
6780   lapicw(ESR, 0);
6781   lapicw(ESR, 0);
6782 
6783   // Ack any outstanding interrupts.
6784   lapicw(EOI, 0);
6785 
6786   // Send an Init Level De-Assert to synchronise arbitration ID's.
6787   lapicw(ICRHI, 0);
6788   lapicw(ICRLO, BCAST | INIT | LEVEL);
6789   while(lapic[ICRLO] & DELIVS)
6790     ;
6791 
6792   // Enable interrupts on the APIC (but not on the processor).
6793   lapicw(TPR, 0);
6794 }
6795 
6796 
6797 
6798 
6799 
6800 int
6801 cpunum(void)
6802 {
6803   // Cannot call cpu when interrupts are enabled:
6804   // result not guaranteed to last long enough to be used!
6805   // Would prefer to panic but even printing is chancy here:
6806   // almost everything, including cprintf and panic, calls cpu,
6807   // often indirectly through acquire and release.
6808   if(readeflags()&FL_IF){
6809     static int n;
6810     if(n++ == 0)
6811       cprintf("cpu called from %x with interrupts enabled\n",
6812         __builtin_return_address(0));
6813   }
6814 
6815   if(lapic)
6816     return lapic[ID]>>24;
6817   return 0;
6818 }
6819 
6820 // Acknowledge interrupt.
6821 void
6822 lapiceoi(void)
6823 {
6824   if(lapic)
6825     lapicw(EOI, 0);
6826 }
6827 
6828 // Spin for a given number of microseconds.
6829 // On real hardware would want to tune this dynamically.
6830 void
6831 microdelay(int us)
6832 {
6833 }
6834 
6835 #define IO_RTC  0x70
6836 
6837 // Start additional processor running entry code at addr.
6838 // See Appendix B of MultiProcessor Specification.
6839 void
6840 lapicstartap(uchar apicid, uint addr)
6841 {
6842   int i;
6843   ushort *wrv;
6844 
6845   // "The BSP must initialize CMOS shutdown code to 0AH
6846   // and the warm reset vector (DWORD based at 40:67) to point at
6847   // the AP startup code prior to the [universal startup algorithm]."
6848   outb(IO_RTC, 0xF);  // offset 0xF is shutdown code
6849   outb(IO_RTC+1, 0x0A);
6850   wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
6851   wrv[0] = 0;
6852   wrv[1] = addr >> 4;
6853 
6854   // "Universal startup algorithm."
6855   // Send INIT (level-triggered) interrupt to reset other CPU.
6856   lapicw(ICRHI, apicid<<24);
6857   lapicw(ICRLO, INIT | LEVEL | ASSERT);
6858   microdelay(200);
6859   lapicw(ICRLO, INIT | LEVEL);
6860   microdelay(100);    // should be 10ms, but too slow in Bochs!
6861 
6862   // Send startup IPI (twice!) to enter code.
6863   // Regular hardware is supposed to only accept a STARTUP
6864   // when it is in the halted state due to an INIT.  So the second
6865   // should be ignored, but it is part of the official Intel algorithm.
6866   // Bochs complains about the second one.  Too bad for Bochs.
6867   for(i = 0; i < 2; i++){
6868     lapicw(ICRHI, apicid<<24);
6869     lapicw(ICRLO, STARTUP | (addr>>12));
6870     microdelay(200);
6871   }
6872 }
6873 
6874 
6875 
6876 
6877 
6878 
6879 
6880 
6881 
6882 
6883 
6884 
6885 
6886 
6887 
6888 
6889 
6890 
6891 
6892 
6893 
6894 
6895 
6896 
6897 
6898 
6899 
