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

#ifdef __CC_ARM
__asm void PendSV_Handler(void) {
	IMPORT utCurrentTask
	IMPORT utNextTask

	LDR     R3, =utCurrentTask
	LDR     R1, [R3]			// R1 == current task
	LDR     R2, =utNextTask
	LDR     R2, [R2]			// R2 == next task

	CMP     R1, R2
	BEQ     exitPendSV
	MRS     R0, PSP				// Load Process Stack Pointer
	STMDB   R0!, {R4-R11}		// PUSH R4 through R11 on stack

	FSTMDBS R0!, {S0-S31}		// store FPU regs
	FMRX    R12, FPSCR
	STMDB   R0!,{R12}			// store FPU status reg

	STR     R0, [R1]			// Save orig PSP

	STR     R2, [R3]			// utCurrentTask = utNextTask
	LDR     R0, [R2]			// load SP of task be switch into

	LDMIA   R0!, {R12}			// load FPU status reg
	FMXR    FPSCR, R12
	FLDMIAS R0!, {S0-S31}		// load FPU regs

	LDMIA   R0!, {R4-R11}		// POP R4 through R11
	MSR     PSP, R0				// move new stack point to PSP

exitPendSV
	ORR     LR, LR, #0x04		// ensure exception return uses process stack
	BX      LR					// exit interrupt
	ALIGN
}
#endif

#ifdef __GNUC__
__attribute__ ((naked))
void PendSV_Handler(void) {
	__asm volatile(
		"LDR		R3, =utCurrentTask	\n"
		"LDR		R1, [R3]			\n" // R1 == current task
		"LDR		R2, =utNextTask		\n"
		"LDR		R2, [R2]			\n" // R2 == next task

		"CMP		R1, R2				\n"
		"BEQ		exitPendSV			\n"
		"MRS		R0, PSP				\n" // Load Process Stack Pointer
		"STMDB		R0!, {R4-R11}		\n" // PUSH R4 through R11 on stack

		"fstmdbs	r0!, {s0-s31}		\n" // store FPU regs
		"fmrx		r12, fpscr			\n"
		"stmdb		r0!, {r12}			\n" // store FPU status reg

		"STR		R0, [R1]			\n" // Save orig PSP

		"STR		R2, [R3]			\n" // utCurrentTask = utNextTask
		"LDR		R0, [R2]			\n" // load SP of task be switch into

		"ldmia		r0!, {r12}			\n" // load FPU status reg
		"fmxr		fpscr, r12			\n"
		"fldmias	r0!, {s0-s31}		\n" // load FPU regs

		"LDMIA		R0!, {R4-R11}		\n" // POP R4 through R11
		"MSR		PSP, R0				\n" // move new stack point to PSP

		"exitPendSV:					\n"
		"ORR		LR, LR, #0x04		\n" // ensure exception return uses process stack
		"BX			LR					\n" // exit interrupt
	);
}
#endif

#ifdef UT_DEBUG
/**
 * HardFaultHandler_C:
 * This is called from the HardFault_HandlerAsm with a pointer the Fault stack
 * as the parameter. We can then read the values from the stack and place them
 * into local variables for ease of reading.
 * We then read the various Fault Status and Address Registers to help decode
 * cause of the fault.
 * The function ends with a BKPT instruction to force control back into the debugger
 */
void HardFault_HandlerC(unsigned long *hardfault_args) {
	volatile unsigned long stacked_r0 ;
	volatile unsigned long stacked_r1 ;
	volatile unsigned long stacked_r2 ;
	volatile unsigned long stacked_r3 ;
	volatile unsigned long stacked_r12 ;
	volatile unsigned long stacked_lr ;
	volatile unsigned long stacked_pc ;
	volatile unsigned long stacked_psr ;
	volatile unsigned long _CFSR ;
	volatile unsigned long _HFSR ;
	volatile unsigned long _DFSR ;
	volatile unsigned long _AFSR ;
	volatile unsigned long _BFAR ;
	volatile unsigned long _MMAR ;

	stacked_r0 = ((unsigned long)hardfault_args[0]) ;
	stacked_r1 = ((unsigned long)hardfault_args[1]) ;
	stacked_r2 = ((unsigned long)hardfault_args[2]) ;
	stacked_r3 = ((unsigned long)hardfault_args[3]) ;
	stacked_r12 = ((unsigned long)hardfault_args[4]) ;
	stacked_lr = ((unsigned long)hardfault_args[5]) ;
	stacked_pc = ((unsigned long)hardfault_args[6]) ;
	stacked_psr = ((unsigned long)hardfault_args[7]) ;

	// Configurable Fault Status Register
	// Consists of MMSR, BFSR and UFSR
	_CFSR = (*((volatile unsigned long *)(0xE000ED28))) ;

	// Hard Fault Status Register
	_HFSR = (*((volatile unsigned long *)(0xE000ED2C))) ;

	// Debug Fault Status Register
	_DFSR = (*((volatile unsigned long *)(0xE000ED30))) ;

	// Auxiliary Fault Status Register
	_AFSR = (*((volatile unsigned long *)(0xE000ED3C))) ;

	// Read the Fault Address Registers. These may not contain valid values.
	// Check BFARVALID/MMARVALID to see if they are valid values
	// MemManage Fault Address Register
	_MMAR = (*((volatile unsigned long *)(0xE000ED34))) ;
	// Bus Fault Address Register
	_BFAR = (*((volatile unsigned long *)(0xE000ED38))) ;

	__asm("BKPT #0\n") ; // Break into the debugger
}

#ifdef __CC_ARM
__asm void HardFault_Handler(void) {
    IMPORT HardFault_HandlerC
    MOVS    R0,#4
    MOVS    R1, LR
    TST     R0, R1
    BEQ     _MSP
    MRS     R0, PSP
    B _HALT
	
_MSP
    MRS     R0, MSP
	
_HALT
    LDR     R1,[R0,#20]
    B       HardFault_HandlerC
    BKPT #0
}
#endif

#ifdef __GNUC__
__attribute__((naked))
void HardFault_Handler(void) {
	__asm volatile (
		" movs r0,#4			\n"
		" movs r1, lr			\n"
		" tst r0, r1			\n"
		" beq _MSP				\n"
		" mrs r0, psp			\n"
		" b _HALT				\n"
		
		"_MSP:					\n"
		" mrs r0, msp			\n"
		
		"_HALT:					\n"
		" ldr r1,[r0,#20]		\n"
		" b HardFault_HandlerC	\n"
		" bkpt #0				\n"
	);
}
#endif


#endif
