8500 #include "asm.h"
8501 #include "memlayout.h"
8502 #include "mmu.h"
8503 
8504 # Start the first CPU: switch to 32-bit protected mode, jump into C.
8505 # The BIOS loads this code from the first sector of the hard disk into
8506 # memory at physical address 0x7c00 and starts executing in real mode
8507 # with %cs=0 %ip=7c00.
8508 
8509 .code16                       # Assemble for 16-bit mode
8510 .globl start
8511 start:
8512   cli                         # BIOS enabled interrupts; disable
8513 
8514   # Zero data segment registers DS, ES, and SS.
8515   xorw    %ax,%ax             # Set %ax to zero
8516   movw    %ax,%ds             # -> Data Segment
8517   movw    %ax,%es             # -> Extra Segment
8518   movw    %ax,%ss             # -> Stack Segment
8519 
8520   # Physical address line A20 is tied to zero so that the first PCs
8521   # with 2 MB would run software that assumed 1 MB.  Undo that.
8522 seta20.1:
8523   inb     $0x64,%al               # Wait for not busy
8524   testb   $0x2,%al
8525   jnz     seta20.1
8526 
8527   movb    $0xd1,%al               # 0xd1 -> port 0x64
8528   outb    %al,$0x64
8529 
8530 seta20.2:
8531   inb     $0x64,%al               # Wait for not busy
8532   testb   $0x2,%al
8533   jnz     seta20.2
8534 
8535   movb    $0xdf,%al               # 0xdf -> port 0x60
8536   outb    %al,$0x60
8537 
8538   # Switch from real to protected mode.  Use a bootstrap GDT that makes
8539   # virtual addresses map directly to physical addresses so that the
8540   # effective memory map doesn't change during the transition.
8541   lgdt    gdtdesc
8542   movl    %cr0, %eax
8543   orl     $CR0_PE, %eax
8544   movl    %eax, %cr0
8545 
8546 
8547 
8548 
8549 
8550   # Complete transition to 32-bit protected mode by using long jmp
8551   # to reload %cs and %eip.  The segment descriptors are set up with no
8552   # translation, so that the mapping is still the identity mapping.
8553   ljmp    $(SEG_KCODE<<3), $start32
8554 
8555 .code32  # Tell assembler to generate 32-bit code now.
8556 start32:
8557   # Set up the protected-mode data segment registers
8558   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
8559   movw    %ax, %ds                # -> DS: Data Segment
8560   movw    %ax, %es                # -> ES: Extra Segment
8561   movw    %ax, %ss                # -> SS: Stack Segment
8562   movw    $0, %ax                 # Zero segments not ready for use
8563   movw    %ax, %fs                # -> FS
8564   movw    %ax, %gs                # -> GS
8565 
8566   # Set up the stack pointer and call into C.
8567   movl    $start, %esp
8568   call    bootmain
8569 
8570   # If bootmain returns (it shouldn't), trigger a Bochs
8571   # breakpoint if running under Bochs, then loop.
8572   movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
8573   movw    %ax, %dx
8574   outw    %ax, %dx
8575   movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
8576   outw    %ax, %dx
8577 spin:
8578   jmp     spin
8579 
8580 # Bootstrap GDT
8581 .p2align 2                                # force 4 byte alignment
8582 gdt:
8583   SEG_NULLASM                             # null seg
8584   SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
8585   SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
8586 
8587 gdtdesc:
8588   .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
8589   .long   gdt                             # address gdt
8590 
8591 
8592 
8593 
8594 
8595 
8596 
8597 
8598 
8599 
