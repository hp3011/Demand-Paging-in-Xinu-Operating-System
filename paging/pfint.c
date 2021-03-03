/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  STATWORD ps;
  disable(ps);

  //Reading faulted address a
  unsigned long a = read_cr2();

  virt_addr_t *a_viradd = (virt_addr_t*)&a;
  unsigned int vp_pt_off = a_viradd->pt_offset;
  unsigned int vp_pd_off = a_viradd->pd_offset;

  pd_t *pd = proctab[currpid].pdbr + sizeof(pd_t)*vp_pd_off;

  //If Page Directory not present
  if (pd->pd_pres==0) {
    int frm_num,i; 
    if (get_frm(&frm_num) == SYSERR) {
      restore(ps);
      return SYSERR;
    }
    frm_tab[frm_num].fr_pid = currpid;
		frm_tab[frm_num].fr_type = FR_TBL;
    frm_tab[frm_num].fr_vpno = a/NBPG;

    unsigned int frm_add = (FRAME0 + frm_num) * NBPG;

    //intialise the the page entry 1024
    pt_t *pg = (pt_t*)frm_add;
    for(i=0;i<NFRAMES;i++) {
			pg += (i*sizeof(pt_t));
			pg->pt_pres = 0;
			pg->pt_write = 0;
			pg->pt_user = 0;
			pg->pt_pwt = 0;
			pg->pt_pcd = 0;
			pg->pt_acc = 0;
			pg->pt_dirty = 0;
			pg->pt_mbz = 0;
			pg->pt_global = 0;
			pg->pt_avail = 0;
			pg->pt_base = 0;
		}
    pd->pd_pres = 1;
    pd->pd_write = 1;
    pd->pd_user = 0;
    pd->pd_pwt = 0;
    pd->pd_pcd = 0;
    pd->pd_acc = 0;
    pd->pd_fmb = 0;
    pd->pd_mbz = 0;
    pd->pd_global = 0;
    pd->pd_avail = 0;
    pd->pd_base = FRAME0 + frm_num;
  }
  
  //Page table is not present
  pt_t *pt = (pt_t*)(pd->pd_base*NBPG + vp_pt_off*sizeof(pt_t));

  if (pt->pt_pres==0) {
    int frm_num,store,pageth;
    if (get_frm(&frm_num) == SYSERR) {
      restore(ps);
      return SYSERR;
    }
    frm_tab[frm_num].fr_pid = currpid;
    frm_tab[frm_num].fr_type = FR_PAGE;
    frm_tab[frm_num].fr_vpno = a/NBPG;

    if (bsm_lookup(currpid,a,&store,&pageth)==SYSERR) {
      restore(ps);
      return SYSERR;
    }

    read_bs((char*)((FRAME0+frm_num)*NBPG),store,pageth);

    pt->pt_pres = 1;
		pt->pt_write = 1;
		pt->pt_base = FRAME0 + frm_num;
    frm_tab[pd->pd_base - FRAME0].fr_refcnt++;

  }
  write_cr3(proctab[currpid].pdbr);
  restore(ps);
  return OK;
}


