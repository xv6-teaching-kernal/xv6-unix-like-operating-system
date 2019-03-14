6400 // See MultiProcessor Specification Version 1.[14]
6401 
6402 struct mp {             // floating pointer
6403   uchar signature[4];           // "_MP_"
6404   void *physaddr;               // phys addr of MP config table
6405   uchar length;                 // 1
6406   uchar specrev;                // [14]
6407   uchar checksum;               // all bytes must add up to 0
6408   uchar type;                   // MP system config type
6409   uchar imcrp;
6410   uchar reserved[3];
6411 };
6412 
6413 struct mpconf {         // configuration table header
6414   uchar signature[4];           // "PCMP"
6415   ushort length;                // total table length
6416   uchar version;                // [14]
6417   uchar checksum;               // all bytes must add up to 0
6418   uchar product[20];            // product id
6419   uint *oemtable;               // OEM table pointer
6420   ushort oemlength;             // OEM table length
6421   ushort entry;                 // entry count
6422   uint *lapicaddr;              // address of local APIC
6423   ushort xlength;               // extended table length
6424   uchar xchecksum;              // extended table checksum
6425   uchar reserved;
6426 };
6427 
6428 struct mpproc {         // processor table entry
6429   uchar type;                   // entry type (0)
6430   uchar apicid;                 // local APIC id
6431   uchar version;                // local APIC verison
6432   uchar flags;                  // CPU flags
6433     #define MPBOOT 0x02           // This proc is the bootstrap processor.
6434   uchar signature[4];           // CPU signature
6435   uint feature;                 // feature flags from CPUID instruction
6436   uchar reserved[8];
6437 };
6438 
6439 struct mpioapic {       // I/O APIC table entry
6440   uchar type;                   // entry type (2)
6441   uchar apicno;                 // I/O APIC id
6442   uchar version;                // I/O APIC version
6443   uchar flags;                  // I/O APIC flags
6444   uint *addr;                  // I/O APIC address
6445 };
6446 
6447 
6448 
6449 
6450 // Table entry types
6451 #define MPPROC    0x00  // One per processor
6452 #define MPBUS     0x01  // One per bus
6453 #define MPIOAPIC  0x02  // One per I/O APIC
6454 #define MPIOINTR  0x03  // One per bus interrupt source
6455 #define MPLINTR   0x04  // One per system interrupt source
6456 
6457 
6458 
6459 
6460 
6461 
6462 
6463 
6464 
6465 
6466 
6467 
6468 
6469 
6470 
6471 
6472 
6473 
6474 
6475 
6476 
6477 
6478 
6479 
6480 
6481 
6482 
6483 
6484 
6485 
6486 
6487 
6488 
6489 
6490 
6491 
6492 
6493 
6494 
6495 
6496 
6497 
6498 
6499 
