%{
  #include "../inc/helpers.hpp"
  #include <cstdio>
  #include <string>
	extern int yylex(void);
	extern void yyerror(const char*);
%}

%output "misc/parser.cpp"
%defines "misc/parser.hpp"

%union {
	int num;
	char* ident;
  char* string;
  reg_obj* reg;
	arg* arguments;
  instr_arg* instr_arguments;
  operand* op;
}

%token TOKEN_LPAR
%token TOKEN_RPAR
%token TOKEN_PLUS
%token TOKEN_COMMA
%token TOKEN_DOLLAR
%token TOKEN_COLLON
%token TOKEN_NEWLINE
%token TOKEN_PERCENT

%token <num>   TOKEN_NUM
%token <ident> TOKEN_IDENT
%token <string> TOKEN_STRING
%token <ident> TOKEN_GLOBAL
%token <ident> TOKEN_EXTERN
%token <ident> TOKEN_SECTION
%token <ident> TOKEN_WORD
%token <ident> TOKEN_SKIP
%token <ident> TOKEN_END
%token <ident> TOKEN_ASCII

%token <reg> TOKEN_REGISTER
%token <reg> STATUS_REG
%token <reg> HANDLER_REG
%token <reg> CAUSE_REG

%token <ident> HALT
%token <ident> INT
%token <ident> IRET
%token <ident> CALL
%token <ident> RET
%token <ident> JMP
%token <ident> BEQ
%token <ident> BNE
%token <ident> BGT
%token <ident> PUSH
%token <ident> POP
%token <ident> XCHG
%token <ident> ADD
%token <ident> SUB
%token <ident> MUL
%token <ident> DIV
%token <ident> NOT
%token <ident> AND
%token <ident> OR
%token <ident> XOR
%token <ident> SHL
%token <ident> SHR
%token <ident> LD
%token <ident> ST
%token <ident> CSRRD
%token <ident> CSRWR

%type <reg> reg
%type <reg> special_register
%type <arguments> sym_list
%type <arguments> literal
%type <arguments> literal_list
%type <arguments> symbols_literals
%type <op> operand
%type <op> operand_jump
%%

prog
  :
  | code prog
  ;


code
  : 
  | end code
  | directive code
  | instr code
  | label code
  ;

label  
  : TOKEN_IDENT TOKEN_COLLON 
    { //cout << "Reading label..." << endl;
      string name = $1;
      make_label(name);}
  ;
directive
  : TOKEN_GLOBAL sym_list end
    { //cout << "Reading directive .global ..." << endl;
      arg* temp = $2;
      // cout << "Printing elements in rule directive:" << endl;
      // while(temp){
      //   cout << temp->symbol << " " << temp->literal << endl;
      //   temp = temp->next;
      // }
      //print_arg($2);
      //cout << "Printed all the arguments!" << endl;
      make_directive(".global", $2, 1);}
  | TOKEN_EXTERN sym_list end
    { //cout << "Reading directive .extern ..." << endl;
      make_directive(".extern", $2, 1);}
  | TOKEN_SECTION TOKEN_IDENT end
    {
      //cout << "Reading directive .section ..." << endl;
      string name = $2;
      arg* argument = make_argument(name, NONE);  
      make_directive(".section", argument, 3);
    }
  | TOKEN_WORD  symbols_literals end
    { //cout << "Reading directive .word ..." << endl;
      make_directive(".word", $2, 4);}
  | TOKEN_SKIP  literal end
    { //cout << "Reading directive .skip ..." << endl;
      make_directive(".skip", $2, 2);}
  | TOKEN_END end
    { //cout << "Reading directive .end ..." << endl;
      make_directive(".end", NULL, 0);}
  | TOKEN_ASCII TOKEN_STRING end
    {
      string name = $2;
      arg* argument = make_argument(name, NONE);  
      make_directive(".ascii", argument, 3);
    }
  ;

