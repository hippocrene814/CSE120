/*	User-level thread system
 *
 */

#include <setjmp.h>
#include <string.h>

#include "aux.h"
#include "umix.h"
#include "mythreads.h"

static int MyInitThreadsCalled = 0;	/* 1 if MyInitThreads called, else 0 */
static int currentRunningThread;
static int lastAssignedThread;/*for spawn*/
static int previousThread;/*for yield*/
static int activeThreadCount;
//static int returnThread;

int threadQueue[MAXTHREADS-1];

static struct thread {			/* thread table */
	int valid;			/* 1 if entry is valid, else 0 */
	jmp_buf env;			/* current context */
	jmp_buf initialEnv;

	void (*threadFunc)(); /*function*/
	int funcParam;   /*parameter*/
} thread[MAXTHREADS];

#define STACKSIZE	65536		/* maximum size of thread stack */

/*	MyInitThreads () initializes the thread package.  Must be the first
 *	function called by any user program that uses the thread package.
 */

void MyInitThreads ()
{
	int i;

	if (MyInitThreadsCalled) {                /* run only once */
		Printf ("InitThreads: should be called only once\n");
		Exit ();
	}

	for (i = 0; i < MAXTHREADS; i++) {	/* initialize thread table */
		thread[i].valid = 0;
	}

	for (i = 0; i < MAXTHREADS-1; i++) {	/* initialize thread queue */
		threadQueue[i]=-1;
	}

	thread[0].valid = 1;			/* initialize thread 0 */

	MyInitThreadsCalled = 1;

	currentRunningThread=0;
	lastAssignedThread=0;//for spawn
	activeThreadCount=1;
	previousThread=-1;//for yield

	for(i=0;i<MAXTHREADS;i++)
	{
		char s[i*STACKSIZE+1];
		if(setjmp(thread[i].initialEnv)!=0) //original env
		{
			(*(thread[currentRunningThread].threadFunc)) (thread[currentRunningThread].funcParam);
			MyExitThread();
		}
	}
}

/*	MySpawnThread (func, param) spawns a new thread to execute
 *	func (param), where func is a function with no return value and
 *	param is an integer parameter.  The new thread does not begin
 *	executing until another thread yields to it.
 */

int MySpawnThread (func, param)
	void (*func)();		/* function to be executed */
	int param;		/* integer parameter */
{
	if (! MyInitThreadsCalled) {
		Printf ("SpawnThread: Must call InitThreads first\n");
		Exit ();
	}
	else if(activeThreadCount>=MAXTHREADS)
		return -1;

	int index;
	int validIndex;
	int queueIndex;
	int j;
	//put into thread table
	for(index=(lastAssignedThread+1)%MAXTHREADS,j=0;j<MAXTHREADS;index=(index+1)%MAXTHREADS,j++)
	{
		if(thread[index].valid==0)
		{
			validIndex=index;
			lastAssignedThread=index;
			thread[validIndex].valid=1;
			activeThreadCount++;
			break;
		}
	}
	//put into thread queue on the tail
	for(queueIndex=0;queueIndex<(MAXTHREADS-1);queueIndex++)
	{
		if(threadQueue[queueIndex]==-1)
		{
			threadQueue[queueIndex]=validIndex;
			break;
		}
	}

	//copy original env to .env
	memcpy(thread[validIndex].env, thread[validIndex].initialEnv, sizeof(jmp_buf));
	thread[validIndex].threadFunc=func;
	thread[validIndex].funcParam=param;
	return validIndex;		/* done spawning, return new thread ID */
}

/*	MyYieldThread (t) causes the running thread to yield to thread t.
 *	Returns ID of thread that yielded to t (i.e., the thread that called
 *	MyYieldThread), or -1 if t is an invalid ID.
 */

int MyYieldThread (t)
	int t;				/* thread being yielded to */
{
	if (! MyInitThreadsCalled) {
		Printf ("YieldThread: Must call InitThreads first\n");
		Exit ();
	}

	if (t < 0 || t >= MAXTHREADS) {
		Printf ("YieldThread: %d is not a valid thread ID\n", t);
		return (-1);
	}
	if (! thread[t].valid) {
		Printf ("YieldThread: Thread %d does not exist\n", t);
		return (-1);
	}
//	currentRunningThread=MyGetThread();
	previousThread=currentRunningThread;
	if(currentRunningThread==t) //yield to itself, return t
		return t;
	
	currentRunningThread=t;
	int queueIndex;
	int i;
	//find t
	for(i=0;i<(MAXTHREADS-1);i++)
	{
		if(threadQueue[i]==t)
		{
			queueIndex=i;
			break;
		}
	}
	//shift left
	for(i=queueIndex;i<(MAXTHREADS-2);i++)
		threadQueue[i]=threadQueue[i+1];
	threadQueue[MAXTHREADS-2]=-1;
    if (thread[previousThread].valid == 1) {
		for(i=0;i<(MAXTHREADS-1);i++)
		{
			if(threadQueue[i]==-1)
			{
				threadQueue[i]=previousThread;
				break;
			}
		}
	}
	
	//go to yield thread
	if(setjmp(thread[previousThread].env)==0)
	{
		longjmp(thread[currentRunningThread].env,1);
	}
	return previousThread;
}

/*	MyGetThread () returns ID of currently running thread.
 */

int MyGetThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("GetThread: Must call InitThreads first\n");
		Exit ();
	}
	return currentRunningThread;
}

/*	MySchedThread () causes the running thread to simply give up the
 *	CPU and allow another thread to be scheduled.  Selecting which
 *	thread to run is determined here.  Note that the same thread may
 * 	be chosen (as will be the case if there are no other threads).
 */

void MySchedThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("SchedThread: Must call InitThreads first\n");
		Exit ();
	}
	else if(activeThreadCount==0)
	{
		Exit();
	}
	else if(thread[threadQueue[0]].valid==0)
		return;
	MyYieldThread(threadQueue[0]);

}

/*	MyExitThread () causes the currently running thread to exit.
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("ExitThread: Must call InitThreads first\n");
		Exit ();
	}
	int eThread;
	eThread=currentRunningThread;
	thread[eThread].valid=0;
	activeThreadCount--;
	MySchedThread();
}
