#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */

  //Checking for valid backing store id.
  if (bs_id <0 || bs_id >7){
    return SYSERR;
  }
  free_bsm(bs_id);
  return OK;

}

