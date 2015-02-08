 /* Test Case 2: Yield/Spawn Ordering
 *
 * In this program you are to kill the zombies that are spawned. The only
 * way to correctly kill the zombies is by killing every third zombie you
 * see 32 times, or else all hell breaks loose! You must use your knowledge
 * of OS to yield to the zombie, kill it it, and save the world!
 *
 * You should test your kernel by seeing if it will work with the program
 * below, as well as other test cases you design.  You should test it
 * thoroughly, as it will be graded by seeing if it successfully runs with
 * other test programs (not available to you).
 */
#include "aux.h"
#include "umix.h"
#include "mythreads.h"

#define USE_REF 0
#define MAX_ZOMBIES 32

static void (*_init)();
static int (*_spawn)();
static int (*_yield)(int);
static int (*_get)();
static void (*_sched)();
static void (*_exit)();

static void Zombie();
static void Zombie1();
static int num_zombies;

void Main ()
{
  // Set the pointers of desired functions to corresponding variables.
  if (USE_REF == 0) {
    _init = &MyInitThreads;
    _spawn = &MySpawnThread;
    _yield = &MyYieldThread;
    _get = &MyGetThread;
    _sched = &MySchedThread;
    _exit = &MyExitThread;
  } else {
    _init = &InitThreads;
    _spawn = &SpawnThread;
    _yield = &YieldThread;
    _get = &GetThread;
    _sched = &SchedThread;
    _exit = &ExitThread;
  }

  _init ();
  int i;

  /* Create all threads */
  for (i = 1; i < MAXTHREADS; i++) {
    Printf ("Zombie %d has appeared.\n", _spawn (Zombie));
  }

  _yield (_get () + 3);

  _exit ();
}

void Zombie ()
{
  num_zombies++;
  Printf ("You have shot and killed Zombie %d.\n", _get ());
  _yield ((_get () + 3) % MAXTHREADS);

  int t = _spawn (Zombie1);
  if (t != -1) {
    Printf ("Zombie %d has appeared.\n", t);
  }
}

void Zombie1 ()
{
  num_zombies++;
  Printf ("You have shot and killed Zombie %d.\n", _get ());

  if (num_zombies < MAX_ZOMBIES) {
    int t = _spawn (Zombie1);
    if (t != -1) {
      Printf ("Zombie %d has appeared.\n", t);
    }
  }
}