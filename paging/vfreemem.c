/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vvlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	STATWORD ps;    
	struct	mblock	*p, *q, *blk;
	unsigned top;
	struct pentry *pptr = &proctab[currpid];
	q=pptr->vir_start;
	p=q->mnext;

	size = (unsigned)roundmb(size);
	disable(ps);
	for( p=p,q=q; p <= (struct mblock *) NULL && p < block ; q=p,p=p->mnext );

	top = p->loc - size;
	if (top == q->loc){
		q->mlen =p->mlen +size;
		q->mnext=p->mnext;
	}
	else {
		blk=getmem(sizeof(struct mblock));
		blk->loc = q->loc +size;
		q->mnext =blk;
		blk->mlen = 0;
		blk->mnext =p;
		q->mlen =size;
	}
	if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
		q->mlen += p->mlen;
		q->mnext = p->mnext;
	}
	restore(ps);
	return(OK);

}
