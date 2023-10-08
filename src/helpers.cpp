#include "../inc/helpers.hpp"
#include <cstring>

int is_register(const char *in)
{
	int last = 0;
	while (in[last+1]) last++;
	return in[0] == 'r' && in[last] == 'x';
}

char* copy_str(const char *in)
{
	size_t len = strnlen(in, 32);
	char* buf = new char[len + 1];
	strncpy(buf, in, len);
	buf[len] = '\0';
	return buf;
}

arg* make_argument(string sym, int lit)
{
	arg* a = new arg();
	a->symbol = sym;
	a->literal = lit;
	a->next = NULL;
	//cout << "Made argument " << a->symbol << endl;
	return a;
}

 instr* make_instruction(string name, instr_arg *args)
{
	instr* i = new instr();
	i->name = name;
	i->arguments = args;
	global_struct* temp = head;
	global_struct* el = new global_struct();
	el->next = NULL;
	if(!head){
		head = el;
	}
	else{
		while(temp->next){
			temp = temp->next;
			}
		temp->next = el;
	}
	
	el->dir = NULL;
	el->instruction = i;
	el->lb = NULL;
	return i;
}

directive* make_directive(string directive_name, arg* arguments, int type)
{
	//cout << "Making directive with name " << directive_name << endl;
	directive* d = new directive();
	d->directive_name = directive_name;
	d->arguments = arguments;
	
	global_struct* el = new global_struct();
	el->next = NULL;
	if(!head){
		head = el;
	}
	else{
		global_struct* temp = head;
		while(temp->next){
		temp = temp->next;
		}
		temp->next = el;
	}
	el->dir = d;
	el->instruction = NULL;
	el->next = NULL;
	//cout << "Made directive " << d->directive_name << endl;
	return d;
}

void print_arg(arg *argument)
{
	arg* temp = argument;
	// cout << "Before while, argument is " << temp->symbol <<" " << temp->literal << endl;
	// cout <<"It's next element is " << temp->next->symbol <<" " << temp->next->literal << endl;
	// cout <<"It's third element is " << temp->next->next->symbol <<" " << temp->next->next->literal << endl;
	while(temp != NULL){
		if(temp->symbol != ""){
			cout << temp->symbol;
		}
		else{
				cout << temp->literal;
			}
		if(temp->next != NULL)
			{cout << ", ";}
		temp = temp->next;
	}
	//cout << "\nExiting function print_arg" << endl;
}

void print_operand(operand* op){
	//cout << "Before printing operand in print_operand" << endl;
	cout << "Operand: value= "<< op->value <<", register=" << (op->reg != NULL ? op->reg->name : "No registers") <<", symbol=" << op->symbol << ", read_from_memory=" <<op->memory<<", value_is_present="<<op->value_present<<", jump_operand="<<op->jump;
}

void print_instr_args(instr_arg* argument){
	if(argument->registers){
		cout << "Registers: ";
		reg_obj* temp = argument->registers;
		while(temp){
			cout << temp->name << ", special: " << temp->special;
			if(temp->next)
				cout << ", ";
			temp = temp->next;
		}
		cout <<"; ";
	}
	if(argument->op){
		print_operand(argument->op);
	}
	if(!argument->registers && !argument->op)
		cout << "No arguments for this instruction.";
}

void print_instruction(instr* instruction){
	cout << "Instruction: " << instruction->name;
	if(!instruction->arguments)
		cout << endl;
	else{
		cout << ", arguments: ";
		print_instr_args(instruction->arguments);
		cout << endl;
	}
}

void print_directive(directive* directive){
	cout << "Directive: " << directive->directive_name << ", arguments: ";
	print_arg(directive->arguments);
	cout << endl;
}

void print_label(label* label){
	cout << "Label: " << label->name << endl;
}

void print_all(global_struct *head)
{
	global_struct* temp = head;
	cout<<endl << "****PRINTING GLOBAL LIST*****" <<endl;
	while(temp){
		if(temp->dir){
			print_directive(temp->dir);
		}
		else if(temp->instruction){
			print_instruction(temp->instruction);
		}
		else{
			print_label(temp->lb);
		}
		temp = temp->next;
	}
}

label* make_label(string name){
	label* lb = new label();
	lb->name = name;
	global_struct* temp = head;
	while(temp->next){
		temp = temp->next;
	}
	temp->next = new global_struct();
	temp->next->lb = lb;
	temp->next->next = NULL;
	return lb;
}

void free_args(arg *args)
{
	/* in future :) */
}
void free_instrs(instr *instr)
{
	/* in future :) */
}

int little_endian_representation(int number){
	int x1 = (number >> (8*0)) & 0xff;
  int x2 = (number >> (8*1)) & 0xff;
  int x3 = (number >> (8*2)) & 0xff;
  int x4 = (number >> (8*3)) & 0xff;
  //printf("%02x %02x %02x %02x", (unsigned int)(x1 & 0xff), (unsigned int)(x2 & 0xff), (unsigned int)(x3 & 0xff), (unsigned int)(x4 & 0xff));
  int ret = x4 | (x3 << 8) | (x2 << 16) | (x1 << 24);
	return ret;
}

global_struct *head = NULL;