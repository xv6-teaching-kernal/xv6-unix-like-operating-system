6000 #include "types.h"
6001 #include "param.h"
6002 #include "memlayout.h"
6003 #include "mmu.h"
6004 #include "proc.h"
6005 #include "defs.h"
6006 #include "x86.h"
6007 #include "elf.h"
6008 
6009 int exec(char *path, char **argv)
6010 {
6011   char *s, *last;
6012   int i, off;
6013   uint argc, sz, sp, ustack[3+MAXARG+1];
6014   struct elfhdr elf;
6015   struct inode *ip;
6016   struct proghdr ph;
6017   pde_t *pgdir, *oldpgdir;
6018 
6019   if((ip = namei(path)) == 0)
6020     return -1;
6021   ilock(ip);
6022   pgdir = 0;
6023 
6024   // Check ELF header
6025   if(readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf))
6026     goto bad;
6027   if(elf.magic != ELF_MAGIC)
6028     goto bad;
6029 
6030   if((pgdir = setupkvm()) == 0)
6031     goto bad;
6032 
6033   // Load program into memory.
6034   sz = 0;
6035   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
6036     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
6037       goto bad;
6038     if(ph.type != ELF_PROG_LOAD)
6039       continue;
6040     if(ph.memsz < ph.filesz)
6041       goto bad;
6042     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
6043       goto bad;
6044     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
6045       goto bad;
6046   }
6047   iunlockput(ip);
6048   ip = 0;
6049 
6050   // Allocate two pages at the next page boundary.
6051   // Make the first inaccessible.  Use the second as the user stack.
6052   sz = PGROUNDUP(sz);
6053   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
6054     goto bad;
6055   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
6056   sp = sz;
6057 
6058   // Push argument strings, prepare rest of stack in ustack.
6059   for(argc = 0; argv[argc]; argc++) {
6060     if(argc >= MAXARG)
6061       goto bad;
6062     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
6063     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
6064       goto bad;
6065     ustack[3+argc] = sp;
6066   }
6067   ustack[3+argc] = 0;
6068 
6069   ustack[0] = 0xffffffff;  // fake return PC
6070   ustack[1] = argc;
6071   ustack[2] = sp - (argc+1)*4;  // argv pointer
6072 
6073   sp -= (3+argc+1) * 4;
6074   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
6075     goto bad;
6076 
6077   // Save program name for debugging.
6078   for(last=s=path; *s; s++)
6079     if(*s == '/')
6080       last = s+1;
6081   safestrcpy(proc->name, last, sizeof(proc->name));
6082 
6083   // Commit to the user image.
6084   oldpgdir = proc->pgdir;
6085   proc->pgdir = pgdir;
6086   proc->sz = sz;
6087   proc->tf->eip = elf.entry;  // main
6088   proc->tf->esp = sp;
6089   switchuvm(proc);
6090   freevm(oldpgdir);
6091   return 0;
6092 
6093  bad:
6094   if(pgdir)
6095     freevm(pgdir);
6096   if(ip)
6097     iunlockput(ip);
6098   return -1;
6099 }
