7300 // Console input and output.
7301 // Input is from the keyboard or serial port.
7302 // Output is written to the screen and serial port.
7303 
7304 #include "types.h"
7305 #include "defs.h"
7306 #include "param.h"
7307 #include "traps.h"
7308 #include "spinlock.h"
7309 #include "fs.h"
7310 #include "file.h"
7311 #include "memlayout.h"
7312 #include "mmu.h"
7313 #include "proc.h"
7314 #include "x86.h"
7315 
7316 static void consputc(int);
7317 
7318 static int panicked = 0;
7319 
7320 static struct {
7321   struct spinlock lock;
7322   int locking;
7323 } cons;
7324 
7325 static void
7326 printint(int xx, int base, int sign)
7327 {
7328   static char digits[] = "0123456789abcdef";
7329   char buf[16];
7330   int i;
7331   uint x;
7332 
7333   if(sign && (sign = xx < 0))
7334     x = -xx;
7335   else
7336     x = xx;
7337 
7338   i = 0;
7339   do{
7340     buf[i++] = digits[x % base];
7341   }while((x /= base) != 0);
7342 
7343   if(sign)
7344     buf[i++] = '-';
7345 
7346   while(--i >= 0)
7347     consputc(buf[i]);
7348 }
7349 
7350 // Print to the console. only understands %d, %x, %p, %s.
7351 void
7352 cprintf(char *fmt, ...)
7353 {
7354   int i, c, locking;
7355   uint *argp;
7356   char *s;
7357 
7358   locking = cons.locking;
7359   if(locking)
7360     acquire(&cons.lock);
7361 
7362   if (fmt == 0)
7363     panic("null fmt");
7364 
7365   argp = (uint*)(void*)(&fmt + 1);
7366   for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
7367     if(c != '%'){
7368       consputc(c);
7369       continue;
7370     }
7371     c = fmt[++i] & 0xff;
7372     if(c == 0)
7373       break;
7374     switch(c){
7375     case 'd':
7376       printint(*argp++, 10, 1);
7377       break;
7378     case 'x':
7379     case 'p':
7380       printint(*argp++, 16, 0);
7381       break;
7382     case 's':
7383       if((s = (char*)*argp++) == 0)
7384         s = "(null)";
7385       for(; *s; s++)
7386         consputc(*s);
7387       break;
7388     case '%':
7389       consputc('%');
7390       break;
7391     default:
7392       // Print unknown % sequence to draw attention.
7393       consputc('%');
7394       consputc(c);
7395       break;
7396     }
7397   }
7398 
7399 
7400   if(locking)
7401     release(&cons.lock);
7402 }
7403 
7404 void
7405 panic(char *s)
7406 {
7407   int i;
7408   uint pcs[10];
7409 
7410   cli();
7411   cons.locking = 0;
7412   cprintf("cpu%d: panic: ", cpu->id);
7413   cprintf(s);
7414   cprintf("\n");
7415   getcallerpcs(&s, pcs);
7416   for(i=0; i<10; i++)
7417     cprintf(" %p", pcs[i]);
7418   panicked = 1; // freeze other CPU
7419   for(;;)
7420     ;
7421 }
7422 
7423 
7424 
7425 
7426 
7427 
7428 
7429 
7430 
7431 
7432 
7433 
7434 
7435 
7436 
7437 
7438 
7439 
7440 
7441 
7442 
7443 
7444 
7445 
7446 
7447 
7448 
7449 
7450 #define BACKSPACE 0x100
7451 #define CRTPORT 0x3d4
7452 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
7453 
7454 static void
7455 cgaputc(int c)
7456 {
7457   int pos;
7458 
7459   // Cursor position: col + 80*row.
7460   outb(CRTPORT, 14);
7461   pos = inb(CRTPORT+1) << 8;
7462   outb(CRTPORT, 15);
7463   pos |= inb(CRTPORT+1);
7464 
7465   if(c == '\n')
7466     pos += 80 - pos%80;
7467   else if(c == BACKSPACE){
7468     if(pos > 0) --pos;
7469   } else
7470     crt[pos++] = (c&0xff) | 0x0700;  // black on white
7471 
7472   if((pos/80) >= 24){  // Scroll up.
7473     memmove(crt, crt+80, sizeof(crt[0])*23*80);
7474     pos -= 80;
7475     memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
7476   }
7477 
7478   outb(CRTPORT, 14);
7479   outb(CRTPORT+1, pos>>8);
7480   outb(CRTPORT, 15);
7481   outb(CRTPORT+1, pos);
7482   crt[pos] = ' ' | 0x0700;
7483 }
7484 
7485 void
7486 consputc(int c)
7487 {
7488   if(panicked){
7489     cli();
7490     for(;;)
7491       ;
7492   }
7493 
7494   if(c == BACKSPACE){
7495     uartputc('\b'); uartputc(' '); uartputc('\b');
7496   } else
7497     uartputc(c);
7498   cgaputc(c);
7499 }
7500 #define INPUT_BUF 128
7501 struct {
7502   struct spinlock lock;
7503   char buf[INPUT_BUF];
7504   uint r;  // Read index
7505   uint w;  // Write index
7506   uint e;  // Edit index
7507 } input;
7508 
7509 #define C(x)  ((x)-'@')  // Control-x
7510 
7511 void
7512 consoleintr(int (*getc)(void))
7513 {
7514   int c;
7515 
7516   acquire(&input.lock);
7517   while((c = getc()) >= 0){
7518     switch(c){
7519     case C('P'):  // Process listing.
7520       procdump();
7521       break;
7522     case C('U'):  // Kill line.
7523       while(input.e != input.w &&
7524             input.buf[(input.e-1) % INPUT_BUF] != '\n'){
7525         input.e--;
7526         consputc(BACKSPACE);
7527       }
7528       break;
7529     case C('H'): case '\x7f':  // Backspace
7530       if(input.e != input.w){
7531         input.e--;
7532         consputc(BACKSPACE);
7533       }
7534       break;
7535     default:
7536       if(c != 0 && input.e-input.r < INPUT_BUF){
7537         c = (c == '\r') ? '\n' : c;
7538         input.buf[input.e++ % INPUT_BUF] = c;
7539         consputc(c);
7540         if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
7541           input.w = input.e;
7542           wakeup(&input.r);
7543         }
7544       }
7545       break;
7546     }
7547   }
7548   release(&input.lock);
7549 }
7550 int
7551 consoleread(struct inode *ip, char *dst, int n)
7552 {
7553   uint target;
7554   int c;
7555 
7556   iunlock(ip);
7557   target = n;
7558   acquire(&input.lock);
7559   while(n > 0){
7560     while(input.r == input.w){
7561       if(proc->killed){
7562         release(&input.lock);
7563         ilock(ip);
7564         return -1;
7565       }
7566       sleep(&input.r, &input.lock);
7567     }
7568     c = input.buf[input.r++ % INPUT_BUF];
7569     if(c == C('D')){  // EOF
7570       if(n < target){
7571         // Save ^D for next time, to make sure
7572         // caller gets a 0-byte result.
7573         input.r--;
7574       }
7575       break;
7576     }
7577     *dst++ = c;
7578     --n;
7579     if(c == '\n')
7580       break;
7581   }
7582   release(&input.lock);
7583   ilock(ip);
7584 
7585   return target - n;
7586 }
7587 
7588 
7589 
7590 
7591 
7592 
7593 
7594 
7595 
7596 
7597 
7598 
7599 
7600 int
7601 consolewrite(struct inode *ip, char *buf, int n)
7602 {
7603   int i;
7604 
7605   iunlock(ip);
7606   acquire(&cons.lock);
7607   for(i = 0; i < n; i++)
7608     consputc(buf[i] & 0xff);
7609   release(&cons.lock);
7610   ilock(ip);
7611 
7612   return n;
7613 }
7614 
7615 void
7616 consoleinit(void)
7617 {
7618   initlock(&cons.lock, "console");
7619   initlock(&input.lock, "input");
7620 
7621   devsw[CONSOLE].write = consolewrite;
7622   devsw[CONSOLE].read = consoleread;
7623   cons.locking = 1;
7624 
7625   picenable(IRQ_KBD);
7626   ioapicenable(IRQ_KBD, 0);
7627 }
7628 
7629 
7630 
7631 
7632 
7633 
7634 
7635 
7636 
7637 
7638 
7639 
7640 
7641 
7642 
7643 
7644 
7645 
7646 
7647 
7648 
7649 