sym_list
  : TOKEN_IDENT
    { 
      string name = $1;
      //cout << "Making symbol in :TOKEN_IDENT" << endl;
      $$ = make_argument(name, NONE);
    }
  | sym_list TOKEN_COMMA TOKEN_IDENT
    { 
      //cout <<"Making symbol in second rule" << endl;
      arg* first_arg = $1;
      arg* temp = first_arg;
      while(temp->next){
        temp = temp->next;
      }
      //cout << "In rule, element to link to is " << temp->symbol << temp->literal << endl;
      string name = $3;
      arg* argument = make_argument(name, NONE);
      temp->next = argument;
      $$ = first_arg;
    }
  ;

literal
  : TOKEN_NUM
  {$$ = make_argument("", $1);}
  ; 

literal_list
  : literal
  | literal_list TOKEN_COMMA literal
  {   
  arg* first_arg = $1;
  first_arg->next = $3;
  $$ = $1;
  }
  ;

symbols_literals
  : sym_list
  | literal_list
  ;

instr
  : HALT end
    { 
      make_instruction("halt", NULL);}
  | INT end
    { 
      make_instruction("int", NULL);}
  | IRET end
    { 
      make_instruction("iret", NULL);}
  | CALL operand_jump end
    {
      instr_arg* arguments = new instr_arg();
      arguments->op = $2;
      //print_operand($2);
      make_instruction("call", arguments);
      }
  | RET end
    { 
      make_instruction("ret", NULL);}
  | JMP operand_jump end
    {
      
      instr_arg* arguments = new instr_arg();
      $2->jump = true;
      arguments->op = $2;
      make_instruction("jmp", arguments);
    }
  | BEQ TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg TOKEN_COMMA operand_jump end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      arguments->op = $8;
      $8->jump = true;
      make_instruction("beq", arguments);
    }
  | BNE TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg TOKEN_COMMA operand_jump end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      $8->jump = true;
      arguments->op = $8;
      make_instruction("bne", arguments);
    }
  | BGT TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg TOKEN_COMMA operand_jump end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      $8->jump = true;
      arguments->op = $8;
      make_instruction("bgt", arguments);
    }
  | PUSH TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      arguments->registers = first;
      make_instruction("push", arguments);
    }
  | POP TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      arguments->registers = first;
      make_instruction("pop", arguments);
    }
  | XCHG TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("xchg", arguments);
    }
  | ADD TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("add", arguments);
    }
  | SUB TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("sub", arguments);
    }
  | MUL TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("mul", arguments);
    }
  | DIV TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("div", arguments);
    }
  | NOT TOKEN_PERCENT reg end
    { 
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      arguments->registers = first;
      make_instruction("not", arguments);
    }
  | AND TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("and", arguments);
    }
  | OR TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("or", arguments);
    }
  | XOR TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("xor", arguments);
    }
  | SHL TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("shl", arguments);
    }
  | SHR TOKEN_PERCENT reg TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $3;
      first->next = $6;
      arguments->registers = first;
      make_instruction("shr", arguments);
    }
  | LD operand TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      arguments->op = $2;
      arguments->registers = $5;
      make_instruction("ld", arguments);
    }
  | ST TOKEN_PERCENT reg TOKEN_COMMA operand end
    {
      
      instr_arg* arguments = new instr_arg();
      arguments->registers = $3;
      arguments->op = $5;
      make_instruction("st", arguments);
    }
  | CSRRD special_register TOKEN_COMMA TOKEN_PERCENT reg end
    {
      
      instr_arg* arguments = new instr_arg();
      reg_obj* first = $2;
      $2->special = true;
      first->next = $5;
      arguments->registers = first;
      make_instruction("csrrd", arguments);
    }
  | CSRWR TOKEN_PERCENT reg TOKEN_COMMA special_register end
    {
      
      instr_arg* arguments = new instr_arg();
      $5->special = true;
      reg_obj* first = $3;
      first->next = $5;
      arguments->registers = first;
      make_instruction("csrwr", arguments);
    }
  ;

