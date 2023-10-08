#include "../inc/linker.hpp"

string remove_extra_spaces(string str){
    regex reg(R"(\s+)");
    return regex_replace(str, reg, " ");
}

int resolve_program_argument(string arg){
    if(arg == "-o"){
        return 1;
    }else if(arg.find("-place") != string::npos){
        return 2;
    }else if(arg == "-hex"){
        return 3;
    }else if(arg == "-relocatable"){
        return 4;
    }
    return 0; //file name
}

program_arguments process_program_arguments(int argc, char* argv[]){
    program_arguments args;
    args.contains_hex = false;
    args.contains_reloc = false;
    args.output_file = NULL;
    int i;
    for(i = 1; i < argc; i++){
        string argument = argv[i];
        switch (resolve_program_argument(argument))
        {
        case 1: //output file name option
            {
            i++;
            string output_file_name = argv[i];
            FILE* output_file = fopen(output_file_name.c_str(), "w");
            
            args.output_file = output_file;
            break;}
        case 2: //place option
            {
            int start_index = argument.find("=") + 1;
            string data_value_string = argument.substr(start_index);
            int monkey_sign_index = data_value_string.find("@");
            string section_name = data_value_string.substr(0, monkey_sign_index);
            string hex_value_string = data_value_string.substr(monkey_sign_index + 1);
            uint address = (uint)strtol(hex_value_string.c_str(), NULL, 0);
            //int address = stoi(hex_value_string, nullptr, 16);
            
            place_argument place;
            place.section_name = section_name;
            place.address = address;
            
            args.place_arguments.push_back(place);
            break;}
        case 3:
            {
            args.contains_hex = true;
            break;}
        case 4:
            {
            args.contains_reloc = true;
            break;}
        default: //input files
            {
            string file_name = argv[i];
            FILE* file = fopen(file_name.c_str(), "r");
            args.files.push_back(file);
            break;}
        }
    }
    return args;
}

vector<byte_type*> Linker :: gather_data_from_machine_code(vector<word*> machine_code, int section_size){
    vector<byte_type*> data;
    int counter = 0;
    //printf("SECTION start ********, given argument: %d\n", section_size);
    for(word* word : machine_code){
        if(counter >= section_size){   
            //printf("COUNTER: %d, SECTION SIZE: %d\n", counter, section_size);
            break;
        }else{
            int value = word->value;
            int x4 = (value >> (8*0)) & 0xff;
            int x3 = (value >> (8*1)) & 0xff;
            int x2 = (value >> (8*2)) & 0xff;
            int x1 = (value >> (8*3)) & 0xff;
            byte_type* first = new byte_type();
            byte_type* second = new byte_type();
            byte_type* third = new byte_type();
            byte_type* fourth = new byte_type();
            first->value = x1;
            second->value = x2;
            third->value = x3;
            fourth->value = x4;
            data.push_back(first);
            data.push_back(second);
            data.push_back(third);
            data.push_back(fourth);        
            counter += 4;
            //printf("CURR_COUNTER: %d\n", counter);
        }
    }
    //cout <<"SECTION.... DATA SIZE: " << data.size()<<endl;
    return data;
}

vector<word*> Linker :: gather_pool_from_machine_code(vector<word*> machine_code, int section_size){
    vector<word*> pool;
    int start_index = section_size / 4;
    //cout << "START INDEX FOR POOL IN MACHINE CODE " << start_index << endl;
    for(int i = start_index; i < machine_code.size();i++){
        pool.push_back(machine_code.at(i));
    }
    return pool;
}

