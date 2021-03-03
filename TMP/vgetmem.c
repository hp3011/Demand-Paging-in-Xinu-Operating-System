/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
   	STATWORD ps;    
	struct	mblock	*p, *q, *leftover;
	struct pentry *pptr =&proctab[currpid];
	q = pptr->vir_start;
	p = q->mnext;

	disable(ps);
	if (nbytes==0 || p== (struct mblock *) NULL) {
		restore(ps);
		return( (WORD *)SYSERR);
	}

	nbytes = (unsigned int) roundmb(nbytes);
	
	for (q=q,p=p ; p != (struct mblock *) NULL ; q=p,p=p->mnext){
		if ( q->mlen >= nbytes ) {
			leftover =getmem(sizeof(struct mblock));
			q->mnext = leftover;
			leftover->mnext =p;
			leftover->mlen = q->mlen- nbytes;
			leftover->loc = q->loc + nbytes;
			q->mlen =0; 
			restore(ps);
			return((WORD *)(q->loc));
		}
	}
			
	restore(ps);
	return( (WORD *)SYSERR );

}
