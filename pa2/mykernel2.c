/* mykernel2.c: your portion of the kernel (last modified 10/18/09)
 *
 *	Below are procedures that are called by other parts of the kernel.
 *	Your ability to modify the kernel is via these procedures.  You may
 *	modify the bodies of these procedures any way you wish (however,
 *	you cannot change the interfaces).
 * 
 */

#include "aux.h"
#include "sys.h"
#include "mykernel2.h"

//#define TIMERINTERVAL 100000	/* in ticks (tick = 10 msec) */
#define TIMERINTERVAL 1

/*	A sample process table.  You may change this any way you wish.
 */

static struct {
	int valid;		/* is this entry valid: 1 = yes, 0 = no */
	int pid;		/* process id (as provided by kernel) */
    float cpu_uti;      /*the percent of CPU time that the process gets*/
	float cpu_prom;
	float ratio;     /* cpu_uti/cpu_prom */
	int start;     /*start CPU time*/
	int cpu_occ;       /*total occupied CPU time*/
	int flag;
	int nn;
//	int cpu_acc;       /*total CPU time,cputime - start cpu time*/
} proctab[MAXPROCS];

/*queue for FIFO*/
static struct
{
	int valid;
	int pid;
}q[MAXPROCS];
int qptr=0; //queue's pointer
/*queue for LIFO*/
static struct
{
	int valid;
	int pid;
}s[MAXPROCS];
int sptr=0; //stack's pointer
/*queue for ROUNDROBIN*/
static struct
{
	int valid;
	int pid;
}q2[MAXPROCS];
int q2ptr=0;//queue2's pointer

int curindex;  //for ROUNDROBIN and PROPORTIONAL, current index
int curp; //for ROUNDROBIN, current process
int cputime;
double cpu_total;//sum of all processes' cpu fractions
int non_request;

/*	InitSched () is called when kernel starts up.  First, set the
 *	scheduling policy (see sys.h).  Make sure you follow the rules
 *	below on where and how to set it.  Next, initialize all your data
 *	structures (such as the process table).  Finally, set the timer
 *	to interrupt after a specified number of ticks.
 */

void InitSched ()
{
	int i;
	curindex=0;
	curp=0;
	cpu_total=0.0;
	non_request=0;

	/* First, set the scheduling policy.  You should only set it
	 * from within this conditional statement.  While you are working
	 * on this assignment, GetSchedPolicy will return NOSCHEDPOLICY,
	 * and so the condition will be true and you may set the scheduling
	 * policy to whatever you choose (i.e., you may replace ARBITRARY).
	 * After the assignment is over, during the testing phase, we will
	 * have GetSchedPolicy return the policy we wish to test, and so
	 * the condition will be false and SetSchedPolicy will not be
	 * called, thus leaving the policy to whatever we chose to test.
	 */
	if (GetSchedPolicy () == NOSCHEDPOLICY) {	/* leave as is */
		SetSchedPolicy (PROPORTIONAL);		/* set policy here */
	}
		
	/* Initialize all your data structures here */
	for (i = 0; i < MAXPROCS; i++) {
		proctab[i].valid = 0;

		q[i].valid=0;
		s[i].valid=0;
		proctab[i].nn=20;
		proctab[i].cpu_uti=0.0;
		proctab[i].cpu_prom=1.0;
		proctab[i].ratio=20.0;
//		proctab[i].cpu_acc=0;
		proctab[i].flag=0;
		proctab[i].cpu_occ=0;
	}

	/* Set the timer last */
	SetTimer (TIMERINTERVAL);
}


/*	StartingProc (pid) is called by the kernel when the process
 *	identified by pid is starting.  This allows you to record the
 *	arrival of a new process in the process table, and allocate
 *	any resources (if necessary).  Returns 1 if successful, 0 otherwise.
 */

int StartingProc (pid)
	int pid;
{
	/*for FIFO*/
	if(!q[qptr].valid)
	{
		q[qptr].valid=1;
		q[qptr].pid=pid;
		qptr=(qptr+1)%MAXPROCS;
	}
	/*for LIFO*/
	if(!s[sptr].valid)
	{
		s[sptr].valid=1;
		s[sptr].pid=pid;
		sptr++;
		DoSched();
	}
	/*for ROUNDROBIN*/
	if(!q2[q2ptr].valid)
	{
		q2[q2ptr].valid=1;
		q2[q2ptr].pid=pid;
		q2ptr=(q2ptr+1)%MAXPROCS; /*use the circular queue*/
	}

	/*for ARBITRARY AND PROPORTIONAL*/
	int i;
	for (i = 0; i < MAXPROCS; i++) {
		if (! proctab[i].valid) {
			proctab[i].valid = 1;
			proctab[i].pid = pid;
			proctab[i].start=0;

			return (1);
		}
	}

	Printf ("Error in StartingProc: no free table entries\n");
	return (0);
}
			

/*	EndingProc (pid) is called by the kernel when the process
 *	identified by pid is ending.  This allows you to update the
 *	process table accordingly, and deallocate any resources (if
 *	necessary).  Returns 1 if successful, 0 otherwise.
 */


int EndingProc (pid)
	int pid;
{
	int i;

	for(i=0;i<MAXPROCS;i++)
	{
		if(q[i].valid&&q[i].pid==pid)
		{
			q[i].valid=0;
			//qptr--;
		}
	}

	for(i=0;i<MAXPROCS;i++)
	{
		if(s[i].valid&&s[i].pid==pid)
		{
			s[i].valid=0;
			sptr--;
		}
	}

	for(i=0;i<MAXPROCS;i++)
	{
		if(q2[i].valid&&q2[i].pid==pid)
		{
			q2[i].valid=0;
			//q2ptr--;
		}
	}

	for (i = 0; i < MAXPROCS; i++) {
		if (proctab[i].valid && proctab[i].pid == pid) {
			proctab[i].valid = 0;
			
			if(GetSchedPolicy()==PROPORTIONAL)
			{
				proctab[i].flag=0;
				cpu_total = cpu_total - proctab[i].cpu_prom;
			}
			return (1);
		}
	}


	Printf ("Error in EndingProc: can't find process %d\n", pid);
	return (0);
}