object_tables Linker :: get_sections_and_symbols(FILE* file){
    sec_table table;
    sym_table sym_table;

    object_tables file_tables;
    
    char* text_temp = new char[MAX_LINE_SIZE];
    char* line = new char[MAX_LINE_SIZE];
    string line_str;
    int symbol_table_size;
    
    fgets(line, MAX_LINE_SIZE, file);
    line_str = line;
    
    sscanf(line_str.c_str(), "%s %d\n", text_temp, &symbol_table_size);
    
    //now gather all the symbols
    fgets(line, MAX_LINE_SIZE, file);

    string processed_line = line;
    processed_line = remove_extra_spaces(processed_line);
    sscanf(processed_line.c_str(), "%s %s %s %s %s %s %s\n", text_temp, text_temp, text_temp, text_temp, text_temp, text_temp, text_temp); //header of the table
    
    fgets(line, MAX_LINE_SIZE, file); //ommit first ZERO symbol, insert it staticly
    symbol_record* zero_symbol = new symbol_record();
    zero_symbol->number = 0;
    zero_symbol->value = 0;
    zero_symbol->type = "NOTYP";
    zero_symbol->bind = "LOC";
    zero_symbol->ndx = UND;
    zero_symbol->name = "";
    sym_table.push_back(zero_symbol);

    for(int i = 0; i < symbol_table_size - 1; i++){
        symbol_record* symbol = new symbol_record();
        string str_line;
        int num;
        int value;
        int size;
        char* type = new char[MAX_LINE_SIZE];
        char* bind = new char[MAX_LINE_SIZE];
        char* Ndx = new char[MAX_LINE_SIZE];
        string ndx_string;
        char* name = new char[MAX_LINE_SIZE];

        fgets(line, MAX_LINE_SIZE, file);
        str_line = line;
        str_line = remove_extra_spaces(str_line);
    
        sscanf(str_line.c_str(), "%d %d %d %s %s %s %s\n", &num, &value, &size, type, bind, Ndx, name);
        
        ndx_string = Ndx;
        symbol->number = num;
        symbol->value = value;
        symbol->type = type;
        symbol->bind = bind;
        
        symbol->ndx = (ndx_string == "UND" ? UND : stoi(ndx_string));
        symbol->name = name;
        sym_table.push_back(symbol);
    }
    //next to be processed is section table for the given file
    int number_of_sections;
    fgets(line, MAX_LINE_SIZE, file);
    processed_line = line;
    sscanf(processed_line.c_str(), "%s %d\n", text_temp, &number_of_sections);
    fgets(line, MAX_LINE_SIZE, file);
    //fscanf(file, "%s %s %s %s\n", text_temp, text_temp, text_temp, text_temp);
    for(int i = 0; i < number_of_sections; i++){
        section_record* section = new section_record();
        int num;
        int addr;
        int size;
        char* name = new char[MAX_LINE_SIZE];
        
        fgets(line, MAX_LINE_SIZE, file);
        processed_line = line;
        processed_line = remove_extra_spaces(processed_line);
        sscanf(processed_line.c_str(), "%d %d %d %s\n", &num, &addr, &size, name);
        
        section->number = num;
        section->address = addr;
        section->size = size;
        section->name = name;
        section->placed = false;

        table.push_back(section);
    }
    //now gather relocation records and machine code for each section
    for(section_record* section : table){
        int number_of_relocations;
        fgets(line, MAX_LINE_SIZE, file);
        processed_line = line;
        sscanf(processed_line.c_str(), "%s %s %d\n", text_temp, text_temp, &number_of_relocations);
        fgets(line, MAX_LINE_SIZE, file);
        //fscanf(file, "%s %s %s %s\n", text_temp, text_temp, text_temp, text_temp);
        for(int i = 0; i < number_of_relocations; i++){
            int offset;
            char* type = new char[MAX_LINE_SIZE];
            char* symbol = new char[MAX_LINE_SIZE];
            int addend;
            relocation_record* rel_rec = new relocation_record();
            fgets(line, MAX_LINE_SIZE, file);
            processed_line = line;
            processed_line = remove_extra_spaces(processed_line);
            sscanf(processed_line.c_str(), "%d %s %s %d\n",&offset, type, symbol, &addend);
            rel_rec->offset = offset;
            rel_rec->type = type;
            rel_rec->symbol = symbol;
            rel_rec->addend = addend;
            // cout << "RELOC FOR SECTION:"<< section->name << ", NUMBER: " << i << endl;
            // cout << offset << endl;
            // cout << type << endl;
            // cout << symbol << endl;
            // cout << addend << endl << endl;
            section->reloc_table.push_back(rel_rec);
        }
        int total_section_size;
        fgets(line, MAX_LINE_SIZE, file);
        processed_line = line;
        sscanf(processed_line.c_str(),"%s %d\n", text_temp, &total_section_size);
        
        //now we gather the code
        vector<word*> machine_code;
        int data_rows = (total_section_size % 8 != 0 ? total_section_size / 8 + 1 : total_section_size / 8);
        int m = 0;
        
        while(m < data_rows){
            if(m == data_rows - 1){
                if(total_section_size % 8 != 0){
                    int first;
                    word* first_word = new word();
                    
                    fgets(line, MAX_LINE_SIZE, file);
                    processed_line = line;
                    
                    sscanf(processed_line.c_str(), "%x\n", &first);
                    
                    first_word->value = first;
                    machine_code.push_back(first_word);
                }else{
                    int first, second;
                    word* first_word = new word();
                    word* second_word = new word();
                    
                    fgets(line, MAX_LINE_SIZE, file);
                    processed_line = line;
                    sscanf(processed_line.c_str(), "%x %x\n", &first, &second);
                    
                    first_word->value = first;
                    second_word->value = second;
                    machine_code.push_back(first_word);
                    machine_code.push_back(second_word);
                }
            }else{
                int first, second;
                word* first_word = new word();
                word* second_word = new word();
                
                fgets(line, MAX_LINE_SIZE, file);
                processed_line = line;
                sscanf(processed_line.c_str(), "%x %x\n", &first, &second);
                
                first_word->value = first;
                second_word->value = second;
                machine_code.push_back(first_word);
                machine_code.push_back(second_word);
            }
            m++;
        }
        //now we have all the machine code stored
        section->data = gather_data_from_machine_code(machine_code, section->size);
        section->pool = gather_pool_from_machine_code(machine_code, section->size);
        machine_code.clear();
    }
    file_tables.section_table = table;
    file_tables.symbol_table = sym_table;
    return file_tables;
}

