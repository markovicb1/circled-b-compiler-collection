# circled-b-compiler-collection // Fully functional assembler, linker and emulator for abstract computer system

## About the project

This project represents fully functional compiler collection for abstract computer system. It is made from scratch in C/C++, with additional tools such as Flex and Bison.

The whole project was third year's biggest obstacle, which was covered on ***System software*** course at the School of Electrical Engineering @ University of Belgrade :serbia:

## Computer system overview

The system is made of CPU (Central Processing Unit), RAM (Random Access Memory), System Timer and Terminal. All of the components are interconnected via the system bus. System Timer and Terminal are additionally connected with the CPU using interrupt busses. The system can be presented with the following scheme:

![scheme](https://github.com/markovicb1/circled-b-compiler-collection/assets/115867204/5416e2f2-0686-4278-941a-3f707c960192)

## CPU Overview

Central Processing Unit is the most important part of the system. It is 32bit two address type with 16 GPRs (General Purpose Register) and 3 special (user inaccessible) registers. The following registers have special usage:

- r14: SP (Stack pointer) register
  - Holds the address of the last used place in stack memory
  - Stack memory is special memory used by program
- r15: PC (Program counter) register
  - Holds the address of the next instruction to be executed
- status: Holds processors status word
  - The status word of the processor, that is, the status register, consists of flags that provide the possibility of configuring the interrupt mechanism
- handler: Holds the address of the interrupt handler routine
  - This particular system has ONLY ONE interrupt handler routine
- cause: Holds the reason for the interrupt routine
  - There are 4 causes for the interrupt
    - 1: Bad instruction passed
    - 2: System Timer interrupt
    - 3: Terminal interrupt
    - 4: Software interrupt

All of the instructions are performed atomically. Interrupt requests are serviced only after the current machine instruction has been atomically executed to completion. When accepting an interrupt request and entering the interrupt routine, the processor places the status word and the return address on the stack in that order and then globally masks the interrupts

## CPU Instruction set

CPU Instructions are divided in several groups.

a,b,c,ddd represent GPRA, GPRB, GPRC and displacement.

| Instruction type | Instruction modifier | Effect | Machine code |
| ---------------- | -------------------- | ------ | ------------ |
| HALT | 0 | Shuts down the CPU | 0x00000000 |
| INT | 0 | push status; push pc; cause <= 4; status<=status&(~0x1); pc<=handle; | 0x10000000 |
| CALL | 0 | push pc; pc<=gpr[a]+gpr[b]+d; | 0x20ab0ddd |
| CALL | 1 | push pc; pc<=mem32[gpr[a]+gpr[b]+d]; | 0x21ab0ddd |
| JMP | 0 | pc<=gpr[a]+d; | 0x30abcddd |
| BEQ | 1 | if (gpr[b] == gpr[c]) pc<=gpr[a]+d; | 0x31abcddd |
| BNE | 2 | if (gpr[b] != gpr[c]) pc<=gpr[a]+d; | 0x32abcddd |
| BGT | 3 | if (gpr[b] signed> gpr[c]) pc<=gpr[a]+d; | 0x33abcddd |
| JMP | 8 | pc<=mem32[gpr[a]+d]; | 0x38abcddd |
| BEQ | 9 | if (gpr[b] == gpr[c]) pc<=mem32[gpr[a]+d]; | 0x39abcddd |
| BNE | A | if (gpr[b] != gpr[c]) pc<=mem32[gpr[a]+d]; | 0x3Aabcddd |
| BGT | B | if (gpr[b] signed> gpr[c]) pc<=mem32[gpr[a]+d]; | 0x3Babcddd |
| XCHG | 0 | temp<=gpr[b]; gpr[b]<=gpr[c]; gpr[c]<=temp; | 0x400bc000 |
| ADD | 0 | gpr[a]<=gpr[b] + gpr[c]; | 0x50abc000 |
| SUB | 1 | gpr[a]<=gpr[b] - gpr[c]; | 0x51abc000 |
| MUL | 2 | gpr[a]<=gpr[b] * gpr[c]; | 0x52abc000 |
| DIV | 3 | gpr[a]<=gpr[b] / gpr[c]; | 0x53abc000 |
| NOT | 0 | gpr[a]<=~gpr[b]; | 0x60abc000 |
| AND | 1 | gpr[a]<=gpr[b] & gpr[c]; | 0x61abc000 |
| OR | 2 | gpr[a]<=gpr[b] or gpr[c]; | 0x62abc000 |
| XOR | 3 | gpr[a]<=gpr[b] ^ gpr[c]; | 0x63abc000 |
| SHL | 0 | gpr[a]<=gpr[b] << gpr[c]; | 0x70abc000 |
| SHR | 1 | gpr[a]<=gpr[b] >> gpr[c]; | 0x71abc000 |
| ST | 0 | mem32[gpr[a]+gpr[b]+d]<=gpr[c]; | 0x80abcddd |
| ST | 2 | mem32[mem32[gpr[a]+gpr[b]+d]]<=gpr[c]; | 0x82abcddd |
| ST | 1 | gpr[a]<=gpr[a]+d; mem32[gpr[a]]<=gpr[c]; | 0x81abcddd |
| LD | 0 | gpr[a]<=csr[b]; | 0x90abcddd |
| LD | 1 | gpr[a]<=gpr[b]+d; | 0x91abcddd |
| LD | 2 | gpr[a]<=mem32[gpr[b]+gpr[c]+d]; | 0x92abcddd |
| LD | 3 | gpr[a]<=mem32[gpr[b]]; gpr[b]<=gpr[b]+d; | 0x93abcddd |
| LD | 4 | csr[a]<=gpr[b]; | 0x94abcddd |
| LD | 5 | csr[a]<=csr[b] or d; | 0x95abcddd |
| LD | 6 | csr[a]<=mem32[gpr[b]+gpr[c]+d]; | 0x96abcddd |
| LD | 7 | csr[a]<=mem32[gpr[b]]; gpr[b]<=gpr[b]+d; | 0x97abcddd |

## Terminal


