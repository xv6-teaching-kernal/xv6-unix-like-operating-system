6250 #include "types.h"
6251 #include "x86.h"
6252 
6253 void*
6254 memset(void *dst, int c, uint n)
6255 {
6256   if ((int)dst%4 == 0 && n%4 == 0){
6257     c &= 0xFF;
6258     stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
6259   } else
6260     stosb(dst, c, n);
6261   return dst;
6262 }
6263 
6264 int
6265 memcmp(const void *v1, const void *v2, uint n)
6266 {
6267   const uchar *s1, *s2;
6268 
6269   s1 = v1;
6270   s2 = v2;
6271   while(n-- > 0){
6272     if(*s1 != *s2)
6273       return *s1 - *s2;
6274     s1++, s2++;
6275   }
6276 
6277   return 0;
6278 }
6279 
6280 void*
6281 memmove(void *dst, const void *src, uint n)
6282 {
6283   const char *s;
6284   char *d;
6285 
6286   s = src;
6287   d = dst;
6288   if(s < d && s + n > d){
6289     s += n;
6290     d += n;
6291     while(n-- > 0)
6292       *--d = *--s;
6293   } else
6294     while(n-- > 0)
6295       *d++ = *s++;
6296 
6297   return dst;
6298 }
6299 
6300 // memcpy exists to placate GCC.  Use memmove.
6301 void*
6302 memcpy(void *dst, const void *src, uint n)
6303 {
6304   return memmove(dst, src, n);
6305 }
6306 
6307 int
6308 strncmp(const char *p, const char *q, uint n)
6309 {
6310   while(n > 0 && *p && *p == *q)
6311     n--, p++, q++;
6312   if(n == 0)
6313     return 0;
6314   return (uchar)*p - (uchar)*q;
6315 }
6316 
6317 char*
6318 strncpy(char *s, const char *t, int n)
6319 {
6320   char *os;
6321 
6322   os = s;
6323   while(n-- > 0 && (*s++ = *t++) != 0)
6324     ;
6325   while(n-- > 0)
6326     *s++ = 0;
6327   return os;
6328 }
6329 
6330 // Like strncpy but guaranteed to NUL-terminate.
6331 char*
6332 safestrcpy(char *s, const char *t, int n)
6333 {
6334   char *os;
6335 
6336   os = s;
6337   if(n <= 0)
6338     return os;
6339   while(--n > 0 && (*s++ = *t++) != 0)
6340     ;
6341   *s = 0;
6342   return os;
6343 }
6344 
6345 
6346 
6347 
6348 
6349 
6350 int
6351 strlen(const char *s)
6352 {
6353   int n;
6354 
6355   for(n = 0; s[n]; n++)
6356     ;
6357   return n;
6358 }
6359 
6360 
6361 
6362 
6363 
6364 
6365 
6366 
6367 
6368 
6369 
6370 
6371 
6372 
6373 
6374 
6375 
6376 
6377 
6378 
6379 
6380 
6381 
6382 
6383 
6384 
6385 
6386 
6387 
6388 
6389 
6390 
6391 
6392 
6393 
6394 
6395 
6396 
6397 
6398 
6399 
