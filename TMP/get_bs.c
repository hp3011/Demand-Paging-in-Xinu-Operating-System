#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
struct bs_map_t bsm_tab[8];

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */

  //Checking for invalid npages and bs_id.
  STATWORD ps;
  disable(ps);
  if ((npages == 0 || npages >256) || (bs_id<0 || bs_id>7)) {
    restore(ps);
    return SYSERR;
  }

  //Checking if the backing store is private.
  if (bsm_tab[bs_id].bs_priv == 1){
    return SYSERR;
  }

  //If the backing store is shared then return shared npages
  if (bsm_tab[bs_id].bs_status == BSM_MAPPED){
    return bsm_tab[bs_id].bs_npages;
  }

  return npages;

}


