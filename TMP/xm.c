/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);
  
  //Checking for invalid virtpage, backing store id, npages.
  if ((virtpage<NBPG) || (source<0) || (source>7) || (npages<0) || (npages>256)) {
    restore(ps);
    return SYSERR;
  }

  //can request a new mapping?
  if (get_bs(source,npages) == SYSERR){
    restore(ps);
    return SYSERR;
  }

  //If Yes, then bsm_map
  if (bsm_map(currpid,virtpage,source,npages) == SYSERR){
    restore(ps);
    return SYSERR;
  }
  restore(ps);
  return OK;
  
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  //kprintf("xmunmap - called by pid : %d\n", currpid);
  STATWORD ps;
  disable(ps);
  write_dirfrm_bs(currpid);
  if (bsm_unmap(currpid,virtpage) == SYSERR) {
    kprintf("xmunmap - Error bsm_unmap");
    restore(ps);
    return SYSERR;
  }
  
  restore(ps);
  return OK;
}
