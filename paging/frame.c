/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[NFRAMES];
int glb_pt[4];
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  STATWORD ps;
  int i;
  disable(ps);
  for (i=0 ; i<NFRAMES ; i++){
		frm_tab[i].fr_status = FRM_UNMAPPED;
	}
  restore(ps);
  return(OK);
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  STATWORD ps;
  int i;
  disable(ps);
  for (i=0;i<NFRAMES;i++) {
    if (frm_tab[i].fr_status == FRM_UNMAPPED) {
      *avail = i;
      frm_tab[i].fr_status = FRM_MAPPED;
      restore(ps);
      return OK;
    }
  }
  int swap_frm;

  switch(grpolicy()){
    case SC:
      if (SC_policy(&swap_frm) == SYSERR){
        restore(ps);
        return SYSERR;
      }
      if (free_frm(swap_frm) == SYSERR){
        restore(ps);
        return SYSERR;
      }
      if (DEBUG == 1)
        kprintf("Replaced frame: %d", FRAME0+swap_frm);
      *avail = swap_frm;
      frm_tab[swap_frm].fr_status = FRM_MAPPED;
      restore(ps);
      return OK;

    case AGING:
      if (AG_policy(&swap_frm) == SYSERR){
          restore(ps);
          return SYSERR;
        }
        if (free_frm(swap_frm) == SYSERR){
          restore(ps);
          return SYSERR;
        }
        if (DEBUG == 1)
          kprintf("Replaced frame:%d", FRAME0 + swap_frm);
        *avail = swap_frm;
        frm_tab[swap_frm].fr_status = FRM_MAPPED;
        restore(ps);
        return OK;

    default:
      break;
  }
  restore(ps);
  return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

  STATWORD ps;
  int store, pg;
  disable(ps);

  frm_tab[i].fr_status = FRM_UNMAPPED;
  unsigned long frm_addr = (frm_tab[i].fr_vpno*NBPG);

  virt_addr_t *viraddr = (virt_addr_t*)&frm_addr;
  unsigned int vp_pt_off = viraddr->pt_offset;
  unsigned int vp_pd_off = viraddr->pd_offset;

  pd_t *pd = proctab[currpid].pdbr + sizeof(pd_t)*vp_pd_off;
  pt_t *pt = (pt_t*)(pd->pd_base*NBPG + vp_pt_off*sizeof(pt_t));

  if (bsm_lookup(frm_tab[i].fr_pid, (frm_tab[i].fr_vpno*NBPG), &store, &pg) == SYSERR){
    restore(ps);
    return SYSERR;
  }
  write_bs((char *)((i+FRAME0)*NBPG), store, pg);
  pt->pt_pres = 0;

  restore(ps);
  return OK;
}

int init_page_dir(int pid) 
{
  STATWORD ps;
  disable(ps);
  int frm_num, i;
  unsigned long frm_add;
  pd_t *pt;
  get_frm(&frm_num);
  frm_tab[frm_num].fr_pid = pid;
	frm_tab[frm_num].fr_type = FR_DIR;

  frm_add = (FRAME0 + frm_num) * NBPG;
  proctab[pid].pdbr = frm_add;
  pt = (pd_t*)frm_add;

  for(i=0;i<NFRAMES;i++) {
    if (i<4) {
      pt->pd_pres = 1;
      pt->pd_base = glb_pt[i];
    } else {
      pt->pd_pres = 0;
      pt->pd_base = 0;
    }
    pt->pd_write = 1;
    pt->pd_user = 0;
    pt->pd_pwt = 0;
    pt->pd_pcd = 0;
    pt->pd_acc = 0;
    pt->pd_fmb = 0;
    pt->pd_mbz = 0;
    pt->pd_global = 0;
    pt->pd_avail = 0;
    pt++;
  }
  restore(ps);
  return frm_num;

}

void init_glb_pgtab()
{
  STATWORD ps;
  disable(ps);
	int frm_num,gt,i;
	unsigned int frm_add;
	pt_t *pg;
	
	for(gt=0;gt<4;gt++) {
		get_frm(&frm_num);
		frm_tab[frm_num].fr_pid = -1;
		frm_tab[frm_num].fr_type = FR_TBL;

		glb_pt[gt] = FRAME0 + frm_num;

		frm_add = (FRAME0 + frm_num) * NBPG;
		pg = (pt_t*)frm_add;

		for(i=0;i<NFRAMES;i++) {
			pg->pt_pres = 1;
			pg->pt_write = 1;
			pg->pt_user = 0;
			pg->pt_pwt = 0;
			pg->pt_pcd = 0;
			pg->pt_acc = 0;
			pg->pt_dirty = 0;
			pg->pt_mbz = 0;
			pg->pt_global = 1;
			pg->pt_avail = 0;
			pg->pt_base = gt*1024 + i;
      pg++;
		}	
	}
  restore(ps);
}

