3750 // On-disk file system format.
3751 // Both the kernel and user programs use this header file.
3752 
3753 // Block 0 is unused.
3754 // Block 1 is super block.
3755 // Blocks 2 through sb.ninodes/IPB hold inodes.
3756 // Then free bitmap blocks holding sb.size bits.
3757 // Then sb.nblocks data blocks.
3758 // Then sb.nlog log blocks.
3759 
3760 #define ROOTINO 1  // root i-number
3761 #define BSIZE 512  // block size
3762 
3763 // File system super block
3764 struct superblock {
3765   uint size;         // Size of file system image (blocks)
3766   uint nblocks;      // Number of data blocks
3767   uint ninodes;      // Number of inodes.
3768   uint nlog;         // Number of log blocks
3769 };
3770 
3771 #define NDIRECT 12
3772 #define NINDIRECT (BSIZE / sizeof(uint))
3773 #define MAXFILE (NDIRECT + NINDIRECT)
3774 
3775 // On-disk inode structure
3776 struct dinode {
3777   short type;           // File type
3778   short major;          // Major device number (T_DEV only)
3779   short minor;          // Minor device number (T_DEV only)
3780   short nlink;          // Number of links to inode in file system
3781   uint size;            // Size of file (bytes)
3782   uint addrs[NDIRECT+1];   // Data block addresses
3783 };
3784 
3785 // Inodes per block.
3786 #define IPB           (BSIZE / sizeof(struct dinode))
3787 
3788 // Block containing inode i
3789 #define IBLOCK(i)     ((i) / IPB + 2)
3790 
3791 // Bitmap bits per block
3792 #define BPB           (BSIZE*8)
3793 
3794 // Block containing bit for block b
3795 #define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)
3796 
3797 // Directory is a file containing a sequence of dirent structures.
3798 #define DIRSIZ 14
3799 
3800 struct dirent {
3801   ushort inum;
3802   char name[DIRSIZ];
3803 };
3804 
3805 
3806 
3807 
3808 
3809 
3810 
3811 
3812 
3813 
3814 
3815 
3816 
3817 
3818 
3819 
3820 
3821 
3822 
3823 
3824 
3825 
3826 
3827 
3828 
3829 
3830 
3831 
3832 
3833 
3834 
3835 
3836 
3837 
3838 
3839 
3840 
3841 
3842 
3843 
3844 
3845 
3846 
3847 
3848 
3849 
