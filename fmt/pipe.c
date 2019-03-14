6100 #include "types.h"
6101 #include "defs.h"
6102 #include "param.h"
6103 #include "mmu.h"
6104 #include "proc.h"
6105 #include "fs.h"
6106 #include "file.h"
6107 #include "spinlock.h"
6108 
6109 #define PIPESIZE 512
6110 
6111 struct pipe {
6112   struct spinlock lock;
6113   char data[PIPESIZE];
6114   uint nread;     // number of bytes read
6115   uint nwrite;    // number of bytes written
6116   int readopen;   // read fd is still open
6117   int writeopen;  // write fd is still open
6118 };
6119 
6120 int
6121 pipealloc(struct file **f0, struct file **f1)
6122 {
6123   struct pipe *p;
6124 
6125   p = 0;
6126   *f0 = *f1 = 0;
6127   if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
6128     goto bad;
6129   if((p = (struct pipe*)kalloc()) == 0)
6130     goto bad;
6131   p->readopen = 1;
6132   p->writeopen = 1;
6133   p->nwrite = 0;
6134   p->nread = 0;
6135   initlock(&p->lock, "pipe");
6136   (*f0)->type = FD_PIPE;
6137   (*f0)->readable = 1;
6138   (*f0)->writable = 0;
6139   (*f0)->pipe = p;
6140   (*f1)->type = FD_PIPE;
6141   (*f1)->readable = 0;
6142   (*f1)->writable = 1;
6143   (*f1)->pipe = p;
6144   return 0;
6145 
6146 
6147 
6148 
6149 
6150  bad:
6151   if(p)
6152     kfree((char*)p);
6153   if(*f0)
6154     fileclose(*f0);
6155   if(*f1)
6156     fileclose(*f1);
6157   return -1;
6158 }
6159 
6160 void
6161 pipeclose(struct pipe *p, int writable)
6162 {
6163   acquire(&p->lock);
6164   if(writable){
6165     p->writeopen = 0;
6166     wakeup(&p->nread);
6167   } else {
6168     p->readopen = 0;
6169     wakeup(&p->nwrite);
6170   }
6171   if(p->readopen == 0 && p->writeopen == 0){
6172     release(&p->lock);
6173     kfree((char*)p);
6174   } else
6175     release(&p->lock);
6176 }
6177 
6178 
6179 int
6180 pipewrite(struct pipe *p, char *addr, int n)
6181 {
6182   int i;
6183 
6184   acquire(&p->lock);
6185   for(i = 0; i < n; i++){
6186     while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
6187       if(p->readopen == 0 || proc->killed){
6188         release(&p->lock);
6189         return -1;
6190       }
6191       wakeup(&p->nread);
6192       sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
6193     }
6194     p->data[p->nwrite++ % PIPESIZE] = addr[i];
6195   }
6196   wakeup(&p->nread);  //DOC: pipewrite-wakeup1
6197   release(&p->lock);
6198   return n;
6199 }
6200 int
6201 piperead(struct pipe *p, char *addr, int n)
6202 {
6203   int i;
6204 
6205   acquire(&p->lock);
6206   while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
6207     if(proc->killed){
6208       release(&p->lock);
6209       return -1;
6210     }
6211     sleep(&p->nread, &p->lock); //DOC: piperead-sleep
6212   }
6213   for(i = 0; i < n; i++){  //DOC: piperead-copy
6214     if(p->nread == p->nwrite)
6215       break;
6216     addr[i] = p->data[p->nread++ % PIPESIZE];
6217   }
6218   wakeup(&p->nwrite);  //DOC: piperead-wakeup
6219   release(&p->lock);
6220   return i;
6221 }
6222 
6223 
6224 
6225 
6226 
6227 
6228 
6229 
6230 
6231 
6232 
6233 
6234 
6235 
6236 
6237 
6238 
6239 
6240 
6241 
6242 
6243 
6244 
6245 
6246 
6247 
6248 
6249 
