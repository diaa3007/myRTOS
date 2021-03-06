/*
 * mutex.c
 *
 *  Created on: Jun 8, 2020
 *      Author: Diaa Eldeen
 */


#include "mutex.h"


extern volatile queue_t readyQueues[PRIORITY_LEVELS];
extern OSThread_t* volatile pxRunning;


void OS_mutexCreate(mutex_t* pxMutex) {

	ASSERT_TRUE(pxMutex != NULL);

	pxMutex->val = 1;	// Available = 1
	OS_queueInit(&pxMutex->waitingQueue);
}


uint32_t OS_mutexLock(mutex_t* pxMutex) {

	ASSERT_TRUE(pxMutex != NULL);

	// Try locking
	while(1) {

		if(__LDREXW(&pxMutex->val)) {	// Available

			if( !(__STREXW(0, &pxMutex->val)) ) {	// Lock success
				__DMB();
				break;
			}
			else {		// Lock not success
				// Try locking again
			}

		}
		else {						// Non-available

			// Put it in the mutex waiting list then force a context switch
			OS_queuePushThread(&pxMutex->waitingQueue, pxRunning);
			OS_yield();
			return 2;	// The function repeats the mutex lock SVCall
		}
	}

	return 0;	// Success
}

uint32_t OS_mutexRelease(mutex_t* pxMutex) {

	ASSERT_TRUE(pxMutex != NULL);


	// Use mutual exclusion pair instructions to release it
	while(1) {

		ASSERT_TRUE(!(__LDREXW(&pxMutex->val)));	// Must read 0 (locked)

		if( !(__STREXW(1, &pxMutex->val)) ) {	// Release success
			__DMB();
			break;
		}
		else {			// Release not success
			// Try releasing again
		}

	}


	// If the mutex waiting list is not empty, make the front thread ready
	if(pxMutex->waitingQueue.ui32NoOfItems) {
		OSThread_t* pxThread = pxMutex->waitingQueue.xHead.pxNext;
		OS_queuePushThread(&readyQueues[pxThread->ui32Priority], \
				OS_queuePopThread(&pxMutex->waitingQueue, pxThread));

		// If the unblocked thread has a higher priority than the running one, yield
		if(pxThread->ui32Priority < pxRunning->ui32Priority) {
			OS_queuePushThread(&readyQueues[pxRunning->ui32Priority], pxRunning);
			OS_yield();
		}
	}

	return 0;	// Success
}
