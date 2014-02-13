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

#ifndef _ut_conf_h
#define _ut_conf_h

/*
    System's clock frequency for calculating SysTick period
*/
//#define UT_CLOCK_FREQ		168000000					// Hz
#define UT_CLOCK_FREQ		rccClocks.HCLK_Frequency	// Hz

/*
    Tick frequency
*/
#define UT_TICK_FREQ		1000						// Hz

/*
    What to use for calloc() calls
*/
#define UT_CALLOC		calloc
#define UT_FREE			free

/*
    base priority of ut's kernel.  Above this ISRs can call ut API calls,
    below it are fast interrupts which cannot.
*/
#define UT_PRIORITY		8


#endif
