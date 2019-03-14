3600 struct buf {
3601   int flags;
3602   uint dev;
3603   uint sector;
3604   struct buf *prev; // LRU cache list
3605   struct buf *next;
3606   struct buf *qnext; // disk queue
3607   uchar data[512];
3608 };
3609 #define B_BUSY  0x1  // buffer is locked by some process
3610 #define B_VALID 0x2  // buffer has been read from disk
3611 #define B_DIRTY 0x4  // buffer needs to be written to disk
3612 
3613 
3614 
3615 
3616 
3617 
3618 
3619 
3620 
3621 
3622 
3623 
3624 
3625 
3626 
3627 
3628 
3629 
3630 
3631 
3632 
3633 
3634 
3635 
3636 
3637 
3638 
3639 
3640 
3641 
3642 
3643 
3644 
3645 
3646 
3647 
3648 
3649 
