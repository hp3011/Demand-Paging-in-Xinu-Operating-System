## Xinu-Demand-Paging
### Introduction
Demand paging is a method of mapping a large address space into a relatively small amount of physical memory. It allows a program to use an address space that is larger than the physical memory, and access non-contiguous sections of the physical memory in a contiguous way. Demand paging is accomplished by using a "backing store" (usually disk) to hold pages of memory that are not currently in use.

### Implemented System Calls
##### SYSCALL xmmap (int virtpage, bsd_t source, int npages)
Much like its Unix counterpart (see man mmap), it maps a source file ("backing store" here) of size npages pages to the virtual page virtpage. A process may call this multiple times to map data structures, code, etc.

##### SYSCALL xmunmap (int virtpage)
This call, like munmap, removes a virtual memory mapping. See man munmap for the details of the Unix call.

##### SYSCALL vcreate (int *procaddr, int ssize, int hsize, int priority, char *name, int nargs, long args)
This call creates a new Xinu process. The difference from create() is that the process' heap will be private and exist in its virtual memory. The size of the heap (in number of pages) is specified by the user through hsize.

##### WORD *vgetmem (int nbytes)
Much like getmem(), vgetmem() allocates the desired amount of memory if possible. The difference is that vgetmem() will get the memory from a process' private heap located in virtual memory. getmem() still allocates memory from the regular Xinu kernel heap.

##### SYSCALL srpolicy (int policy)
This function sets the page replacement policy to Second-Chance (SC) or Aging (AGING). 

##### SYSCALL vfreemem (block_ptr, int size_in_bytes)
vfreemem() takes two parameters and returns OK or SYSERR. The two parameters are similar to those of the original freemem() in Xinu. The type of the first parameter block_ptr depends on your own implementation.

### Memory Layout
Since this version of Xinu does not have file system support, we emulate the backing store with physical memory. In particular, we use the following memory layout:
<p align="center">
<img src="https://user-images.githubusercontent.com/60016007/99140747-86eedd00-2612-11eb-8e10-0234505dbd05.PNG"/>
</p>
A Xinu instance has 16 MB (4096 pages) of real memory in total. The top 8MB real memory are considered as backing stores. Therefore we have 8 backing stores and each backing store maps up to 256 pages (each page is 4K size).

Xinu code compiles to about 100KB, or 25 pages. There is an area of memory from page 160 through the end of page 405 that cannot be used (this is referred to as the "HOLE" in initialize.c). Free frames are placed in 1024 through 4095, giving 3072 frames.The frames are used to store resident pages, page directories, and page tables. The remaining free memory below page 4096 is used for Xinu's kernel heap (organized as a freelist). getmem() and getstk() obtains memory from this area (from the bottom and top, respectively).

All memory below page 4096 is global. That is, it is usable and visible by all processes and accessible by simply using actual physical addresses. As a result, the first four page tables for every process are always the same, and therefore are shared.Memory at page 4096 and above constitute a process' virtual memory. This address space is private and visible only to the process which owns it. Note that the process' private heap and stack are located somewhere in this area.

### Page Replacement Policies
Two page replacement algorithms have been implemented
1. Second Chance page replacement (SC)
SC is implemented using a circular queue. When a page replacement occurs, SC first looks at the current position in the queue (current position starts from the head of the queue), checks to see whether its reference bit is set (i.e., pt_acc = 1). If it is not set, the page is swapped out. Otherwise, the reference bit is cleared, the current position moves to the next page and this process is repeated. If all the pages have their reference bits set, on the second encounter, the page will be swapped out, as it now has its reference bit cleared.
2. Aging page replacement (AGING)
For AGING, when a frame is allocated for a page, you insert the frame into a FIFO queue. When a page replacement occurs, we first decrease by half (= one bit shift to the right) the age of each page in the FIFO queue. If a page has been accessed (i.e., pt_acc = 1), you add 128 to its age (255 is the maximum age). Then, a page with the youngest age is replaced first. If more than one page have a common smallest value, a page that is closest to FIFO queue header, i.e. the oldest page among them in memory, is used for replacement.
