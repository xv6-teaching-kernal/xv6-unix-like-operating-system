7250 #include "types.h"
7251 #include "x86.h"
7252 #include "defs.h"
7253 #include "kbd.h"
7254 
7255 int
7256 kbdgetc(void)
7257 {
7258   static uint shift;
7259   static uchar *charcode[4] = {
7260     normalmap, shiftmap, ctlmap, ctlmap
7261   };
7262   uint st, data, c;
7263 
7264   st = inb(KBSTATP);
7265   if((st & KBS_DIB) == 0)
7266     return -1;
7267   data = inb(KBDATAP);
7268 
7269   if(data == 0xE0){
7270     shift |= E0ESC;
7271     return 0;
7272   } else if(data & 0x80){
7273     // Key released
7274     data = (shift & E0ESC ? data : data & 0x7F);
7275     shift &= ~(shiftcode[data] | E0ESC);
7276     return 0;
7277   } else if(shift & E0ESC){
7278     // Last character was an E0 escape; or with 0x80
7279     data |= 0x80;
7280     shift &= ~E0ESC;
7281   }
7282 
7283   shift |= shiftcode[data];
7284   shift ^= togglecode[data];
7285   c = charcode[shift & (CTL | SHIFT)][data];
7286   if(shift & CAPSLOCK){
7287     if('a' <= c && c <= 'z')
7288       c += 'A' - 'a';
7289     else if('A' <= c && c <= 'Z')
7290       c += 'a' - 'A';
7291   }
7292   return c;
7293 }
7294 
7295 void
7296 kbdintr(void)
7297 {
7298   consoleintr(kbdgetc);
7299 }
