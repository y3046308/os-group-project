/*
 * catmouse.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 * 26-11-2007: KMS : Modified to use cat_eat and mouse_eat
 * 21-04-2009: KMS : modified to use cat_sleep and mouse_sleep
 * 21-04-2009: KMS : added sem_destroy of CatMouseWait
 * 05-01-2012: TBB : added comments to try to clarify use/non use of volatile
 * 22-08-2013: TBB: made cat and mouse eating and sleeping time optional parameters
 *
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>
#include <synchprobs.h>
#include "opt-A1.h"

/*
 * 
 * cat,mouse,bowl simulation functions defined in bowls.c
 *
 * For Assignment 1, you should use these functions to
 *  make your cats and mice eat from the bowls.
 * 
 * You may *not* modify these functions in any way.
 * They are implemented in a separate file (bowls.c) to remind
 * you that you should not change them.
 *
 * For information about the behaviour and return values
 *  of these functions, see bowls.c
 *
 */

/* this must be called before any calls to cat_eat or mouse_eat */
extern int initialize_bowls(unsigned int bowlcount);

extern void cleanup_bowls( void );
extern void cat_eat(unsigned int bowlnumber, int eat_time );
extern void mouse_eat(unsigned int bowlnumber, int eat_time);
extern void cat_sleep(int sleep_time);
extern void mouse_sleep(int sleep_time);

/*
 *
 * Problem parameters
 *
 * Values for these parameters are set by the main driver
 *  function, catmouse(), based on the problem parameters
 *  that are passed in from the kernel menu command or
 *  kernel command line.
 *
 * Once they have been set, you probably shouldn't be
 *  changing them.
 *
 * These are only ever modified by one thread, at creation time,
 * so they do not need to be volatile.
 */
int NumBowls;  // number of food bowls
int NumCats;   // number of cats
int NumMice;   // number of mice
int NumLoops;  // number of times each cat and mouse should eat

/* 
 * Defaults here are as they were with the previos implementation
 * where these could not be changed.
 */
int CatEatTime = 1;      // length of time a cat spends eating
int CatSleepTime = 5;    // length of time a cat spends sleeping
int MouseEatTime = 3;    // length of time a mouse spends eating
int MouseSleepTime = 3;  // length of time a mouse spends sleeping

/*
 * Once the main driver function (catmouse()) has created the cat and mouse
 * simulation threads, it uses this semaphore to block until all of the
 * cat and mouse simulations are finished.
 */
struct semaphore *CatMouseWait;

/*
 * 
 * Function Definitions
 * 
 */


/*
 * cat_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds cat identifier from 0 to NumCats-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Each cat simulation thread runs this function.
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 */

//keep track of how many and which bowls are left
//should be volatile because multiple threads must be able to access it
#if OPT_A1
volatile char* bowl; //keep track of used bowls
volatile int eating_cats = 0;
volatile int eating_mice = 0;
volatile int cat_so_far = 0;
volatile int mouse_so_far = 0;
volatile bool cat_wchan = 0;
volatile bool mouse_wchan = 0;
struct lock **lock;
struct cv *wait_mice, *wait_cats;
#endif


static
void
cat_simulation(void * unusedpointer, 
               unsigned long catnumber)
{
#if OPT_A1
  int i;
  int j;
  (void) unusedpointer;
  (void) catnumber;

  for(i=0;i<NumLoops;i++) {

	cat_sleep(CatSleepTime);
	lock_acquire(lock[0]); //for checking the bowl state
	while(eating_cats == NumBowls || eating_mice > 0){
		cv_wait(wait_cats, lock[0]);
		cat_wchan = 1;
	}
/*	if(cat_so_far - NumBowls == 1){
		kprintf("cat ate enough\n\n");
		while(mouse_so_far - NumBowls != 1){
			cv_broadcast(wait_mice, lock[0]);
			if(eating_mice == 0){
				break;
			}
			cv_wait(wait_cats, lock[0]);
			cat_wchan = 1;
		}
		cat_so_far = 0;
	}*/
	
	for(j=0;j<NumBowls;j++){
		if(bowl[j] == '-'){
			if(cat_so_far == 10){
				while(mouse_so_far < 10){
					cv_wait(wait_cats, lock[0]);
					cv_broadcast(wait_mice, lock[0]);
					if(!mouse_wchan && eating_mice == 0){
						cv_broadcast(wait_cats, lock[0]);
					}
					cat_wchan = 1;
				}
				cat_so_far = 0;
			}
			lock_acquire(lock[j+1]); //lock for a specific bowl
			eating_cats++;
			bowl[j] = 'c';
			lock_release(lock[0]); //let others check bowl state
			cat_eat(j+1, CatEatTime);
		//	kprintf("\n\ncat ate\n\n");
			cat_so_far++;
			eating_cats--;
			bowl[j] = '-';
			cv_broadcast(wait_mice, lock[0]);
            if(!mouse_wchan && eating_mice ==0){
            cv_broadcast(wait_cats, lock[0]);
            }
			mouse_wchan = 0;
			lock_release(lock[j+1]); //let others eat from this bowl
			break;
		}
	}
  }	


#else
  int i;
  unsigned int bowl;

  /* avoid unused variable warnings. */
  (void) unusedpointer;
  (void) catnumber;


  /* your simulated cat must iterate NumLoops times,
   *  sleeping (by calling cat_sleep() and eating
   *  (by calling cat_eat()) on each iteration */
  for(i=0;i<NumLoops;i++) {

    /* do not synchronize calls to cat_sleep().
       Any number of cats (and mice) should be able
       sleep at the same time. */
    cat_sleep(CatSleepTime);

    /* for now, this cat chooses a random bowl from
     * which to eat, and it is not synchronized with
     * other cats and mice.
     *
     * you will probably want to control which bowl this
     * cat eats from, and you will need to provide 
     * synchronization so that the cat does not violate
     * the rules when it eats */

    /* legal bowl numbers range from 1 to NumBowls */
    bowl = ((unsigned int)random() % NumBowls) + 1;
    cat_eat(bowl, CatEatTime);

  }
#endif
  /* indicate that this cat simulation is finished */
  V(CatMouseWait); 
}

