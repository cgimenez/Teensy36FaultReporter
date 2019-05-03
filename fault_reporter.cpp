/*

    Teensy Fault Reporter

    Copyright (C) 2019 TACTIF CIE <www.tactif.com> / Bordeaux - France
    Author Christophe Gimenez <christophe.gimenez@gmail.com>
    Sources of knowledge :
    - the definitive guide to arm cortex-m3 and cortex-m4 processors, Joseph Yiu
    - https://github.com/cvra/arm-cortex-tools/blob/master/fault.c
    - https://blog.feabhas.com/2013/02/developing-a-generic-hard-fault-handler-for-arm-cortex-m3cortex-m4/
    - And many others

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

#include <Arduino.h>
#include "fault_reporter.h"

#ifdef FAULT_REPORTER_ENABLE

FAULT_HANDLER(3, hard_fault_isr)
FAULT_HANDLER(4, memmanage_fault_isr)
FAULT_HANDLER(5, bus_fault_isr)
FAULT_HANDLER(6, usage_fault_isr)

#define MMFSR_IACCVIOL (1 << 0)  // Instruction access violation
#define MMFSR_DACCVIOL (1 << 1)  // Data access violation
#define MMFSR_MUNSTKERR (1 << 3) // Unstacking error
#define MMFSR_MSTKERR (1 << 4)   // Stacking error
#define MMFSR_MLSPERR (1 << 5)   // MemManage Fault during FP lazy state preservation
#define MMFSR_MMARVALID (1 << 7) // Memory Manage Address Register address valid flag
// BusFault Status Register, BFSR
#define BFSR_IBUSERR (1 << 0)     // Instruction bus error flag
#define BFSR_PRECISERR (1 << 1)   // Precise data bus error
#define BFSR_IMPRECISERR (1 << 2) // Imprecise data bus error
#define BFSR_UNSTKERR (1 << 3)    // Unstacking error
#define BFSR_STKERR (1 << 4)      // Stacking error
#define BFSR_LSPERR (1 << 5)      // Bus Fault during FP lazy state preservation
#define BFSR_BFARVALID (1 << 7)   // Bus Fault Address Register address valid flag
// UsageFault Status Register, UFSR
#define UFSR_UNDEFINSTR (1 << 0) // The processor attempt to execute an undefined instruction
#define UFSR_INVSTATE (1 << 1)   // Invalid combination of EPSR and instruction
#define UFSR_INVPC (1 << 2)      // Attempt to load EXC_RETURN into pc illegally
#define UFSR_NOCP (1 << 3)       // Attempt to use a coprocessor instruction
#define UFSR_UNALIGNED (1 << 8)  // Fault occurs when there is an attempt to make an unaligned memory access
#define UFSR_DIVBYZERO (1 << 9)  // Fault occurs when SDIV or DIV instruction is used with a divisor of 0
// HardFault Status Register, HFSR
#define HFSR_VECTTBL (1 << 1)   // Fault occurs because of vector table read on exception processing
#define HFSR_FORCED (1 << 30)   // Hard Fault activated when a configurable Fault was received and cannot activate
#define HFSR_DEBUGEVT (1 << 31) // Fault related to debug

char fault_reporter_s[FAULT_REPORTER_BUFSIZE];
uint64_t fault_reporter_pattern;

void fault_reporter_init() {
#ifdef FAULT_REPORTER_USE_BUILTIN_LED
    pinMode(LED_BUILTIN, OUTPUT);
#endif
    SCB_SHCSR |= SCB_SHCSR_BUSFAULTENA | SCB_SHCSR_USGFAULTENA | SCB_SHCSR_MEMFAULTENA; // Enable handlers
    ACTLR |= (1 << 1);                                                                  // Precise addr
    ACTLR |= (1 << 2);                                                                  //
    ACTLR |= (1 << 3);                                                                  //
}

void fault_reporter_report() {
    if (fault_reporter_pattern == FAULT_REPORTER_PATTERN) {
        fault_reporter_pattern = 0;
        Serial.println("LAST CRASH");
        Serial.println(fault_reporter_s);
        while (1) {
            yield();
        }
    }
}

inline void to_binary(int size, uint32_t x, char *b) {
    for (int z = 0; z < size; z++) {
        b[size - 1 - z] = ((x >> z) & 0x1) ? '1' : '0';
    }
    b[size] = '\n';
    b[size + 1] = 0;
}

void fault_handler(int kind, uint32_t stack[]) {
#ifdef FAULT_REPORTER_USE_BUILTIN_LED
    digitalWriteFast(LED_BUILTIN, HIGH);
    delayMicroseconds(5000);
#endif
    bzero(fault_reporter_s, sizeof(fault_reporter_s));
    fault_reporter_pattern = FAULT_REPORTER_PATTERN;

    uint16_t UFSR = SCB_CFSR >> 16;
    uint8_t BFSR = (SCB_CFSR & 0x0000FFFF) >> 8;
    uint8_t MMFSR = SCB_CFSR & 0x000000FF;

    for (int i = r0; i <= r3; i++) {
        sprintf(&fault_reporter_s[strlen(fault_reporter_s)], "R%i        : %08lX %lu\n", i, stack[i], stack[i]);
    }
    sprintf(&fault_reporter_s[strlen(fault_reporter_s)], "SCB_BFAR  : %08lX\n", SCB_BFAR);
    sprintf(&fault_reporter_s[strlen(fault_reporter_s)], "SCB_MMFAR : %08lX\n", SCB_MMFAR);

    strcat(&fault_reporter_s[strlen(fault_reporter_s)], "SCB_CFSR  : ");
    to_binary(32, SCB_CFSR, &fault_reporter_s[strlen(fault_reporter_s)]);

    strcat(&fault_reporter_s[strlen(fault_reporter_s)], "UFSR      : ");
    to_binary(16, UFSR, &fault_reporter_s[strlen(fault_reporter_s)]);

    strcat(&fault_reporter_s[strlen(fault_reporter_s)], "BFSR      : ");
    to_binary(8, BFSR, &fault_reporter_s[strlen(fault_reporter_s)]);

    strcat(&fault_reporter_s[strlen(fault_reporter_s)], "MMFSR     : ");
    to_binary(8, MMFSR, &fault_reporter_s[strlen(fault_reporter_s)]);

    strcat(&fault_reporter_s[strlen(fault_reporter_s)], "REASON    : ");
    switch (kind) {
        case 3:
            strcat(&fault_reporter_s[strlen(fault_reporter_s)], "HARD FAULT\n");
            break;
        case 4:
            strcat(&fault_reporter_s[strlen(fault_reporter_s)], "MEM MANAGE FAULT\n");
            break;
        case 5:
            strcat(&fault_reporter_s[strlen(fault_reporter_s)], "BUS FAULT\n");
            break;
        case 6:
            strcat(&fault_reporter_s[strlen(fault_reporter_s)], "USAGE FAULT\n");
            break;
        default:
            strcat(&fault_reporter_s[strlen(fault_reporter_s)], "UNKNOWN FAULT\n");
            break;
    }

    sprintf(&fault_reporter_s[strlen(fault_reporter_s)], "\nPC=0x%lX LR=0x%lX make crash\n", stack[pc], stack[lr]); // LR is EXC_RETURN
    SCB_AIRCR = 0x05FA0004 | (SCB_AIRCR & 0x700);
    while (1)
        ; // Wait for the phoenix to rebirth
}

#endif
