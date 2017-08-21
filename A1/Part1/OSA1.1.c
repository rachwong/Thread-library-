/*
 ============================================================================
 Name        : OSA1.c
 Author      : Rachel Wong rwon253 
 Version     : 1.1
 Description : multiple thread no pre-emption implementation.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "littleThread.h"
#include "threads1.c" // rename this for different threads
Thread threads[50]; //my global variable, thread array 
Thread mythreads[50];//a copy of the list which I will link
Thread newThread; // the thread currently being set up
Thread mainThread; // the main thread
Thread currentThread; // this keeps track of the thread that is currently running in the array 
struct sigaction setUpAction;


/*
prints the states of each thread 
*/
void printThreadStates(){
		puts("Thread States");
		puts("=============");	
		for (int t = 0; t < NUMTHREADS; t++) {
			if(threads[t]->state == SETUP){
				printf("threadID: %d state: setup\n", threads[t]->tid);
			}
			if(threads[t]->state == RUNNING){
				printf("threadID: %d state: running\n", threads[t]->tid);
			}
			if(threads[t]->state == READY){
				printf("threadID: %d state: ready\n", threads[t]->tid);
			}
			if(threads[t]->state == FINISHED){
				printf("threadID: %d state: finished\n", threads[t]->tid);
			}
			
		}
} 

/*
 * Switches execution from prevThread to nextThread.
 * To find the next thread to execute you just need to find the next thread in the READY state.
 */
void switcher(Thread prevThread, Thread nextThread) {
	
	if (prevThread->state == FINISHED) { // it has finished, we need to move to the next thread 
		printf("\ndisposing %d\n", prevThread->tid);
		nextThread->state = RUNNING;
		currentThread->next->prev = currentThread->prev;
		currentThread->prev->next = currentThread->next;
		currentThread = nextThread;
		free(prevThread->stackAddr); // Wow! //clearing the stack memory 
		longjmp(nextThread->environment, 1); //Longjmp jumps to setjmp where we called it last. Will make setjmp call value 1
	} else if (setjmp(prevThread->environment) == 0) { // so we can come back here Setjmp is saving the current state of the thread. 
		prevThread->state = READY;
		nextThread->state = RUNNING;
		//printThreadStates();
		printf("scheduling %d\n", nextThread->tid);
		currentThread = nextThread;
		longjmp(nextThread->environment, 1);
		
		
	}
	
}

//first check if finished then run next thread on stack 
//otherwise probaby need to check if it's just one thread
//then if last thread we just run switcher back with localthread and mainthread 
void scheduler() {
	if (NUMTHREADS == 1){
		switcher(currentThread, mainThread);
	}
	else if (currentThread == currentThread->next){
		switcher(currentThread, mainThread);
	}
	else {
		switcher(currentThread, currentThread->next);
	}
	
}

/*
 * Associates the signal stack with the newThread.
 * Also sets up the newThread to start running after it is long jumped to.
 * This is called when SIGUSR1 is received.
 * Sets up the environment for the thread to do it's execution
 */
void associateStack(int signum) {
	Thread localThread = newThread; // what if we don't use this local variable?
	localThread->state = READY; // now it has its stack
	if (setjmp(localThread->environment) != 0) { // will be zero if called directly
		printThreadStates();
		(localThread->start)();
		localThread->state = FINISHED;
		scheduler();
		//switcher(localThread, mainThread); // at the moment back to the main thread
	}
}

/*
 * Sets up the user signal handler so that when SIGUSR1 is received
 * it will use a separate stack. This stack is then associated with
 * the newThread when the signal handler associateStack is executed.
 */
void setUpStackTransfer() {
	setUpAction.sa_handler = (void *) associateStack;
	setUpAction.sa_flags = SA_ONSTACK;
	sigaction(SIGUSR1, &setUpAction, NULL);
}


/*
 *  Sets up the new thread.
 *  The startFunc is the function called when the thread starts running.
 *  It also allocates space for the thread's stack.
 *  This stack will be the stack used by the SIGUSR1 signal handler.
 */
Thread createThread(void (startFunc)()) {
	static int nextTID = 0;
	Thread thread;
	stack_t threadStack;

	if ((thread = malloc(sizeof(struct thread))) == NULL) {
		perror("allocating thread");
		exit(EXIT_FAILURE);
	}
	thread->tid = nextTID++;
	thread->state = SETUP;
	thread->start = startFunc;
	if ((threadStack.ss_sp = malloc(SIGSTKSZ)) == NULL) { // space for the stack
		perror("allocating stack");
		exit(EXIT_FAILURE);
	}
	thread->stackAddr = threadStack.ss_sp;
	threadStack.ss_size = SIGSTKSZ; // the size of the stack
	threadStack.ss_flags = 0;
	if (sigaltstack(&threadStack, NULL) < 0) { // signal handled on threadStack
		perror("sigaltstack");
		exit(EXIT_FAILURE);
	}
	newThread = thread; // So that the signal handler can find this thread
	kill(getpid(), SIGUSR1); // Send the signal. After this everything is set.
	return thread;
}

int main(void) {
	struct thread controller;
	mainThread = &controller;
	mainThread->state = RUNNING;
	setUpStackTransfer();
	// create the threads
	for (int t = 0; t < NUMTHREADS; t++) {
		threads[t] = createThread(threadFuncs[t]);
		mythreads[t] = threads[t];
		
	}
	currentThread = mythreads[0];
	for (int t = 0; t < NUMTHREADS; t++) {
		if(NUMTHREADS == 1){
		mythreads[t]->prev = mythreads[t];
		mythreads[t]->next =  mythreads[t];
		}
		else {
			if (t == 0){
				mythreads[t]->prev = mythreads[NUMTHREADS-1];
				mythreads[t]->next = mythreads[t+1];
			}
			else if (t == NUMTHREADS-1){
				mythreads[t]->prev = mythreads[t-1];
				mythreads[t]->next =  mythreads[0];
			}
			else {
				mythreads[t]->prev = mythreads[t-1];
				mythreads[t]->next =  mythreads[t+1];
			}
		}
	}

	printThreadStates();
	//printf("%d\n", NUMTHREADS);
	puts("\nswitching to first thread\n");
	switcher(mainThread, threads[0]); //running the first thread? Main to thread[0] in array 
	puts("back to the main thread");
	printThreadStates();
	return EXIT_SUCCESS;
}
