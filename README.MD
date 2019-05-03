Teensy Fault Reporter
=====================

Quick & Dirty fault reporter for Teensy and Cortex-M4. This simple tool helped me A LOT to debug my own code.

This tool might be unreliable and is provided AS-IS.

USAGE
-----

Copy fault_reporter.h and fault_reporter.cpp in your project (or add them in your local library folder)

```
#include "fault_reporter.h" // or <fault_reporter.h>
```

in your setup() function add

```
Serial.begin(112500);
delay(1000);
FAULT_REPORTER_INIT
FAULT_REPORTER_REPORT
```

When a fault occurs, teensy will reboot and report some informations about the cause.
The reporting is very barebone but sufficient for many use cases, I might extend it or rewrite it a better way in the future.

Example of what you might get

```
Waiting for '3955990-Teensy'...
Monitoring '3955990-Teensy'
LAST CRASH
R0        : 00000000 0
R1        : 00000000 0
R2        : 00000001 1
R3        : 00000000 0
SCB_BFAR  : E000ED38
SCB_MMFAR : E000ED34
SCB_CFSR  : 00000010000000000000000000000000
UFSR      : 0000001000000000
BFSR      : 00000000
MMFSR     : 00000000
REASON    : USAGE FAULT

PC=0x490 LR=0x4B3 make crash
```

The last line is for my own Makefile usage but you can find the offending line by issuing this command (provided that you're located in the directory where the firmware was compiled AND that the arm-none-eabi tools are in your PATH variable)

Then the teensy will wait for a new firmware upload.

```
arm-none-eabi-addr2line -a -i -p -s -f -C -e firmware.elf 0x490 0x4B3
```

Which output something like

```
0x00000490: do_usagefault() at main.cpp:27
0x000004b3: loop at main.cpp:32
```

If you want to disable fault reporting comment the line (in fault_reporter.h)

```
#define FAULT_REPORTER_ENABLE
```

Sketch example :

```
#include <Arduino.h>
#include <fault_reporter.h>

void setup() {
    Serial.begin(112500);
    delay(1000);
    FAULT_REPORTER_INIT
    FAULT_REPORTER_REPORT
}

void do_busfault() {
    struct lnk {
        lnk *next;
    };
    lnk *first = (lnk *)0x194c6059;
    for (lnk *p = first; p->next; p = p->next)
        ;
}

void do_usagefault() {
    SCB_CCR |= 1 << 4;
    int a = 1, b = 0;
    Serial.printf("%i", a / b);
}

void loop() {
    // do_busfault();
    do_usagefault();
}

```

**Side notes :**

The optimize flag options passed to gcc might produced binary code that is sometimes more difficult to analyze with addr2toline

I usually use -Og