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

// Don't try to use the FPU before calling utInit()

#ifndef _ut_h
#define _ut_h

#include "ut_conf.h"
#include <stdlib.h>
#include <stdint.h>

#ifndef UT_CLOCK_FREQ
#define UT_CLOCK_FREQ			168000000	// Hz
#endif
#ifndef UT_TICK_FREQ
#define UT_TICK_FREQ			1000		// Hz
#endif
#ifndef UT_CALLOC
#define UT_CALLOC				calloc
#endif
#ifndef UT_PRIORITY
#define UT_PRIORITY				8
#endif

#define utTick()				(utData.tick)
#define utThisTask()			(utCurrentTask)

#define UT_DEBUG

#ifndef AIRCR_VECTKEY_MASK
#define AIRCR_VECTKEY_MASK		((uint32_t)0x05FA0000)
#endif
#ifndef NVIC_PriorityGroup_4
#define NVIC_PriorityGroup_4	((uint32_t)0x300)
#endif

#define UT_SCHED_LOCKED			(__get_BASEPRI() == (UT_PRIORITY << 4))

enum {
    TASK_STATE_HIBERNATE = 0,
    TASK_STATE_WAITING,
    TASK_STATE_RUNNING
};

typedef uint32_t utStack_t;

typedef struct taskStruct {
    utStack_t	*stackPtr;
    uint32_t	stackSize;
    utStack_t	*stackTop;

    struct taskStruct *next;
    struct taskStruct *prev;

    uint32_t	runTick;
    uint8_t	priority;
    uint8_t	state;
} taskStruct_t;

typedef struct {
    volatile uint32_t tick;
    taskStruct_t *tasks;
    taskStruct_t *idleTask;
	volatile uint8_t needSchedule;
    uint8_t started;
} utStruct_t;

typedef void utTaskFunction_t(void *p);

extern utStruct_t utData;
extern taskStruct_t *utCurrentTask;
extern taskStruct_t *utNextTask;
extern uint32_t utSchedLocked;

extern taskStruct_t *utInit(utTaskFunction_t *idleFunc, void *param,  uint32_t stackSize);
extern taskStruct_t *utCreateTask(utTaskFunction_t *func, void *param, uint8_t priority, uint32_t stackSize);
extern void utStart(void);
extern void utYield(int n);
extern void utRunThread(taskStruct_t *threadP);
extern void utSleep(void);

#endif
