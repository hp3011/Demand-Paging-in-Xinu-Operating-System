/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);

	int pid, store;

	pid = create(procaddr,ssize,priority,name,nargs,args);
	struct pentry *pptr=&proctab[pid];

	if (get_bsm(&store) == SYSERR) {
		restore(ps);
		return(SYSERR);
	}
	if (bsm_map(pid,4096,store,hsize) == SYSERR) {
		restore(ps);
		return(SYSERR);
	}

	pptr->store = store;
	pptr->vhpno = 4096;
	pptr->vhpnpages = hsize;
	bsm_tab[store].bs_priv = 1; 

	pptr->vir_start =getmem(sizeof(struct mblock));
	pptr->vir_start->mlen =hsize*NBPG;
	pptr->vir_start->loc = NBPG*4096;

	pptr->vir_end =getmem(sizeof(struct mblock));
	pptr->vir_end->mlen = 0;
	pptr->vir_end->loc =(hsize+4096)*NBPG;
	pptr->vir_start->mnext = pptr->vir_end;
	pptr->vir_end->mnext=NULL;

	restore(ps);
	return (pid);
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}


