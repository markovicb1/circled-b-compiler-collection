#include <iostream>
#include <string>
#include <cstring>
using namespace std;
#define NONE -360

typedef struct BYTE_TYPE {
	char value;
}byte_type;

typedef struct WORD{
	int value;
}word;

typedef struct ARG {
	string symbol;
	int literal;
	struct ARG *next;
}arg;

typedef struct REG{
	string name;
	struct REG* next;
	bool special;
}reg_obj;

typedef struct OPERAND{
	int value;
	reg_obj* reg;
	string symbol;
	bool memory; /* if true -> value is addres from which we read the real value; if false -> thats the real value */
	bool value_present;
	bool jump;
}operand;

typedef struct INSTR_ARG{
	reg_obj* registers;
	operand* op;
}instr_arg;

typedef struct INSTR {
	string name;
	instr_arg* arguments;
	int address;  //ADDED AFTER MAKING PARSER
}instr;

typedef struct DIRECTIVE {
	string directive_name;
	arg* arguments;
}directive;

typedef struct LABEL{
	string name;
	int val;
}label;

typedef struct GLOBAL_STRUCT{
	instr* instruction;
	directive* dir;
	label* lb;
	struct GLOBAL_STRUCT* next;
}global_struct;

extern global_struct *head;

char* copy_str(const char*);

void print_directive(directive* dir);

arg* make_argument(string, int);
instr* make_instruction(string, instr_arg*);
directive* make_directive(string directive_name, arg* arguments, int type);
label* make_label(string name);

void print_arg(arg* argument);
void print_operand(operand* op);
void print_instr_args(instr_arg* argument);
void print_instruction(instr*);
void print_directive(directive*);
void print_label(label*);
void print_all(global_struct*);
void free_all(global_struct*);

int little_endian_representation(int number);