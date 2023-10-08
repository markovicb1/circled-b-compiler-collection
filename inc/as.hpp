#include "helpers.hpp"
#include <vector>


#define UND 0

typedef struct symbol_record{
  int number; //id in symbol table
  int value;
  string type; //NOTYP, SCTN
  string bind; //LOC, GLOB
  int ndx; //section id
  string name;
}symbol_record;

symbol_record* make_symbol(int number, long value, string type, string bind, int ndx, string name);
void set_symbol_number(symbol_record* symbol, int number);

typedef struct relocation_record{
  long offset; //refers to the first Byte of machine code that has to be changed
  string type;
  string symbol;
  int addend; //value to be added to the symbol value; differs from zero in case of local symbols
}reloc_record;

typedef struct section_record{ //represents a single section:
  int number;
  int address;
  int size;
  string name;
  vector<byte_type*> data;
  vector<word*> pool;
  vector<relocation_record*> reloc_table;
  bool placed;
}section_record;

char get_register_number(string register_name);

class Assembler{
  private:
    global_struct* input;
    FILE* output_file;
    vector<symbol_record*> symbol_table;
    vector<section_record*> section_table;
    string output;
    string current_section_name;
    int current_section_number;
    bool is_first_pass_done;
    bool is_second_pass_done;
    void write_machine_code(byte_type* first, byte_type* second, byte_type* third, byte_type* fourth);
    int resolve_operand_type(operand* op);
    void align_sections();
    void resolve_reloc_records();
  public:
    Assembler(global_struct* input, FILE* output_file){
        this->input = input;
        this->output_file = output_file;
        current_section_name = "";
        current_section_number = 0;
        symbol_record* init_symbol = new symbol_record();
        init_symbol->number = 0;
        init_symbol->value = 0l;
        init_symbol->type = "NOTYP";
        init_symbol->bind = "LOC";
        init_symbol->ndx = UND;
        init_symbol->name = "";
        this->symbol_table.push_back(init_symbol);
        this->is_first_pass_done = false;
        this->is_second_pass_done = false;
    }
    ~Assembler(){
      // delete all dynamic objects
      fclose(output_file);
    }

    void add_symbol(symbol_record* symbol);
    void add_section(section_record* section_rec);
    int get_section_number(string section_name);
    bool section_in_table(string section_name);
    bool symbol_defined(string name);
    symbol_record* get_symbol_by_name(string sym_name);
    int get_offset_to_pool();
    int process_instruction_and_update_counter(instr* instruction,  int locationCounter);

    void first_pass();
    void second_pass();
    void make_object_file();
    void print_results();

};