int write_dirfrm_bs(int pid){
  STATWORD ps;
  int i;
  disable(ps);
  for (i=0;i<NFRAMES;i++) {
    if (frm_tab[i].fr_pid == pid && frm_tab[i].fr_type == FR_PAGE) {

      unsigned long frm_addr = (frm_tab[i].fr_vpno*NBPG);
      virt_addr_t *viraddr = (virt_addr_t*)&frm_addr;
      unsigned int vp_pt_off = viraddr->pt_offset;
      unsigned int vp_pd_off = viraddr->pd_offset;

      pd_t *pd = proctab[currpid].pdbr + sizeof(pd_t)*vp_pd_off;
      pt_t *pt = (pt_t*)(pd->pd_base*NBPG + vp_pt_off*sizeof(pt_t));
      if (pt->pt_dirty==1){
        int store, pg;
        if (bsm_lookup(pid, (frm_tab[i].fr_vpno*NBPG), &store, &pg) == SYSERR){
          restore(ps);
          return SYSERR;
        }
        write_bs((char *)((i+FRAME0)*NBPG), store, pg);
        frm_tab[i].fr_status = FRM_UNMAPPED;
        pt->pt_pres = 0;
      }
    }
  }
  restore(ps);
  return OK;
}

int SC_policy(int* swap_frm){
  STATWORD ps;
  int i = SC_frm;
  int frm_num;
  disable(ps);

  while(1){
    frm_num = i%NFRAMES;
    if (frm_tab[frm_num].fr_type == FR_PAGE) {

      unsigned long frm_addr = (frm_tab[frm_num].fr_vpno*NBPG);
      virt_addr_t *viraddr = (virt_addr_t*)&frm_addr;
      unsigned int vp_pt_off = viraddr->pt_offset;
      unsigned int vp_pd_off = viraddr->pd_offset;

      pd_t *pd = proctab[currpid].pdbr + sizeof(pd_t)*vp_pd_off;
      pt_t *pt = (pt_t*)(pd->pd_base*NBPG + vp_pt_off*sizeof(pt_t));

      if (pt->pt_acc == 1){
        pt->pt_acc = 0;
      } else if (pt->pt_acc == 0) {
        *swap_frm = frm_num;
        SC_frm = frm_num+1;
        restore(ps);
        return OK;
      }
    }
    i++;
  }
}
void init_frm_queue(){
  frm_qhead.age = -1;
  frm_qhead.frm = -1;
  frm_qhead.next = NULL;
  frm_qhead.prev = NULL;
}

int frm_enqueu(int frm_num, int age){
  STATWORD ps;
  disable(ps);
  if (frm_rear == NFRAMES - 1){
    restore(ps);
    return SYSERR;
  } else {
    if (frm_front == -1){
      frm_front = 0;
    }
      
    frm_rear++;
    struct fr_queue *insertnode = (struct fr_queue*)getmem(sizeof(struct fr_queue));
    insertnode->age = age;
    insertnode->frm = frm_num;
    insertnode->next = NULL;
    struct fr_queue *curr = &frm_qhead;
    while(curr->next != NULL){
      curr = curr->next;
    }
    curr->next = insertnode;
    insertnode->prev = curr;
    restore(ps);
    return OK;
  }

}

int AG_policy(int* swap_frm){
  STATWORD ps;
  disable(ps);
  int min_age = 260;
  kprintf("frm_num qhead- %d", frm_qhead.frm);
  struct fr_queue *curr = frm_qhead.next;
  while (1){
    curr->age = curr->age/2;

    unsigned long frm_addr = (frm_tab[curr->frm].fr_vpno*NBPG);

    virt_addr_t *viraddr = (virt_addr_t*)&frm_addr;
    unsigned int vp_pt_off = viraddr->pt_offset;
    unsigned int vp_pd_off = viraddr->pd_offset;

    pd_t *pd = proctab[currpid].pdbr + sizeof(pd_t)*vp_pd_off;
    pt_t *pt = (pt_t*)(pd->pd_base*NBPG + vp_pt_off*sizeof(pt_t));

    if (pt->pt_acc == 1){
      pt->pt_acc = 0;
      if (curr->age + 128 >255){
        curr->age = 255;
      } else{
        curr->age += 128;
      }
    }
    
    if (curr->age < min_age){
      min_age = curr->age;
    }
    if (curr->next == NULL){
      break;
    }
  }

  //Remove the frame with lowest age

  struct fr_queue *current = frm_qhead.next;
  while(current->age != min_age){
    current = current->next;
  }
  current->prev->next = current->next;
  if (current->next != NULL){
    current->next->prev = current->prev;
  }
  *swap_frm = current->frm;
  freemem(current, sizeof(struct fr_queue));
  frm_front++;
  if (frm_front > frm_rear)
    frm_front = frm_rear = -1;
  restore(ps);
  return OK;

}