void Linker :: read_from_files(){
    for(FILE* file : this->input_objects){
        object_tables tables = get_sections_and_symbols(file);
        this->section_tables.push_back(tables.section_table);
        this->symbol_tables.push_back(tables.symbol_table);
    }
}

void Linker :: print_gathered_structures(){
    FILE* gathered = fopen("zezanje.gathered", "w");
    for(int i = 0; i < this->input_objects.size();i++){
        sec_table section_tab = this->section_tables.at(i);
        sym_table symbol_tab = this->symbol_tables.at(i);
        fprintf(gathered, "%s %ld\n", "#.symtab", symbol_tab.size());
        fprintf(gathered, "%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n", "Num", "Value", "Size", "Type", "Bind", "Ndx", "Name");
        for(symbol_record* symbol : symbol_tab){
            fprintf(gathered, "%-15d %-15d %-15d %-15s %-15s %-15s %-15s\n", symbol->number, symbol->value, 0, (symbol->type).c_str(), (symbol->bind).c_str(), (symbol->ndx == 0 ? "UND" : (to_string(symbol->ndx)).c_str()), (symbol->name).c_str());
        }
        fprintf(gathered, "%s %ld\n", "#.sctntab", section_tab.size());
        fprintf(gathered, "%-15s %-15s %-15s %-15s\n", "Num", "Address", "Size", "Name");
        for(section_record* section : section_tab){
            fprintf(gathered, "%-15d %-15d %-15d %-15s\n", section->number, section->address, section->size, (section->name).c_str());
        }
        for(section_record* section : section_tab){
            fprintf(gathered, "%s %ld\n", ("#.rela " + section->name).c_str(), section->reloc_table.size());
            fprintf(gathered, "%-15s %-15s %-15s %-15s\n", "Offset", "Type", "Symbol", "Addend");
            for(reloc_record* rel_rec : section->reloc_table){
                fprintf(gathered, "%-15ld %-15s %-15s %-15d\n", rel_rec->offset, (rel_rec->type).c_str(), (rel_rec->symbol).c_str(), rel_rec->addend);
            }
            fprintf(gathered, "%s %ld\n", ("#." + section->name).c_str(), (section->size + section->pool.size()*4));
            int i = 0;
            for(byte_type* B : section->data){
                if(i == 3){
                    fprintf(gathered, "%02x ", (unsigned int)(B->value & 0xff));
                    i++;
                }else if(i == 7){
                    i = 0;
                    fprintf(gathered, "%02x\n", (unsigned int)(B->value & 0xff));
                }else{
                    fprintf(gathered, "%02x", (unsigned int)(B->value & 0xff));
                    i++;
                }
            }
            if(section->pool.size() != 0){
                if(i % 8 == 0)
                    i = 1;
                else
                    i = 2;
                for(word* W : section->pool){
                    if(i % 2 == 0){
                        fprintf(gathered, "%08x\n", W->value);
                    }else{
                        fprintf(gathered, "%08x ", W->value);
                    }
                    i++;
                    }
                    if(i % 2 == 0)
                        fprintf(gathered, "\n");
                    }else{
                        if(section->size % 8 != 0){
                            fprintf(gathered, "\n");
                        }
                    }
        }
        fprintf(gathered, "************************************************************************************************\n");
    }
    fclose(gathered);
}

section_record* Linker :: get_section_by_name(int section_table_index, string section_name){
    if(section_table_index < 0 || section_table_index >= this->section_tables.size())
        return NULL;
    sec_table table = this->section_tables.at(section_table_index);
    for(section_record* section : table){
        if(section->name == section_name)
            return section;
    }
    return NULL;
}

bool comparePlaceArguments(place_argument arg1, place_argument arg2){
    return (arg1.address < arg2.address);
}

void Linker :: sort_place_arguments(){
    sort(this->place_arguments.begin(), this->place_arguments.end(), comparePlaceArguments);
}

bool Linker :: section_in_place_arguments(string section_name){
    for(place_argument arg : this->place_arguments){
        if(arg.section_name == section_name)
            return true;
    }
    return false;
}

