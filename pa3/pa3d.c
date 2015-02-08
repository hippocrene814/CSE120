#include <stdio.h>
#include "aux.h"
#include "umix.h"

void InitRoad ();
void driveRoad (int from, int mph);

void Main ()
{
	InitRoad ();

	/* The following code is specific to this particular simulation,
	 * e.g., number of cars, directions, and speeds.  You should
	 * experiment with different numbers of cars, directions, and
	 * speeds to test your modification of driveRoad.  When your
	 * solution is tested, we will use different Main procedures,
	 * which will first call InitRoad before any calls to driveRoad.
	 * So, you should do any initializations in InitRoad.
	 */

	if (Fork () == 0) {			/* Car 2 */
		Delay (1162);
		driveRoad (WEST, 60);
		Exit ();
	}

	if (Fork () == 0) {			/* Car 3 */
		Delay (900);
		driveRoad (EAST, 50);
		Exit ();
	}

	if (Fork () == 0) {			/* Car 4 */
		Delay (900);
		driveRoad (WEST, 30);
		Exit ();
	}

	driveRoad (EAST, 40);			/* Car 1 */

	Exit ();
}

/* Our tests will call your versions of InitRoad and driveRoad, so your
 * solution to the car simulation should be limited to modifying the code
 * below.  This is in addition to your implementation of semaphores
 * contained in mykernel3.c.
 */
struct
{
	int nE;
	int nW;
	int nO;
	int tD;
	int pt[12];
	int wfp;
	int efp;

}shm;

void InitRoad ()
{
	/* do any initializations here */
	Regshm ((char *) &shm, sizeof (shm));	/* register as shared */
	shm.nE=0;
	shm.nW=0;
	shm.nO=0;
	shm.tD=2;
	shm.wfp=0; //when occupied = 1
	shm.efp=0; //when occupied = 1
	for(int i=0;i<12;i++)
	{
		shm.pt[i]=Seminit(1);
	}
//	shm.pt[0]=0;
//	shm.pt[12]=0;
}

#define IPOS(FROM)	(((FROM) == WEST) ? 1 : NUMPOS)

void driveRoad (from, mph)
	int from, mph;
{
	int c;					/* car id c = process id */
	int p, np, i;				/* positions */

	c = Getpid ();				/* learn this car's id */

	if(from == WEST) //come from west
	{
		shm.nW++;
		/////////////////////////
		Wait(shm.pt[0]);
		while(shm.wfp==1) ;
		////////////////////////
		if(shm.nO==0)   //no car on road
		{
			while((shm.tD==0)&&(shm.nE)!=0) ;
			while(shm.nO!=0) ;
			Wait(shm.pt[1]);
			shm.wfp=1;
			shm.nO++;
			shm.nW--;
			shm.tD=0;
			Signal(shm.pt[0]);
		}
		else    //car on the road
		{
			while((shm.tD==0)&&(shm.nE!=0)) ;
			if((shm.tD==0)&&(shm.nE==0))
			{
				Wait(shm.pt[1]);
				shm.wfp=1;
				shm.nO++;
				shm.nW--;
				shm.tD=0;
				Signal(shm.pt[0]);
			}
			else if(shm.tD==1)
			{
				while(shm.nO) ;
				Wait(shm.pt[1]);
				shm.wfp=1;
				shm.nO++;
				shm.nW--;
				shm.tD=0;
				Signal(shm.pt[0]);

			}
		}
	}
	else //come from east
	{
		shm.nE++;
		Wait(shm.pt[11]);
		while(shm.efp==1) ;  //if east first position is occupied, wait
		if(shm.nO==0)   //no car on road
		{
			while((shm.tD==1)&&(shm.nW!=0)) ;
			while(shm.nO!=0) ;
			Wait(shm.pt[10]);
			shm.efp=1;
			shm.nO++;
			shm.nE--;
			shm.tD=1;
			Signal(shm.pt[11]);
		}
		else  //car on the road
		{
			while((shm.tD==1)&&(shm.nW!=0)) ;
			if((shm.tD==1)&&(shm.nW==0))
			{
				Wait(shm.pt[10]);
				shm.efp=1;
				shm.nO++;
				shm.nE--;
				shm.tD=1;
				Signal(shm.pt[11]);
				
			}
			else if(shm.tD==0)
			{
				while(shm.nO) ;
				Wait(shm.pt[10]);
				shm.efp=1;
				shm.nO++;
				shm.nE--;
				shm.tD=1;
				Signal(shm.pt[11]);
			}
		}
	}
//--------------------------------------------------------------------------
	EnterRoad (from);
	PrintRoad ();
	Printf ("Car %d enters at %d at %d mph\n", c, IPOS(from), mph);
//--------------------------------------------------------------------------
	for (i = 1; i < 10; i++) {
		if (from == WEST) {
			p = i;
			np = i + 1;
			Wait(shm.pt[np]);
		} else {
			p = NUMPOS + 1 - i;
			np = p - 1;
			Wait(shm.pt[np]);
		}
//--------------------------------------------------------------------------
		Delay (3600/mph);
		ProceedRoad ();
		PrintRoad ();
		Printf ("Car %d moves from %d to %d\n", c, p, np);
		if((p==1)&&(from==WEST))
			shm.wfp=0;
		else if((p==10)&&(from==EAST))
			shm.efp=0;
		Signal(shm.pt[p]);

	}

	Delay (3600/mph);
	ProceedRoad ();
	PrintRoad ();
	Printf ("Car %d exits road\n", c);
//-----------------------------------------------------------------
	if(from == WEST)
	{
		Signal(shm.pt[10]);
		shm.nO--;
	}
	else
	{
		Signal(shm.pt[1]);
		shm.nO--;
	}
//-----------------------------------------------------------------
}