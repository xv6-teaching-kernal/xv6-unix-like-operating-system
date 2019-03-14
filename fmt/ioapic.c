6900 // The I/O APIC manages hardware interrupts for an SMP system.
6901 // http://www.intel.com/design/chipsets/datashts/29056601.pdf
6902 // See also picirq.c.
6903 
6904 #include "types.h"
6905 #include "defs.h"
6906 #include "traps.h"
6907 
6908 #define IOAPIC  0xFEC00000   // Default physical address of IO APIC
6909 
6910 #define REG_ID     0x00  // Register index: ID
6911 #define REG_VER    0x01  // Register index: version
6912 #define REG_TABLE  0x10  // Redirection table base
6913 
6914 // The redirection table starts at REG_TABLE and uses
6915 // two registers to configure each interrupt.
6916 // The first (low) register in a pair contains configuration bits.
6917 // The second (high) register contains a bitmask telling which
6918 // CPUs can serve that interrupt.
6919 #define INT_DISABLED   0x00010000  // Interrupt disabled
6920 #define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
6921 #define INT_ACTIVELOW  0x00002000  // Active low (vs high)
6922 #define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)
6923 
6924 volatile struct ioapic *ioapic;
6925 
6926 // IO APIC MMIO structure: write reg, then read or write data.
6927 struct ioapic {
6928   uint reg;
6929   uint pad[3];
6930   uint data;
6931 };
6932 
6933 static uint
6934 ioapicread(int reg)
6935 {
6936   ioapic->reg = reg;
6937   return ioapic->data;
6938 }
6939 
6940 static void
6941 ioapicwrite(int reg, uint data)
6942 {
6943   ioapic->reg = reg;
6944   ioapic->data = data;
6945 }
6946 
6947 
6948 
6949 
6950 void
6951 ioapicinit(void)
6952 {
6953   int i, id, maxintr;
6954 
6955   if(!ismp)
6956     return;
6957 
6958   ioapic = (volatile struct ioapic*)IOAPIC;
6959   maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
6960   id = ioapicread(REG_ID) >> 24;
6961   if(id != ioapicid)
6962     cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
6963 
6964   // Mark all interrupts edge-triggered, active high, disabled,
6965   // and not routed to any CPUs.
6966   for(i = 0; i <= maxintr; i++){
6967     ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
6968     ioapicwrite(REG_TABLE+2*i+1, 0);
6969   }
6970 }
6971 
6972 void
6973 ioapicenable(int irq, int cpunum)
6974 {
6975   if(!ismp)
6976     return;
6977 
6978   // Mark interrupt edge-triggered, active high,
6979   // enabled, and routed to the given cpunum,
6980   // which happens to be that cpu's APIC ID.
6981   ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
6982   ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
6983 }
6984 
6985 
6986 
6987 
6988 
6989 
6990 
6991 
6992 
6993 
6994 
6995 
6996 
6997 
6998 
6999 