/*
 * mouse_simulation()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds mouse identifier from 0 to NumMice-1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      each mouse simulation thread runs this function
 *
 *      You need to finish implementing this function using 
 *      the OS161 synchronization primitives as indicated
 *      in the assignment description
 *
 */

static
void
mouse_simulation(void * unusedpointer,
          unsigned long mousenumber)
{
#if OPT_A1
  int i;
  int j;

  (void) unusedpointer;
  (void) mousenumber;

  for(i=0;i<NumLoops;i++) {
	mouse_sleep(MouseSleepTime);
	lock_acquire(lock[0]);
	while(eating_mice == NumBowls || eating_cats > 0){
		cv_wait(wait_mice, lock[0]);
		cv_broadcast(wait_cats, lock[0]);
		mouse_wchan = 1;
	}
	
/*    if(mouse_so_far - NumBowls == 1){
		kprintf("mouse ate enough\n\n");
        while(cat_so_far - NumBowls != 1){
			kprintf("cat eat\n\n");
			cv_broadcast(wait_cats,lock[0]);
            if(eating_cats == 0){
				break;
			}
			cv_wait(wait_mice, lock[0]);
            cat_wchan = 1;
        }
		mouse_so_far = 0;
    }*/

    for(j=0;j<NumBowls;j++){
        if(bowl[j] == '-'){
            if(mouse_so_far == 10){
                while(cat_so_far < 10){
                    cv_wait(wait_mice, lock[0]);
					cv_broadcast(wait_cats, lock[0]);
					if(!cat_wchan&&eating_cats == 0){
						cv_broadcast(wait_mice, lock[0]);
					}
                    mouse_wchan = 1;
                }
                mouse_so_far = 0;
            }
            lock_acquire(lock[j+1]);
            eating_mice++;
            bowl[j] = 'm';
            lock_release(lock[0]);
			mouse_eat(j+1,MouseEatTime);
			mouse_so_far++;
			eating_mice--;
			bowl[j] = '-';
			cv_broadcast(wait_cats, lock[0]);
            if(!cat_wchan && eating_cats == 0){
            	cv_broadcast(wait_mice, lock[0]);
            }
			cat_wchan = 0;
			lock_release(lock[j+1]);
			break;
        }
    }
  }

#else
  int i;
  unsigned int bowl;

  /* Avoid unused variable warnings. */
  (void) unusedpointer;
  (void) mousenumber;


  /* your simulated mouse must iterate NumLoops times,
   *  sleeping (by calling mouse_sleep()) and eating
   *  (by calling mouse_eat()) on each iteration */
  for(i=0;i<NumLoops;i++) {

    /* do not synchronize calls to mouse_sleep().
       Any number of mice (and cats) should be able
       sleep at the same time. */
    mouse_sleep(MouseSleepTime);

    /* for now, this mouse chooses a random bowl from
     * which to eat, and it is not synchronized with
     * other cats and mice.
     *
     * you will probably want to control which bowl this
     * mouse eats from, and you will need to provide 
     * synchronization so that the mouse does not violate
     * the rules when it eats */

    /* legal bowl numbers range from 1 to NumBowls */
    bowl = ((unsigned int)random() % NumBowls) + 1;
    mouse_eat(bowl, MouseEatTime);

  }
#endif
  /* indicate that this mouse is finished */
  V(CatMouseWait); 
}


