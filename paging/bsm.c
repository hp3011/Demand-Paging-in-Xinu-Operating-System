/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
struct bs_map_t bsm_tab[8];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    STATWORD ps;
    int i;
    disable(ps);
    for (i=0 ; i<8 ; i++) {
        
		bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_pid = -1;
        bsm_tab[i].bs_vpno = -1;
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_sem = 0;
        bsm_tab[i].prev = NULL;
        bsm_tab[i].bs_priv = 0;

        struct bs_map_t *tailnode = (struct bs_map_t*)getmem(sizeof(struct bs_map_t));
        bsm_tab[i].next = tailnode;

        tailnode->bs_status = BSM_UNMAPPED;
        tailnode->bs_pid = -1;
        tailnode->bs_vpno = -1;
        tailnode->bs_npages = 0;
        tailnode->bs_sem = 0;
        tailnode->prev = &bsm_tab[i];
        tailnode->next = NULL;
        tailnode->bs_priv = 0;

	}
    restore(ps);
    return(OK);
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD ps;
    int i;
    disable(ps);
    for (i=0 ; i<8 ; i++) {
        if (bsm_tab[i].bs_status == BSM_UNMAPPED){
            *avail = i;
            restore(ps);
            return(OK);
        }
	}
    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    return(OK);
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    int i;
    STATWORD ps;
    disable(ps);
    for (i=0;i<8;i++) {
       
        if ((bsm_tab[i].bs_pid == pid) && ((unsigned)vaddr/NBPG>=bsm_tab[i].bs_vpno) && (((unsigned)vaddr/NBPG) - bsm_tab[i].bs_vpno < bsm_tab[i].bs_npages)){
            *store = i;
            *pageth = ((unsigned)vaddr/NBPG) - bsm_tab[i].bs_vpno;
            restore(ps);
            return OK;
        } else {
            struct bs_map_t *curr = bsm_tab[i].next;
            while ((curr->next != NULL)){
                if ((curr->bs_pid == pid) && ((unsigned)vaddr/NBPG>=curr->bs_vpno) && (((unsigned)vaddr/NBPG) - curr->bs_vpno < curr->bs_npages)) {
                    *store = i;
                    *pageth = ((unsigned)vaddr/NBPG) - curr->bs_vpno;
                    restore(ps);
                    return OK;
                }
                curr = curr->next;
            }
        }
    }
    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD ps;
    disable(ps);

    if (bsm_tab[source].bs_status == BSM_UNMAPPED) {
        bsm_tab[source].bs_status = BSM_MAPPED;
        bsm_tab[source].bs_pid = pid;
        bsm_tab[source].bs_vpno = vpno;
        bsm_tab[source].bs_npages = npages;
        
    } else if ( (bsm_tab[source].bs_status == BSM_MAPPED) && (bsm_tab[source].bs_priv == 0) ) {
        if (npages>bsm_tab[source].bs_npages) {
            restore(ps);
            return SYSERR;
        }
        struct bs_map_t *newnode = (struct bs_map_t*)getmem(sizeof(bs_map_t));
        newnode->next = bsm_tab[source].next;
        newnode->prev = newnode->next->prev;
        bsm_tab[source].next = newnode;

        newnode->bs_status = BSM_MAPPED;
        newnode->bs_pid = pid;
        newnode->bs_vpno = vpno;
        newnode->bs_npages = npages;
    } else if ( (bsm_tab[source].bs_status == BSM_MAPPED) && (bsm_tab[source].bs_priv == 1) ) {
        restore(ps);
        return SYSERR;
    }

    proctab[currpid].vhpno = vpno;
	proctab[currpid].store = source;

    restore(ps);
    return(OK);
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno)
{
    STATWORD ps;
    disable(ps);
    int store, pageth;
    bsm_lookup(pid,vpno*NBPG, &store, &pageth);


    if ((bsm_tab[store].bs_pid == pid) && (vpno - bsm_tab[store].bs_vpno < bsm_tab[store].bs_npages)){
        bsm_tab[store].bs_status = BSM_UNMAPPED;
        bsm_tab[store].bs_pid = -1;
        bsm_tab[store].bs_vpno = -1;
        bsm_tab[store].bs_npages = 0;
        bsm_tab[store].bs_sem = 0;
        bsm_tab[store].prev = NULL;
        bsm_tab[store].bs_priv = 0;


        struct bs_map_t *curr = bsm_tab[store].next;
        struct bs_map_t *prev = curr->prev;

        if (curr->next != NULL) {
            curr->next->prev = curr->prev;

            bsm_tab[store].bs_status = curr->bs_status;
            bsm_tab[store].bs_pid = curr->bs_pid;
            bsm_tab[store].bs_vpno = curr->bs_vpno;
            bsm_tab[store].bs_npages = curr->bs_npages;
            bsm_tab[store].bs_sem = 0;
            bsm_tab[store].prev = NULL;
            bsm_tab[store].bs_priv = 0;
            bsm_tab[store].next = curr->next;

            freemem(curr, sizeof(bs_map_t));
        }
        restore(ps);
        return OK;
    }else {
        struct bs_map_t *curr = bsm_tab[store].next;
        while (curr->next != NULL) {

            if ((curr->bs_pid == pid) && (vpno - curr->bs_vpno < curr->bs_npages)) {
                curr->next->prev = curr->prev;
                curr->prev->next = curr->next;
                freemem(curr, sizeof(bs_map_t));
                restore(ps);
                return OK;
            }
            curr = curr->next;
        }
    }
    restore(ps);
    return SYSERR;

}


