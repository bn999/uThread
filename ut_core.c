/*
    This file is part of uThread.

    uThread is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    uThread is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with uThread.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2014  Bill Nesbitt
*/

#include "ut.h"
#include "rcc.h"
#include <string.h>

utStruct_t utData;
taskStruct_t *utCurrentTask;
taskStruct_t *utNextTask;

void utSysTickInit(void) {
    // SysTick initialization based on the system clock
    SysTick->LOAD = UT_CLOCK_FREQ / UT_TICK_FREQ - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
		  SysTick_CTRL_ENABLE_Msk |
		  SysTick_CTRL_TICKINT_Msk;
}

void utIsrInit(void) {
    // set NVIC to have 4 bits of preemption priority and 0 bits sub priority
    SCB->AIRCR = AIRCR_VECTKEY_MASK | NVIC_PriorityGroup_4;

    // PendSV at ut's base priority
    NVIC_SetPriority(PendSV_IRQn, UT_PRIORITY);

    // SysTick at slightly higher priority
    NVIC_SetPriority(SysTick_IRQn, UT_PRIORITY-1);
}

__STATIC_INLINE void utLock(void) {
    __set_BASEPRI(UT_PRIORITY << 4);
}

__STATIC_INLINE void utUnlock(void) {
    __set_BASEPRI(0);
}

__STATIC_INLINE void utSchedule(void) {
    taskStruct_t *t;

	do {
		utData.needSchedule = 0;

		for (t = utData.tasks; t != 0; t = t->next) {
			if (t->runTick <= utData.tick) {
				utNextTask = t;
				break;
			}
		}

		// trigger PendSV exception
		SCB->ICSR = 0x10000000;
	} while (utData.needSchedule);
}

__STATIC_INLINE void utSchedLock(void) {
    utLock();
}

__STATIC_INLINE void utSchedUnlock(void) {
    utSchedule();

    // release lock, allow interrupts
    utUnlock();
}

// 48 word base stack size
void *utStackContextInit(utTaskFunction_t *func, void *param, utStack_t *stackPtr) {
    uint32_t *context = (uint32_t *)stackPtr;

    *(context--) = (uint32_t)0x01000000L;	    // xPSR
    *(context--) = (uint32_t)func;				// Entry point of task
    *(context--) = (uint32_t)0xFFFFFFFEL;	    // LR
    *(context--) = (uint32_t)0x0000000CL;	    // R12
    *(context--) = (uint32_t)0x00000003L;	    // R3
    *(context--) = (uint32_t)0x00000002L;	    // R2
    *(context--) = (uint32_t)0x00000001L;	    // R1
    *(context--)  = (uint32_t)param;		    // R0: argument
    context = context - 7;

    context = context - 32;						// FPU registers
    *(--context) = (uint32_t)0x30000000;	    // FPU status reg

    return (context);							// Returns location of new stack top
}

// insert task into linked list, ordered by priority
void utInsertTask(taskStruct_t *task) {
    // first task?
    if (utData.tasks == 0) {
		utData.tasks = task;
    }
    // insert into task list
    else {
		taskStruct_t *a = 0;
		taskStruct_t *b = 0;

		// traverse linked list
		for (a = utData.tasks; a != 0; a = a->next) {
			if (task->priority <= a->priority) {
				task->next = a;
				task->prev = a->prev;
				a->prev = task;
				// first on the list?
				if (b == 0)
					utData.tasks = task;
				else
					b->next = task;
				break;
			}
			b = a;
		}

		// didn't find a place?
		if (a == 0) {
			// make it the last on the list
			b->next = task;
			task->prev = b;
		}
    }
}

// stack size in words
taskStruct_t *utCreateTask(utTaskFunction_t *func, void *param, uint8_t priority, uint32_t stackSize) {
    taskStruct_t *task = 0;
    utStack_t *stackPtr;

    stackPtr = (utStack_t *)UT_CALLOC(1, stackSize*4);

	if (stackPtr) {
		memset(stackPtr, 0xff, stackSize*4);

		task = (taskStruct_t *)UT_CALLOC(1, sizeof(taskStruct_t));

		if (task) {
			task->stackTop = stackPtr;

			// move to stack top
			stackPtr += stackSize-1;

			task->stackPtr = utStackContextInit(func, param, stackPtr);
			task->stackSize = stackSize;
			task->priority = priority;
			task->state = TASK_STATE_HIBERNATE;

			utSchedLock();
			utInsertTask(task);

			if (utData.started)
				utSchedUnlock();
		}
		else {
			UT_FREE(stackPtr);
		}
	}

    return task;
}

taskStruct_t *utInit(utTaskFunction_t *idleFunc, void *param, uint32_t stackSize) {
    // setup FPU
    FPU->FPCCR &= ~FPU_FPCCR_ASPEN_Msk;		// turn off FP context save
    FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;		// turn off lazy save

    // setup SysTick
    utSysTickInit();

    // setup ISR priorities
    utIsrInit();

    // create idle task (without unlock)
    utData.idleTask = utCreateTask(idleFunc, param, 0xff, stackSize);

    // set initial PSP register value
	__set_PSP((uint32_t)(utData.idleTask->stackPtr + (48 - 7)));
}

void utStart(void) {
    utCurrentTask = utData.idleTask;
    utNextTask = utCurrentTask;
    utCurrentTask->state = TASK_STATE_RUNNING;

    utData.started = 1;
    utSchedUnlock();
}

void utYield(int n) {
    utSchedLock();
    utCurrentTask->runTick = utData.tick + n;
    utSchedUnlock();
}

void utRunThread(taskStruct_t *threadP) {
	threadP->runTick = 0;

	if (!UT_SCHED_LOCKED) {
		utSchedLock();
		utSchedUnlock();
	}
	else {
		utData.needSchedule = 1;
	}
}

void utSleep(void) {
    utSchedLock();
	utCurrentTask->runTick = -1;
    utSchedUnlock();
}

void SysTick_Handler(void) {
    utData.tick++;

    // SysTick can still fire and preempt even if there is a lock,
    // so, we must only try to schedule if someone else isn't.
    if (!UT_SCHED_LOCKED)
        utSchedule();
	else
		utData.needSchedule = 1;
}