/* operand* make_operand(int val, bool mem, bool sym = false, bool reg = false, int offset = 0); */
operand
  : TOKEN_DOLLAR literal
    {
      operand* op = new operand();
      //cout << "Before asigning values to operand!" << endl;
      op->value = $2->literal;
      op->reg = NULL;
      op->memory = false;
      op->value_present = true;
      //cout << "After making of operand!" << endl;
      $$ = op;
    }
  | TOKEN_DOLLAR TOKEN_IDENT
    {
      operand* op = new operand();
      op->value = NONE;
      op->memory = false;
      op->value_present = false;
      string name = $2;
      op->symbol = name;
      $$ = op;
    }
  | literal
    {
      operand* op = new operand();
      op->value = $1->literal;
      op->memory = true;
      op->value_present = false;
      $$ = op;
    }
  | TOKEN_IDENT
    {
      operand* op = new operand();
      op->value = NONE;
      op->memory = true;
      op->value_present = false;
      string name = $1;
      op->symbol = name;
      $$ = op;
    }
  | TOKEN_PERCENT reg
    {
      operand* op = new operand();
      op->value = NONE;
      op->memory = false;
      op->value_present = false;
      op->reg = $2;
      $$ = op;
    }
  | TOKEN_LPAR TOKEN_PERCENT reg TOKEN_RPAR
    {
      operand* op = new operand();
      op->value = NONE;
      op->memory = true;
      op->value_present = false;
      op->reg = $3;
      op->symbol = "";
      $$ = op;
    }
  | TOKEN_LPAR TOKEN_PERCENT reg TOKEN_PLUS literal TOKEN_RPAR
    {
      operand* op = new operand();
      op->value = $5->literal;
      op->memory = true;
      op->value_present = false;
      op->reg = $3;
      $$ = op;
    }
  | TOKEN_LPAR TOKEN_PERCENT reg TOKEN_PLUS TOKEN_IDENT TOKEN_RPAR
    {
      operand* op = new operand();
      op->value = NONE;
      op->memory = true;
      op->value_present = false;
      op->reg = $3;
      string name = $5;
      op->symbol = name;
      $$ = op;
    }
  ;

operand_jump
  : literal
    {
      operand* op = new operand();
      op->value = $1->literal;
      op->memory = false;
      op->value_present = true;
      op->jump = true;
      $$ = op;
    }
  | TOKEN_IDENT
    {
      operand* op = new operand();
      op->value = NONE;
      op->memory = false;
      op->value_present = false;
      string name = $1;
      op->symbol = name;
      op->jump = true;
      $$ = op;
    }
  ;

reg
  : TOKEN_REGISTER
    {
      // reg_obj* newreg = new reg_obj();
      // newreg->name = *$1;
      // newreg->next = NULL;
      // newreg->special = false;
      // $$ = newreg;
      //cout <<"Made register in rule reg: "<<$1->name << ", special: " << $1->special << endl;
      $$ = $1;
    }
  ;

special_register
  : STATUS_REG
    {
      // reg_obj* newreg = new reg_obj();
      // newreg->name = $1->name;
      // newreg->next = NULL;
      // newreg->special = true;
      // $$ = newreg;
      $$ = $1;
    }
  | HANDLER_REG
    {
      // reg_obj* newreg = new reg_obj();
      // newreg->name = $1->name;
      // newreg->next = NULL;
      // newreg->special = true;
      // $$ = newreg;
      $$ = $1;
    }
  | CAUSE_REG
    {
      // reg_obj* newreg = new reg_obj();
      // newreg->name = $1->name;
      // newreg->next = NULL;
      // newreg->special = true;
      // $$ = newreg;
      $$ = $1;
    }
  ;

end
  : end TOKEN_NEWLINE
  | TOKEN_NEWLINE
  ;

%%