/*
 * catmouse()
 *
 * Arguments:
 *      int nargs: should be 5 or 9
 *      char ** args: args[1] = number of food bowls
 *                    args[2] = number of cats
 *                    args[3] = number of mice
 *                    args[4] = number of loops
 * Optional parameters
 *                    args[5] = cat eating time
 *                    args[6] = cat sleeping time
 *                    args[7] = mouse eating time
 *                    args[8] = mouse sleeping time
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up cat_simulation() and
 *      mouse_simulation() threads.
 *      You may need to modify this function, e.g., to
 *      initialize synchronization primitives used
 *      
 *      However, you should should ensure that this function
 *      continues to create the appropriate numbers of
 *      cat and mouse threads, to initialize the simulation,
 *      and to wait for all cats and mice to finish.
 */

int
catmouse(int nargs,
         char ** args)
{
  int index, error;
  int i;
  /* check and process command line arguments */
  if ((nargs != 9) && (nargs != 5)) {
    kprintf("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS\n");
    kprintf("or\n");
    kprintf("Usage: <command> NUM_BOWLS NUM_CATS NUM_MICE NUM_LOOPS ");
    kprintf("CAT_EATING_TIME CAT_SLEEPING_TIME MOUSE_EATING_TIME MOUSE_SLEEPING_TIME\n");
    return 1;  // return failure indication
  }

  /* check the problem parameters, and set the global variables */
  NumBowls = atoi(args[1]);
  if (NumBowls <= 0) {
    kprintf("catmouse: invalid number of bowls: %d\n",NumBowls);
    return 1;
  }
  NumCats = atoi(args[2]);
  if (NumCats < 0) {
    kprintf("catmouse: invalid number of cats: %d\n",NumCats);
    return 1;
  }
  NumMice = atoi(args[3]);
  if (NumMice < 0) {
    kprintf("catmouse: invalid number of mice: %d\n",NumMice);
    return 1;
  }
  NumLoops = atoi(args[4]);
  if (NumLoops <= 0) {
    kprintf("catmouse: invalid number of loops: %d\n",NumLoops);
    return 1;
  }

  if (nargs == 9) {
    CatEatTime = atoi(args[5]);
    if (NumLoops <= 0) {
      kprintf("catmouse: invalid cat eating time: %d\n",CatEatTime);
      return 1;
    }
  
    CatSleepTime = atoi(args[6]);
    if (NumLoops <= 0) {
      kprintf("catmouse: invalid cat sleeping time: %d\n",CatSleepTime);
      return 1;
    }
  
    MouseEatTime = atoi(args[7]);
    if (NumLoops <= 0) {
      kprintf("catmouse: invalid mouse eating time: %d\n",MouseEatTime);
      return 1;
    }
  
    MouseSleepTime = atoi(args[8]);
    if (NumLoops <= 0) {
      kprintf("catmouse: invalid mouse sleeping time: %d\n",MouseSleepTime);
      return 1;
    }
  }

  kprintf("Using %d bowls, %d cats, and %d mice. Looping %d times.\n",
          NumBowls,NumCats,NumMice,NumLoops);
  kprintf("Using cat eating time %d, cat sleeping time %d\n", CatEatTime, CatSleepTime);
  kprintf("Using mouse eating time %d, mouse sleeping time %d\n", MouseEatTime, MouseSleepTime);

  /* create the semaphore that is used to make the main thread
     wait for all of the cats and mice to finish */
  CatMouseWait = sem_create("CatMouseWait",0);
  if (CatMouseWait == NULL) {
    panic("catmouse: could not create semaphore\n");
  }

  /* 
   * initialize the bowls
   */
  if (initialize_bowls(NumBowls)) {
    panic("catmouse: error initializing bowls.\n");
  }

#if OPT_A1
  lock = kmalloc((NumBowls+1)*sizeof(struct lock*));
  for(i=0; i<NumBowls+1; i++){
    lock[i] = lock_create("lock");
  }
  wait_cats = cv_create("wait_cats");
  wait_mice = cv_create("wait_mice");

  bowl = kmalloc(NumBowls*sizeof(char));
  for(i=0; i<NumBowls; i++){
    bowl[i] = '-';
  }
  

#endif

  /*
   * Start NumCats cat_simulation() threads.
   */
  for (index = 0; index < NumCats; index++) {
    error = thread_fork("cat_simulation thread", NULL, cat_simulation, NULL, index);
    if (error) {
      panic("cat_simulation: thread_fork failed: %s\n", strerror(error));
    }
  }

  /*
   * Start NumMice mouse_simulation() threads.
   */
  for (index = 0; index < NumMice; index++) {
    error = thread_fork("mouse_simulation thread", NULL, mouse_simulation, NULL, index);
    if (error) {
      panic("mouse_simulation: thread_fork failed: %s\n",strerror(error));
    }
  }

  /* wait for all of the cats and mice to finish before
     terminating */  
  for(i=0;i<(NumCats+NumMice);i++) {
    P(CatMouseWait);
  }

  /* clean up the semaphore that we created */
  sem_destroy(CatMouseWait);

  /* clean up resources used for tracking bowl use */
  cleanup_bowls();

  return 0;
}

/*
 * End of catmouse.c
 */