uint Linker :: get_place_for_section(string section_name){
    for(place_argument arg : this->place_arguments){
        if(arg.section_name == section_name)
            return arg.address;
    }
    return 0;
}

bool section_in_vector(vector<section_record*> sections, string name){
    for(section_record* section : sections){
        if(section->name == name)
            return true;
    }
    return false;
}

section_record* get_last_section_from_vector(vector<section_record*> sections, string name){
    for(int i = sections.size() - 1; i >= 0; i--){
        section_record* section = sections.at(i);
        if(section->name == name)
            return section;
    }
    return NULL;
}

bool compareSectionAddresses(section_record* first, section_record* second){
    if((uint)first->address != (uint)second->address){
        return ((uint)first->address < (uint)second->address);
    }else{
        return (first->number < second->number);
    }
}

general_section_record* get_general_section_by_name(gen_sec_table general_section_table, string name){
    for(general_section_record* section : general_section_table){
        if(section->name == name)
            return section;
    }
    return NULL;
}

vector<byte_type*> get_machine_code(vector<byte_type*> data, vector<word*> pool){
    vector<byte_type*> code;
    code = data;
    for(word* w : pool){
        int value = w->value;
        int x4 = (value >> (8*0)) & 0xff;
        int x3 = (value >> (8*1)) & 0xff;
        int x2 = (value >> (8*2)) & 0xff;
        int x1 = (value >> (8*3)) & 0xff;
        byte_type* first = new byte_type();
        byte_type* second = new byte_type();
        byte_type* third = new byte_type();
        byte_type* fourth = new byte_type();
        first->value = x1;
        second->value = x2;
        third->value = x3;
        fourth->value = x4;
        code.push_back(first);
        code.push_back(second);
        code.push_back(third);
        code.push_back(fourth);
    }
    return code;
}

gen_sec_table Linker :: make_general_section_table(vector<section_record*> sections){
    gen_sec_table general_section_table;
    for(section_record* section : sections){
        general_section_record* new_section = new general_section_record();
        new_section->address = section->address;
        new_section->name = section->name;

        new_section->reloc_table = section->reloc_table;
            
        new_section->size = section->size;
        new_section->number = general_section_table.size() + 1;
        new_section->machine_code = get_machine_code(section->data, section->pool);
        general_section_table.push_back(new_section);
}
return general_section_table;
}

gen_sec_table Linker :: merge_sections(vector<section_record*> unmerged){
    gen_sec_table general_section_table;
    for(section_record* section : unmerged){
        general_section_record* general_section = get_general_section_by_name(general_section_table, section->name);
        if(!general_section){ //sections first appearance
            general_section_record* new_section = new general_section_record();
            new_section->address = section->address;
            new_section->name = section->name;

            new_section->reloc_table = section->reloc_table;
            
            new_section->size = section->size;
            new_section->number = general_section_table.size() + 1;
            new_section->machine_code = get_machine_code(section->data, section->pool);
            general_section_table.push_back(new_section);
        }else{ //section exists, append content
            vector<byte_type*> to_append = get_machine_code(section->data, section->pool);
            general_section->size += section->size;
            //append machine code; update relocation_records offsets; append relocation records
            for(byte_type* byte : to_append){
                general_section->machine_code.push_back(byte);
            }
    
            for(relocation_record* rel_rec : section->reloc_table){
                general_section->reloc_table.push_back(rel_rec);
            }
        }
    }
    return general_section_table;
}

vector<section_record*> update_reloc_records(vector<section_record*> place_sections){
    for(section_record* section : place_sections){
        //cout << "SECTION " << section->name << ", " << std::hex << section->address << endl;
        for(reloc_record* rel_rec : section->reloc_table){
            rel_rec->offset = (uint)(rel_rec->offset + section->address);
        }
    }
    return place_sections;
}

