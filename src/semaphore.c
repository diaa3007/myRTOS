/*
 * semaphore.c
 *
 *  Created on: Jun 11, 2020
 *      Author: Diaa Eldeen
 */


#include "semaphore.h"



extern volatile queue_t readyQueues[PRIORITY_LEVELS];
extern OSThread_t* volatile pxRunning;


void OS_semaphoreCreate(semaphore_t* pxSemaphore, uint32_t ui32InitialValue) {

	ASSERT_TRUE(pxSemaphore != NULL);

	pxSemaphore->val = ui32InitialValue;
	OS_queueInit(&pxSemaphore->waitingQueue);
}


uint32_t OS_semaphoreTake(semaphore_t* pxSemaphore) {

	ASSERT_TRUE(pxSemaphore != NULL);
	uint32_t uiSemVal;

	// Try taking
	while(1) {

		uiSemVal = __LDREXW(&pxSemaphore->val);
		if(uiSemVal) {	// Available

			if(!(__STREXW(uiSemVal - 1, &pxSemaphore->val))) { // Taking success
				__DMB();
				break;
			}
			else {		// Taking not success
				// Try taking again
			}

		}
		else {						// Non-available

			// Put it in the semaphore waiting list then force a context switch
			OS_queuePushThread(&pxSemaphore->waitingQueue, pxRunning);
			OS_yield();
			return 2;	// The function repeats the semaphore take SVCall
		}
	}

	return 0;	// Success
}

uint32_t OS_semaphoreGive(semaphore_t* pxSemaphore) {

	ASSERT_TRUE(pxSemaphore != NULL);

	// Use mutual exclusion pair instructions to increment the semaphore value
	while(1) {

		uint32_t uiSemVal = __LDREXW(&pxSemaphore->val);

		if( !(__STREXW(uiSemVal + 1, &pxSemaphore->val)) ) {	// Give success
			__DMB();
			break;
		}
		else {			// Give not success
			// Try Giving again
		}

	}

	// If the semaphore waiting list is not empty, make the front thread ready
	if(pxSemaphore->waitingQueue.ui32NoOfItems) {
		OSThread_t* pxThread = pxSemaphore->waitingQueue.xHead.pxNext;
		OS_queuePushThread(&readyQueues[pxThread->ui32Priority], \
				OS_queuePopThread(&pxSemaphore->waitingQueue, pxThread));

		// If the unblocked thread has a higher priority than the running one, yield
		if(pxThread->ui32Priority < pxRunning->ui32Priority) {
			OS_queuePushThread(&readyQueues[pxRunning->ui32Priority], pxRunning);
			OS_yield();
		}
	}


	return 0;	// Success
}
