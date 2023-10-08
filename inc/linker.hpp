#include "../inc/as.hpp"
#include <regex>
#define MAX_LINE_SIZE 120

typedef vector<section_record*> sec_table;
typedef vector<symbol_record*> sym_table;

typedef struct PLACE_ARGUMENT{
    string section_name;
    uint address;
}place_argument;

typedef struct PROGRAM_ARGUMENTS{
    vector<FILE*> files;
    vector<place_argument> place_arguments;
    FILE* output_file;
    bool contains_hex;
    bool contains_reloc;
}program_arguments;

typedef struct OBJECT_TABLES{
    sec_table section_table;
    sym_table symbol_table;
}object_tables;

typedef struct GENERAL_SECTION_RECORD{
    int number;
    int address;
    int size;
    string name;
    vector<byte_type*> machine_code;
    vector<relocation_record*> reloc_table;
}general_section_record;

typedef vector<general_section_record*> gen_sec_table;

int resolve_program_argument(string arg);
program_arguments process_program_arguments(int argc, char* argv[]);
string remove_extra_spaces(string str);
bool comparePlaceArguments(place_argument arg1, place_argument arg2);

typedef struct FILE_SECTION{
    int file_index;
    section_record* section;
}file_section;

typedef struct ADDRESS_BYTES{
    uint address;
    vector<byte_type*> bytes;
}address_bytes;

class Linker{
private:
    vector<FILE*> input_objects;
    int current_input_number;
    vector<sec_table> section_tables;
    vector<sym_table> symbol_tables;
    vector<symbol_record*> general_symbol_table;
    gen_sec_table general_section_table;
    vector<place_argument> place_arguments;
    FILE* output_file;
    bool relocatable;
    object_tables get_sections_and_symbols(FILE*);
    vector<byte_type*> gather_data_from_machine_code(vector<word*> machine_code, int section_size);
    vector<word*> gather_pool_from_machine_code(vector<word*> machine_code, int section_size);
    section_record* get_section_by_name(int section_table_index, string section_name);
    void sort_place_arguments();
    bool section_in_place_arguments(string section_name);
    uint get_place_for_section(string section_name);
    gen_sec_table make_general_section_table(vector<section_record*> unmerged);
    gen_sec_table merge_sections(vector<section_record*> unmerged);
    void print_general_structures();
    bool defined_section_symbol(string name);
    symbol_record* get_symbol_by_name_and_section_index(string name, int ndx);
public:
    Linker(program_arguments arguments){
        if(arguments.contains_hex == true){
        this->input_objects = arguments.files;
        this->current_input_number = 0;
        this->place_arguments = arguments.place_arguments;
        this->output_file = arguments.output_file;
        relocatable = false;
        this->sort_place_arguments();
        }else{
            this->input_objects = arguments.files;
            this->current_input_number = 0;
            FILE* out = fopen("relocatable.o", "w");
            this->output_file = out;
            relocatable = true;
        }

        symbol_record* blanco_symbol = new symbol_record();
        blanco_symbol->number = 0;
        blanco_symbol->value = 0;
        blanco_symbol->type = "NOTYP";
        blanco_symbol->bind = "LOC";
        blanco_symbol->ndx = UND;
        blanco_symbol->name = "";
        this->general_symbol_table.push_back(blanco_symbol);
    }
    void read_from_files();
    void print_gathered_structures();
    void link();
    void make_object_file();
};

