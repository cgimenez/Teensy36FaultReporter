/*

    Teensy Fault Reporter

    Copyright (C) 2019 TACTIF CIE <www.tactif.com> / Bordeaux - France
    Author Christophe Gimenez <christophe.gimenez@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>

*/

#ifndef TEENSY_FAULT_REPORTER_H
#define TEENSY_FAULT_REPORTER_H

#define FAULT_REPORTER_ENABLE

#ifdef FAULT_REPORTER_ENABLE
#define FAULT_REPORTER_PATTERN 0x4655434B42554753
#define FAULT_REPORTER_BUFSIZE 4096
#define FAULT_REPORTER_USE_BUILTIN_LED

#define SCB_SHCSR_USGFAULTENA (uint32_t)1 << 18
#define SCB_SHCSR_BUSFAULTENA (uint32_t)1 << 17
#define SCB_SHCSR_MEMFAULTENA (uint32_t)1 << 16

#define ACTLR (*(volatile uint32_t *)0xE000E008)

__attribute__((section(".noinit"))) extern uint64_t fault_reporter_pattern;
__attribute__((section(".noinit"))) extern char fault_reporter_s[FAULT_REPORTER_BUFSIZE];

void fault_reporter_init();
void fault_reporter_report();
void fault_handler(int kind, uint32_t stack[]);

enum { r0, r1, r2, r3, r12, lr, pc, psr };

#define FAULT_HANDLER(L, FN)                                                                                                                                   \
    void __attribute__((naked)) FN() {                                                                                                                         \
        uint32_t *sp = 0;                                                                                                                                      \
        asm volatile("TST LR, #0x4\n\t"                                                                                                                        \
                     "ITE EQ\n\t"                                                                                                                              \
                     "MRSEQ %0, MSP\n\t"                                                                                                                       \
                     "MRSNE %0, PSP\n\t"                                                                                                                       \
                     : "=r"(sp)                                                                                                                                \
                     :                                                                                                                                         \
                     : "cc");                                                                                                                                  \
        fault_handler(L, sp);                                                                                                                                  \
    }

#define FAULT_REPORTER_INIT fault_reporter_init();
#define FAULT_REPORTER_REPORT fault_reporter_report();

#else

#define FAULT_REPORTER_INIT
#define FAULT_REPORTER_REPORT

#endif
#endif