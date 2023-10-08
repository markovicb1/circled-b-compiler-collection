#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <termios.h>
#include <unistd.h>
#define NO_VALUE -123456789
#define PC 15
#define SP 14
#define STATUS 0
#define HANDLER 1
#define CAUSE 2
#define TERM_IN 0xffffff04
#define TERM_OUT 0xffffff00
using namespace std;
/*
Emulation is possible if and only if it is possible to successfully load the hex file into the memory of the emulated computer system
At the end of the emulation, the result of the emulator is printed in the following format:
-----------------------------------------------------------------
Emulated processor executed halt instruction
Emulated processor state:
 r0=0x00000000  r1=0x00000000   r2=0x00000000   r3=0x00000000
 r4=0x00000000  r5=0x00000000   r6=0x00000000   r7=0x00000000
 r8=0x00000000  r9=0x00000000  r10=0x00000000  r11=0x00000000
r12=0x00000000 r13=0x00000000  r14=0x00000000  r15=0x00000000

Description of the computer system:

r15 is PC register -> contains the address of the next instruction
r14 is SP register -> contains the address of the last occupied location on the stack -> the stack grows towards lower addresses

Speical registers are also present:
    0 status -> cpu status word
    1 handler -> interrupt routine
    2 cause -> interrupt cause -> 1: bad instruction; 2:timer interrupt; 3:terminal interrupt; 4: software interrupt

Starting from address 0xffffff00 to address 0xffffffff, there is a space with memory-mapped registers used in working with peripherals
The system has only one interrupt routine whose address is written in the handler register

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Interrupt requests are serviced only after the current machine instruction has been atomically executed to completion.
When accepting an interrupt request and entering the 
interrupt routine, the processor places the status word and the return address on the stack in that order and then globally masks the interrupts.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

When keyboard key is clicked, emulator does the following:
    1) writes the ASCII value of the clicked character from the keyboard to the "register" term_in
    2) generates terminal interrupt

term_out: [0xffffff00 - 0xffffff03] -> output data register, when each entry is placed in this "register", its ASCII value is printed on the display
term_in:  [0xffffff04 - 0xffffff07] -> input data register, the ASCII value of the character clicked on the keyboard is written into it

*/

typedef struct BYTE_TYPE{
    char value;
}byte_type;

typedef struct ADDR_BYTES{
    uint address;
    vector<byte_type*> bytes;
}addr_bytes;

typedef struct MEM_LOC{
    uint address;
    char value;
}memory_location;

class Register{
    private:
        int id;
        bool special;
        int value;
    public:
        Register(){

        }
        Register(int id, bool special = false, uint value = 0){
            this->id = id;
            this->special = special;
            this->value = value;
        }
        uint get_value(){
            return value;
        }
        bool is_special(){
            return special;
        }
        void set_value(uint value){
            this->value = value;
        }
        void increase_by_four(){
            this->value += 4;
        }
        void decrease_by_four(){
            this->value -= 4;
        }
};

class Emulator{
    private:
        vector<Register*> gprs;
        vector<Register*> csrs;
        FILE* input;
        vector<addr_bytes*> input_structure;
        vector<memory_location*> memory;
        bool resolve_machine_code(char oc, char mod, char regA, char regB, char regC, short displacement);
        bool address_initialized(uint address);
    public:
        Emulator(FILE* input){

            this->input = input;
            for(int i = 0; i < 16;i++){
                Register* reg = new Register(i);
                gprs.push_back(reg);
            }
            gprs.at(15)->set_value(0x40000000);
            for(int i = 0; i < 3;i++){
                Register* reg = new Register(i, true);
                csrs.push_back(reg);
            }
        }
        char get_byte_value(uint address);
        int get_word_value(uint address);
        void set_word_value(uint address, int value);

        void set_terminal();
        void read_input();
        void fill_memory();
        void emulate();
        void print_registers_image();
        void print_memory();
};