%{
    #include "../inc/helpers.hpp"
    #include "../misc/parser.hpp"

    int line = 1; /* used to find line on which error occured in input file */
%}

%option outfile="misc/lexer.cpp" header-file="misc/lexer.hpp"

%option noyywrap

LPAR  "["
RPAR  "]"
PLUS  "+"
COMMA ","
DOLLAR "$"
COLLON ":"
PERCENT "%"

REGISTER r(0|[1-9][0-5]?)

INT   [0-9]+
HEX   0[Xx][0-9A-Fa-f]+

IDENT [A-Za-z_][A-Za-z0-9_]*
STRING \"[^\"]*\"
COM   "#"[^\n\r]*
WS    [ \r\t]*
NEWLINE "\n"

%%
.global {return TOKEN_GLOBAL;}
.extern {return TOKEN_EXTERN;}
.section {return TOKEN_SECTION;}
.word {return TOKEN_WORD;}
.skip {return TOKEN_SKIP;}
.end {return TOKEN_END;}
.ascii {return TOKEN_ASCII;}


halt {return HALT;}
int {return INT;}
iret {return IRET;}
call {return CALL;}
ret {return RET;}
jmp {return JMP;}
beq {return BEQ;}
bne {return BNE;}
bgt {return BGT;}
push {return PUSH;}
pop {return POP;}
xchg {return XCHG;}
add {return ADD;}
sub {return SUB;}
mul {return MUL;}
div {return DIV;}
not {return NOT;}
and {return AND;}
or {return OR;}
xor {return XOR;}
shl {return SHL;}
shr {return SHR;}
ld {return LD;}
st {return ST;}
csrrd {return CSRRD;}
csrwr {return CSRWR;}

sp      {
        reg_obj* reg = new reg_obj();
        reg->name = yytext;
        reg->next = NULL;
        reg->special = false;
        yylval.reg = reg;
        return TOKEN_REGISTER;
        }
pc      {
        reg_obj* reg = new reg_obj();
        reg->name = yytext;
        reg->next = NULL;
        reg->special = false;
        yylval.reg = reg;
        return TOKEN_REGISTER;
        }
%status  {
        //yylval.reg = copy_str(yytext);
        //yylval.reg = yytext;
        reg_obj* reg = new reg_obj();
        reg->name = "status";
        reg->next = NULL;
        reg->special = true;
        yylval.reg = reg;
        return STATUS_REG;
        }
%handler {
        //yylval.reg = copy_str(yytext);
        //yylval.reg = yytext;
        reg_obj* reg = new reg_obj();
        reg->name = "handler";
        reg->next = NULL;
        reg->special = true;
        yylval.reg = reg;
        return HANDLER_REG;
        }
%cause   { 
        //yylval.reg = copy_str(yytext);
        //yylval.reg = yytext;
        reg_obj* reg = new reg_obj();
        reg->name = "cause";
        reg->next = NULL;
        reg->special = true;
        yylval.reg = reg;
        return CAUSE_REG;
        }

{WS}        {}
{NEWLINE}   { 
            line++;
            return TOKEN_NEWLINE;
            }
{COM}       {}
{HEX}		{
	          sscanf(yytext, "%x", &yylval.num);
			  return TOKEN_NUM;
            }
{INT}       {
            sscanf(yytext, "%d", &yylval.num);
			return TOKEN_NUM;
            }
{REGISTER}  {   
            //yylval.reg = copy_str(yytext);
            //yylval.reg = yytext;
            //cout << "REIGSTER FOUND!!!" << endl;
            reg_obj* reg = new reg_obj();
            reg->name = yytext;
            reg->next = NULL;
            reg->special = false;
            yylval.reg = reg;
            return TOKEN_REGISTER;
            }
{IDENT}     {
	          yylval.ident = copy_str(yytext);
			  return TOKEN_IDENT;
            }
{STRING}    {
	          yylval.string = copy_str(yytext);
			  return TOKEN_STRING;
            }
{LPAR}      { return TOKEN_LPAR; }
{RPAR}      { return TOKEN_RPAR; }
{PLUS}      { return TOKEN_PLUS; }
{COMMA}     { return TOKEN_COMMA; }
{DOLLAR}    {return TOKEN_DOLLAR;}
{COLLON}    {return TOKEN_COLLON;}
{PERCENT}   {return TOKEN_PERCENT;}
%%

int yyerror(const char* msg)
{
    fprintf(stderr, "Lexer error on line %d. Message: %s. yytext value: %s\n", line, msg, yytext);
    return 0;
}