/*	SchedProc () is called by kernel when it needs a decision for
 *	which process to run next.  It calls the kernel function
 *	GetSchedPolicy () which will return the current scheduling policy
 *	which was previously set via SetSchedPolicy (policy). SchedProc ()
 *	should return a process id, or 0 if there are no processes to run.
 */

int SchedProc ()
{
	int i;
	int j,n,p;
	float r,acc,temp;

	switch (GetSchedPolicy ()) {

	case ARBITRARY:

		for (i = 0; i < MAXPROCS; i++) {
			if (proctab[i].valid) {
				return (proctab[i].pid);
			}
		}
		break;

	case FIFO:

		/* your code here */
		for(i=0;i<MAXPROCS;i++)
		{
			if(q[i].valid)
			{
				return(q[i].pid);
			}
		}
		break;

	case LIFO:

		/* your code here */
		if(s[sptr-1].valid)
		{
			return(s[sptr-1].pid);
		}

		break;

	case ROUNDROBIN:

		/* your code here */
		for(i=curindex,j=0;j<MAXPROCS;i=(i+1)%MAXPROCS,j++)
		{
			if(q2[i].valid)
			{
				curindex=(i+1)%MAXPROCS;
				curp=q2[i].pid;
				return curp;
			}
		}

		break;

	case PROPORTIONAL:

		/* your code here */
		temp=20;
		for(i=0;i<MAXPROCS;i++)
		{
			if(proctab[i].ratio<temp&&proctab[i].flag&&proctab[i].valid)
			{
				temp=proctab[i].ratio;
				curindex=i;
			}
			else if((proctab[i].ratio==temp)&&(proctab[i].nn<proctab[curindex].nn)&&proctab[i].flag&&proctab[i].valid)
			{
				//temp=proctab[i].ratio;
			    curindex=i;	
			}
		}

		if(proctab[curindex].ratio>1||temp==20)
		{
			for(i=non_request,j=0;j<MAXPROCS;i=(i+1)%MAXPROCS,j++)
					{
						if(proctab[i].valid&&(proctab[i].flag==0))
						{
							non_request=(i+1)%MAXPROCS;
							return(proctab[i].pid);
						}
					}
		}
		for(p=0;p<MAXPROCS;p++)
		{
			if(proctab[curindex].valid)
			{
				proctab[curindex].cpu_occ++;
				return(proctab[curindex].pid);
			}
			else curindex=(curindex+1)%MAXPROCS;
		}
	}
	
	return (0);
}


/*	HandleTimerIntr () is called by the kernel whenever a timer
 *	interrupt occurs.
 */

void HandleTimerIntr ()
{
	SetTimer (TIMERINTERVAL);
	int i,j;
	int acc=0;
	switch (GetSchedPolicy ()) {	/* is policy preemptive? */

	case ROUNDROBIN:		/* ROUNDROBIN is preemptive */
		DoSched();
		break;
	case PROPORTIONAL:		/* PROPORTIONAL is preemptive */
		cputime++; 
		for(i=curindex,j=0;j<MAXPROCS;i=(i+1)%MAXPROCS,j++)
		{
			if(proctab[i].valid)
			{
				acc=(float)cputime-(float)proctab[i].start;
				proctab[i].cpu_uti=(float)proctab[i].cpu_occ/acc; //fraction=cpu_occupy/accumulate
				proctab[i].ratio=proctab[i].cpu_uti/proctab[i].cpu_prom;
			}
		}
		DoSched ();		/* make scheduling decision */
		break;

	default:			/* if non-preemptive, do nothing */
		break;
	}
}

/*	MyRequestCPUrate (pid, m, n) is called by the kernel whenever a process
 *	identified by pid calls RequestCPUrate (m, n).  This is a request for
 *	a fraction m/n of CPU time, effectively running on a CPU that is m/n
 *	of the rate of the actual CPU speed.  m of every n quantums should
 *	be allocated to the calling process.  Both m and n must be greater
 *	than zero, and m must be less than or equal to n.  MyRequestCPUrate
 *	should return 0 if successful, i.e., if such a request can be
 *	satisfied, otherwise it should return -1, i.e., error (including if
 *	m < 1, or n < 1, or m > n).  If MyRequestCPUrate fails, it should
 *	have no effect on scheduling of this or any other process, i.e., as
 *	if it were never called.
 */

int MyRequestCPUrate (pid, m, n)
	int pid;
	int m;
	int n;
{
	/* your code here */
	if(m<1||n<1||m>n)
	{
		return(-1);
	}

	double alloc=(double)m/(double)n;
	for(int i=0;i<MAXPROCS;i++)
	{
		if(proctab[i].pid==pid&&proctab[i].valid)
		{

			if(proctab[i].flag)
			{
				if(cpu_total-proctab[i].cpu_prom+alloc>1.00001)
				{
					return (-1);
				}
				cpu_total=cpu_total-proctab[i].cpu_prom+alloc;
			}
			else
			{
				if((cpu_total+alloc)>1.00001)
				{
					return(-1);
				}
				cpu_total=cpu_total+alloc;
				proctab[i].flag=1;
			}
			proctab[i].nn=n;
			proctab[i].cpu_prom=alloc;
			Printf("cup_total is %f",cpu_total);
			Printf("pid is %d",proctab[i].pid);
			Printf("\n");

		}
	}

	return (0);
}