void Linker :: print_general_structures(){
    FILE* temp_output = fopen("relocatable.o", "w");
    fprintf(temp_output, "%s %ld\n", "#.symtab", this->general_symbol_table.size());
    fprintf(temp_output, "%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n", "Num", "Value", "Size", "Type", "Bind", "Ndx", "Name");
    for(symbol_record* symbol : this->general_symbol_table){
        fprintf(temp_output, "%-15d %-15x %-15d %-15s %-15s %-15s %-15s\n", symbol->number, (uint)symbol->value, 0, (symbol->type).c_str(), (symbol->bind).c_str(), (symbol->ndx == 0 ? "UND" : (to_string(symbol->ndx)).c_str()), (symbol->name).c_str());
    }
    fprintf(temp_output, "%s %ld\n", "#.sctntab", this->general_section_table.size());
    fprintf(temp_output, "%-15s %-15s %-15s %-15s\n", "Num", "Address", "Size", "Name");
    for(general_section_record* section : this->general_section_table){
        fprintf(temp_output, "%-15d %-15x %-15d %-15s\n", section->number, (uint)section->address, section->size, (section->name).c_str());
    }
    for(general_section_record* section : this->general_section_table){
        fprintf(temp_output, "%s %ld\n", ("#.rela " + section->name).c_str(), section->reloc_table.size());
        fprintf(temp_output, "%-15s %-15s %-15s %-15s\n", "Offset", "Type", "Symbol", "Addend");
        for(reloc_record* rel_rec : section->reloc_table){
        fprintf(temp_output, "%-15x %-15s %-15s %-15d\n", (uint)rel_rec->offset, (rel_rec->type).c_str(), (rel_rec->symbol).c_str(), rel_rec->addend);
        }
        fprintf(temp_output, "%s %ld\n", ("#." + section->name).c_str(), (section->machine_code.size()));
        int i = 0;
        for(byte_type* B : section->machine_code){
        if(i == 3){
            fprintf(temp_output, "%02x ", (unsigned int)(B->value & 0xff));
            i++;
        }else if(i == 7){
            i = 0;
            fprintf(temp_output, "%02x\n", (unsigned int)(B->value & 0xff));
        }else{
            fprintf(temp_output, "%02x", (unsigned int)(B->value & 0xff));
            i++;
            }
            }
            if(section->machine_code.size() % 8 != 0){
                fprintf(temp_output, "\n");
            }
    }
    fclose(temp_output);
}

bool Linker :: defined_section_symbol(string name){
    for(auto symbol : this->general_symbol_table){
        if(symbol->type == "SCTN" && symbol->name == name)
            return true;
    }
    return false;
}

uint get_address_for_section_and_table_index(string name, int ind, vector<file_section*> structure){
    //cout << "SECTION: " << name << " for index: " << ind << endl;
    for(auto obj : structure){
        if(obj->section->name == name && obj->file_index == ind){
            //cout << "FOUND ADDRESS: " << std::hex << (uint)obj->section->address << endl;
            return (uint)(obj->section->address);
        }
    }
    cout << "NO ADDRESS FOR GIVEN NAME AND INDEX IN SYMBOL RESOLUTION PART OF LINKING\n";
    return 0;
}

symbol_record* Linker :: get_symbol_by_name_and_section_index(string name, int ndx=0){
    for(auto symbol : this->general_symbol_table){
        if(symbol->name == name){
            return symbol;
        }
    }
    cout << "THERE IS NO SYMBOL LIKE THAT" << endl;
    return NULL;
}

void Linker :: link(){
    vector<file_section*> files_and_sections; //structure that holds table index along side the section -> used for symbol resolution part
    vector<section_record*> place_sections; //vector of newly created sections that will go to the global section table
    int table_count = 0;
    
    if(this->relocatable){
        for(sec_table section_table : this->section_tables){
        table_count++;
        for(section_record* section : section_table){
                if(section_in_vector(place_sections, section->name)){ //already placed in vector
                    section_record* placed_section = get_last_section_from_vector(place_sections, section->name);
                    
                    section_record* sec_new = new section_record();
                    sec_new->address = (uint)(placed_section->address + placed_section->size + placed_section->pool.size()*4);
                    sec_new->number = 0;
                    sec_new->name = section->name;
                    sec_new->placed = true;
                    section->placed = true;
                    sec_new->size = section->size;
                    sec_new->data = section->data;
                    sec_new->pool = section->pool;
                    sec_new->reloc_table = section->reloc_table;
                    
                    place_sections.push_back(sec_new);
                    
                    file_section* obj = new file_section();
                    obj->file_index = table_count;
                    obj->section = sec_new;
                    files_and_sections.push_back(obj);
                }else{
                    section_record* sec_new = new section_record();
                    sec_new->address = 0;
                    sec_new->number = 0;
                    sec_new->name = section->name;
                    sec_new->placed = true;
                    section->placed = true;
                    sec_new->size = section->size;
                    sec_new->data = section->data;
                    sec_new->pool = section->pool;
                    sec_new->reloc_table = section->reloc_table;
                    
                    place_sections.push_back(sec_new);
                    
                    file_section* obj = new file_section();
                    obj->file_index = table_count;
                    obj->section = sec_new;
                    files_and_sections.push_back(obj);
                }
        }   
    }

    place_sections = update_reloc_records(place_sections);
    this->general_section_table = merge_sections(place_sections);

    // for(auto sec : this->general_section_table){
    //     printf("----------------------------------------------------------------------------\n");
    //     printf("%d \t%08x \t%d \t%s\n", sec->number, sec->address, sec->size, sec->name.c_str());
    //     printf("****************************************************************************\n");
    //     for(auto rr : sec->reloc_table){
    //         printf("%ld \t%s \t%s \t%d\n", rr->offset, rr->type.c_str(), rr->symbol.c_str(), rr->addend);
    //     }
    // }
    
    int i;
    vector<symbol_record*> undefined_symbols;
    for(i = 0; i < this->symbol_tables.size();i++){
        sym_table table = symbol_tables.at(i);
        for(symbol_record* symbol : table){
            if(symbol->number == 0)
                continue;
            if(symbol->bind == "LOC" || (symbol->bind == "GLOB" && symbol->ndx != UND)){
                int section_index = symbol->ndx;
                sec_table section_table = this->section_tables.at(i);
                section_record* section = section_table.at(section_index - 1);
                general_section_record* general_section = get_general_section_by_name(this->general_section_table,section->name);
                if(symbol->type == "SCTN" && !defined_section_symbol(symbol->name)){
                    symbol_record* new_symbol = new symbol_record();
                    //new_symbol->number = this->general_symbol_table.size();
                    new_symbol->number = i + 1;
                    new_symbol->value = (uint)general_section->address;
                    new_symbol->type = symbol->type;
                    new_symbol->bind = symbol->bind;
                    new_symbol->ndx = general_section->number;
                    new_symbol->name = symbol->name;
                
                    this->general_symbol_table.push_back(new_symbol);
                }else if(symbol->type == "SCTN" && defined_section_symbol(symbol->name)){
                    //do nothing ?
                }else{
                    symbol_record* new_symbol = new symbol_record();
                    //new_symbol->number = this->general_symbol_table.size();
                    new_symbol->number = i + 1;
                    new_symbol->value = symbol->value;
                    new_symbol->type = symbol->type;
                    new_symbol->bind = symbol->bind;
                    new_symbol->ndx = general_section->number;
                    new_symbol->name = symbol->name;
                
                    this->general_symbol_table.push_back(new_symbol);
                }
                
            }else{
                //add to the table of undefined symbols
                symbol_record* new_symbol = new symbol_record();
                new_symbol->number = symbol->number;
                new_symbol->value = symbol->value;
                new_symbol->type = symbol->type;
                new_symbol->bind = symbol->bind;
                new_symbol->ndx = symbol->ndx;
                new_symbol->name = symbol->name;
                undefined_symbols.push_back(new_symbol);
            }
        }
    }
    i = 0;
    for(symbol_record* symbol : general_symbol_table){
        if(i == 0){
            i++;
            continue;
        }
        general_section_record* general_section = this->general_section_table.at(symbol->ndx - 1);
        if(symbol->type == "SCTN"){
            //do nothing, already a good value
        }else{
            symbol->value = (uint)(symbol->value + get_address_for_section_and_table_index(general_section->name, symbol->number, files_and_sections));
        }
        symbol->number = i;
        i++;
    }

    return;
    }
    //for -hex option
    for(sec_table section_table : this->section_tables){
        table_count++;
        for(section_record* section : section_table){
            if(section_in_place_arguments(section->name)){
                if(section_in_vector(place_sections, section->name)){ //already placed in vector
                    section_record* placed_section = get_last_section_from_vector(place_sections, section->name);
                    
                    section_record* sec_new = new section_record();
                    sec_new->address = (uint)(placed_section->address + placed_section->size + placed_section->pool.size()*4);
                    sec_new->number = 0;
                    sec_new->name = section->name;
                    sec_new->placed = true;
                    section->placed = true;
                    sec_new->size = section->size;
                    sec_new->data = section->data;
                    sec_new->pool = section->pool;
                    
                    sec_new->reloc_table = section->reloc_table;
                    
                    place_sections.push_back(sec_new);
                    
                    file_section* obj = new file_section();
                    obj->file_index = table_count;
                    obj->section = sec_new;
                    files_and_sections.push_back(obj);
                }else{
                    section_record* sec_new = new section_record();
                    sec_new->address = get_place_for_section(section->name);
                    sec_new->number = 0;
                    sec_new->name = section->name;
                    sec_new->placed = true;
                    section->placed = true;
                    sec_new->size = section->size;
                    // sec_new->data = copy_section_data(section->data);
                    // sec_new->pool = copy_secion_pool(section->pool);
                    // sec_new->reloc_table = copy_section_reloctab(section->reloc_table);
                    sec_new->data = section->data;
                    sec_new->pool = section->pool;
                    sec_new->reloc_table = section->reloc_table;
                    
                    place_sections.push_back(sec_new);
                    
                    file_section* obj = new file_section();
                    obj->file_index = table_count;
                    obj->section = sec_new;
                    files_and_sections.push_back(obj);
                }
            }
        }   
    }
    sort(place_sections.begin(), place_sections.end(), compareSectionAddresses);

    //now we have all "placed" sections in private vector of sections;
    uint first_address_unplaced = (uint)(place_sections.back()->address + place_sections.back()->size + place_sections.back()->pool.size()*4);
    bool first_non_placed = true;
    table_count = 0;
    for(sec_table section_table : this->section_tables){ //go trough all section tables
        table_count++;
        for(section_record* section : section_table){ //go trough all sections in a signle table
            if(!section->placed){ // add to vector of sections
                if(section_in_vector(place_sections, section->name)){ //already placed in vector
                    section_record* placed_section = get_last_section_from_vector(place_sections, section->name);
                    section_record* sec_new = new section_record();
                    sec_new->address = (uint)(placed_section->address + placed_section->size + placed_section->pool.size()*4);
                    sec_new->number = 0;
                    sec_new->name = section->name;
                    sec_new->placed = true;
                    sec_new->size = section->size;
                    sec_new->data = section->data;
                    sec_new->pool = section->pool;
                    sec_new->reloc_table = section->reloc_table;
                    place_sections.push_back(sec_new);
                    sort(place_sections.begin(), place_sections.end(), compareSectionAddresses);

                    file_section* obj = new file_section();
                    obj->file_index = table_count;
                    obj->section = sec_new;
                    files_and_sections.push_back(obj);
                }else{ //first time to see this section
                    section_record* sec_new = new section_record();
                    if(first_non_placed == true){
                        sec_new->address = first_address_unplaced;
                        first_non_placed = false;
                    }else{
                        sec_new->address = (uint)(place_sections.back()->address + place_sections.back()->size + place_sections.back()->pool.size()*4);
                    }
                    sec_new->number = 0;
                    sec_new->name = section->name;
                    sec_new->placed = true;
                    sec_new->size = section->size;
                    // sec_new->data = copy_section_data(section->data);
                    // sec_new->pool = copy_secion_pool(section->pool);
                    // sec_new->reloc_table = copy_section_reloctab(section->reloc_table);
                    sec_new->data = section->data;
                    sec_new->pool = section->pool;
                    sec_new->reloc_table = section->reloc_table;
                    place_sections.push_back(sec_new);
                    sort(place_sections.begin(), place_sections.end(), compareSectionAddresses);

                    file_section* obj = new file_section();
                    obj->file_index = table_count;
                    obj->section = sec_new;
                    files_and_sections.push_back(obj);
                }
            }
        }   
    }
    
    //at this moment we have all sections stored in vector, now we have to merge them in one global section table
    place_sections = update_reloc_records(place_sections);
    this->general_section_table = merge_sections(place_sections);

    //sections are merged, symbol resolution begins
    int i;
    vector<symbol_record*> undefined_symbols;
    for(i = 0; i < this->symbol_tables.size();i++){
        sym_table table = symbol_tables.at(i);
        for(symbol_record* symbol : table){
            if(symbol->number == 0)
                continue;
            if(symbol->bind == "LOC" || (symbol->bind == "GLOB" && symbol->ndx != UND)){
                int section_index = symbol->ndx;
                sec_table section_table = this->section_tables.at(i);
                section_record* section = section_table.at(section_index - 1);
                general_section_record* general_section = get_general_section_by_name(this->general_section_table,section->name);
                if(symbol->type == "SCTN" && !defined_section_symbol(symbol->name)){
                    symbol_record* new_symbol = new symbol_record();
                    //new_symbol->number = this->general_symbol_table.size();
                    new_symbol->number = i + 1;
                    new_symbol->value = (uint)general_section->address;
                    new_symbol->type = symbol->type;
                    new_symbol->bind = symbol->bind;
                    new_symbol->ndx = general_section->number;
                    new_symbol->name = symbol->name;
                
                    this->general_symbol_table.push_back(new_symbol);
                }else if(symbol->type == "SCTN" && defined_section_symbol(symbol->name)){
                    //do nothing ?
                }else{
                    symbol_record* new_symbol = new symbol_record();
                    //new_symbol->number = this->general_symbol_table.size();
                    new_symbol->number = i + 1;
                    new_symbol->value = symbol->value;
                    new_symbol->type = symbol->type;
                    new_symbol->bind = symbol->bind;
                    new_symbol->ndx = general_section->number;
                    new_symbol->name = symbol->name;
                
                    this->general_symbol_table.push_back(new_symbol);
                }
                
            }else{
                //add to the table of undefined symbols
                symbol_record* new_symbol = new symbol_record();
                new_symbol->number = symbol->number;
                new_symbol->value = symbol->value;
                new_symbol->type = symbol->type;
                new_symbol->bind = symbol->bind;
                new_symbol->ndx = symbol->ndx;
                new_symbol->name = symbol->name;
                undefined_symbols.push_back(new_symbol);
            }
        }
    }
    i = 0;
    for(symbol_record* symbol : general_symbol_table){
        if(i == 0){
            i++;
            continue;
        }
        general_section_record* general_section = this->general_section_table.at(symbol->ndx - 1);
        if(symbol->type == "SCTN"){
            //do nothing, already a good value
        }else{
            symbol->value = (uint)(symbol->value + get_address_for_section_and_table_index(general_section->name, symbol->number, files_and_sections));
        }
        symbol->number = i;
        i++;
    }
    //this->print_general_structures();

    //the last thing to do in the process of linking is to execute relocation records
    for(auto section : this->general_section_table){
        for(auto rel_rec : section->reloc_table){
            int place_in_code = ((uint)(rel_rec->offset - section->address));
            string symbol_name = rel_rec->symbol;
            symbol_name = symbol_name.substr(1, symbol_name.size() - 2);
            int value = get_symbol_by_name_and_section_index(symbol_name, section->number)->value + rel_rec->addend;
            //cout << "REL REC FOR SECTION " << section->name << ", SYMBOL TO RESOLVE " << symbol_name << ", ON ADDRESS " << std::hex << rel_rec->offset << ", PUT VALUE  " << value << endl; 
            int x1 = (value >> (8*0)) & 0xff;
            int x2 = (value >> (8*1)) & 0xff;
            int x3 = (value >> (8*2)) & 0xff;
            int x4 = (value >> (8*3)) & 0xff;
            
            vector<byte_type*> mac_code;
            for(int i = 0; i < section->machine_code.size(); i++){
                if(i == place_in_code){
                    byte_type* b1 = new byte_type();
                    b1->value = x1;
                    byte_type* b2 = new byte_type();
                    b2->value = x2;
                    byte_type* b3 = new byte_type();
                    b3->value = x3;
                    byte_type* b4 = new byte_type();
                    b4->value = x4;
                    mac_code.push_back(b1);
                    mac_code.push_back(b2);
                    mac_code.push_back(b3);
                    mac_code.push_back(b4);
                    i += 3;
                }else{
                    mac_code.push_back(section->machine_code.at(i));
                }
            }
            section->machine_code = mac_code;
        }
    }
}

void Linker :: make_object_file(){ //make hex file
    vector<address_bytes*> structure;
    if(this->relocatable){
        this->print_general_structures();
        return;
    }
    this->print_general_structures();
    for(general_section_record* section : this->general_section_table){
        uint address = section->address;
        for(int i = 0; i < section->machine_code.size()/8; i++){
            address_bytes* obj = new address_bytes();
            obj->address = address;
            obj->bytes.push_back(section->machine_code.at(8*i + 0));
            obj->bytes.push_back(section->machine_code.at(8*i + 1));
            obj->bytes.push_back(section->machine_code.at(8*i + 2));
            obj->bytes.push_back(section->machine_code.at(8*i + 3));
            obj->bytes.push_back(section->machine_code.at(8*i + 4));
            obj->bytes.push_back(section->machine_code.at(8*i + 5));
            obj->bytes.push_back(section->machine_code.at(8*i + 6));
            obj->bytes.push_back(section->machine_code.at(8*i + 7));
            address += 8;
            structure.push_back(obj);
        }
    }
    for(auto obj : structure){
        fprintf(this->output_file, "%08x: %02x %02x %02x %02x %02x %02x %02x %02x\n",(uint)obj->address ,(uint)(obj->bytes.at(0)->value & 0xff), (uint)(obj->bytes.at(1)->value & 0xff), (uint)(obj->bytes.at(2)->value & 0xff), (uint)(obj->bytes.at(3)->value & 0xff), (uint)(obj->bytes.at(4)->value & 0xff), (uint)(obj->bytes.at(5)->value & 0xff), (uint)(obj->bytes.at(6)->value & 0xff), (uint)(obj->bytes.at(7)->value & 0xff));
    }
}

int main(int argc, char* argv[]){
    program_arguments arguments = process_program_arguments(argc, argv);
    if(arguments.contains_hex == false && arguments.contains_reloc == false)
    {
        cout << "HEX OR RELOCATABLE OPTION MISSING" << endl;
        exit(-1);
    }
    if(arguments.files.size() == 0)
    {
        cout << "INPUT FILES MISSING" << endl;
        exit(-1);
    }
    if(arguments.contains_hex == true && arguments.contains_reloc == true)
    {
        cout << "BOTH HEX AND RELOCATABLE OPTION PROVIDED" << endl;
        exit(-1);
    }
    Linker* linker = new Linker(arguments);
    linker->read_from_files();
    linker->link();
    linker->make_object_file();
    cout << "****************************************************************\nExiting without errors!\n****************************************************************\n";
    exit(0);
}
