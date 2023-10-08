#include "../inc/as.hpp"
#include "../misc/lexer.hpp"
#include "../misc/parser.hpp"
/*
  gprX (shown in pseudo processor instructions) are accessed from instruction arguments, while regX (shown in "operand") are accessed from instruction operand field 
*/

char get_register_number(string register_name){
  if(register_name == "sp")
    return 14;
  if(register_name == "pc")
    return 15;
  if(register_name == "status")
    return 0;
  if(register_name == "handler")
    return 1;
  if(register_name == "cause")
    return 2;
  register_name.erase(0, 1);
  return stoi(register_name);
}

int Assembler::resolve_operand_type(operand* op){
  if(op->reg == NULL){
    if(op->symbol == "" && op->memory == false && op->value_present == true){
      return 1;
    }else if(op->symbol != "" && op->memory == false && op->value_present == false && op->memory == false){
      return 2;
    }else if(op->symbol == "" && op->memory == true && op->value_present == false){
      return 3;
    }else if(op->symbol != "" && op->memory == true && op->value_present == false){
      return 4;
    }
  }else{
    if(op->value == NONE && op->memory == false && op->value_present == false){
      return 5;
    }else if(op->value == NONE && op->memory == true && op->value_present == false && op->symbol == ""){
      return 6;
    }else if(op->value != NONE && op->memory == true && op->value_present == false){
      return 7;
    }else if(op->value == NONE && op->memory == true && op->value_present == false && op->symbol != ""){
      return 8;
    }
  }
  return 0;
}

void Assembler::write_machine_code(byte_type* first, byte_type* second, byte_type* third, byte_type* fourth){
  this->section_table.at(current_section_number - 1)->data.push_back(first);
  this->section_table.at(current_section_number - 1)->data.push_back(second);
  this->section_table.at(current_section_number - 1)->data.push_back(third);
  this->section_table.at(current_section_number - 1)->data.push_back(fourth);
}

int Assembler::get_offset_to_pool(){ //addressable unit is a byte, so the offset to the place in pool is expressed in bytes
  section_record* section = this->section_table.at(current_section_number - 1);
  return section->size + (section->pool.size() - 1)*4;
}

void Assembler::add_symbol(symbol_record* symbol){
  this->symbol_table.push_back(symbol);
}

symbol_record* make_symbol(int number, long value, string type, string bind, int ndx, string name){
  symbol_record* sym = new symbol_record();
  sym->number = number;
  sym->value = value;
  sym->type = type;
  sym->bind = bind;
  sym->ndx = ndx;
  sym->name = name;

  return sym;
}

symbol_record* Assembler :: get_symbol_by_name(string sym_name){
  //cout << "\nTrying to find symbol with the name " << sym_name << endl;
  for(symbol_record* record : this->symbol_table){
    if(record->name == sym_name)
      return record;
  }
  //cout << "\nNo such symbol\n";
  return nullptr;
}

void Assembler :: add_section(section_record* section_rec){
  this->section_table.push_back(section_rec);
}

section_record* make_section_record(int number, int address, string name){
  section_record* section_rec = new section_record();
  section_rec->number = number;
  section_rec->address = address;
  section_rec->name = name;
  section_rec->placed = false;
  return section_rec;
}

void Assembler :: align_sections(){
  for(section_record* section : this->section_table){
    int total_size = section->size + section->pool.size()*4;
    for(int i = 0; i < ((total_size % 8)/4); i++){
      word* ones = new word();
      ones->value = 0xffffffff;
      section->pool.push_back(ones);
    }
  }
}

int Assembler :: process_instruction_and_update_counter(instr* instruction, int locationCounter){
  string instr_name = instruction->name;
  section_record* current_section = this->section_table.at(current_section_number - 1);
  if(instr_name == "halt"){ // 00000000 00000000 00000000 00000000
    byte_type* zero = new byte_type();
    zero->value = 0;
    write_machine_code(zero, zero, zero, zero);
    return 4;
  }else if(instr_name == "int"){ // 00010000 00000000 00000000 00000000
    byte_type* zero = new byte_type();
    zero->value = 0;
    byte_type* last = new byte_type();
    last->value = 16;
    write_machine_code(zero, zero, zero, last);
    return 4;
  }else if(instr_name == "iret"){ // 10010110 00001110 00000000 00000100 | 10010011 11111110 00000000 00001000
    // iret <==> pop pc; pop status;
    // status <= mem32[sp + 4] <=> csrA <= mem32[gprB + gprC + D]
    // pc <= mem32[sp]; sp <= sp + 8 <=> gprA <= mem32[gprB]; gprB <= gprB + D;
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0x04;
    second->value = 0;
    third->value = 0x0e;
    fourth->value = 0b10010110;
    write_machine_code(first, second, third, fourth);
    // status now holds its value
    byte_type* first1 = new byte_type();
    byte_type* second1 = new byte_type();
    byte_type* third1 = new byte_type();
    byte_type* fourth1 = new byte_type();
    first1->value = 0x08;
    second1->value = 0;
    third1->value = 0xfe;
    fourth1->value = 0b10010011;
    write_machine_code(first1, second1, third1, fourth1);
    return 8;
  }else if(instr_name == "call"){ // 00100001 11110000 0000dddd dddddddd
    //push pc; //does the emulator
    //pc <= mem32[gprA + gprB + D]
    
    byte_type* first1 = new byte_type();
    byte_type* second1 = new byte_type();
    byte_type* third1 = new byte_type();
    byte_type* fourth1 = new byte_type();
    
    if(instruction->arguments->op->symbol != ""){ //symbol
      relocation_record* rel_rec = new relocation_record();
      symbol_record* symbol = get_symbol_by_name(instruction->arguments->op->symbol);
      if(!symbol){
        cout << "ERROR: CALL INSTRUCTION ERROR -> USING UNDEFINED SYMBOL " << instruction->arguments->op->symbol;
        return 0;
      }
      word* zero = new word();
      zero->value = 0;
      this->section_table.at(current_section_number - 1)->pool.push_back(zero);
      
      if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = "(" + symbol->name + ")";
        rel_rec->addend = 0;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }else{ //local symbol

        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = symbol->name;;
        rel_rec->addend = -1;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }
      
      int pool_offset = get_offset_to_pool() - 4 - locationCounter; // minus 4 because Program Counter (PC register) always points 4 Bytes in advance (to the next instruction to be read and executed)
      first1->value = pool_offset & 0x000000ff;
      second1->value = (pool_offset & 0x00000f00) >> 8;
      third1->value = 0xf0;
      fourth1->value = 0b00100001;
      write_machine_code(first1, second1, third1, fourth1);
    }else{
      int literal = instruction->arguments->op->value;
      section_record* current = this->section_table.at(current_section_number - 1);
      word* literal_word = new word();
      literal_word->value = little_endian_representation(literal);
      current->pool.push_back(literal_word);

      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first1->value = pool_offset & 0x000000ff;
      second1->value = (pool_offset & 0x00000f00) >> 8;
      third1->value = 0xf0;
      fourth1->value = 0b00100001;
      write_machine_code(first1, second1, third1, fourth1);
    }
    return 4;
  }else if(instr_name == "ret"){ // 10010011 11111110 00000000 00000100
    //pop pc; <==> pc(15) <= mem32[sp(14)]; sp(14) <= sp(14) + 4
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0x04;
    second->value = 0;
    third->value = 0xfe;
    fourth->value = 0b10010011;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "jmp"){
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    if(instruction->arguments->op->symbol != ""){
      relocation_record* rel_rec = new relocation_record();
      symbol_record* symbol = get_symbol_by_name(instruction->arguments->op->symbol);
      word* zero = new word();
      zero->value = 0;
      this->section_table.at(current_section_number - 1)->pool.push_back(zero);
      if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = "(" + symbol->name + ")";
        rel_rec->addend = 0;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }else{ //local symbol

        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = symbol->name;;
        rel_rec->addend = -1;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (pool_offset & 0x00000f00) >> 8;
      third->value = 0xf0;
      fourth->value = 0b00111000;
      write_machine_code(first, second, third, fourth);
    }else{
      int literal = instruction->arguments->op->value;
      section_record* current = this->section_table.at(current_section_number - 1);
      word* literal_word = new word();
      literal_word->value = little_endian_representation(literal);
      current->pool.push_back(literal_word);

      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (pool_offset & 0x00000f00) >> 8;
      third->value = 0xf0;
      fourth->value = 0b00111000;
      write_machine_code(first, second, third, fourth);
    }
    return 4;
  }else if(instr_name == "beq"){
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    char reg1 = get_register_number(instruction->arguments->registers->name); //gpr1
    char reg2 = get_register_number(instruction->arguments->registers->next->name); //gpr2
    if(instruction->arguments->op->symbol != ""){
      relocation_record* rel_rec = new relocation_record();
      symbol_record* symbol = get_symbol_by_name(instruction->arguments->op->symbol);
      word* zero = new word();
      zero->value = 0;
      this->section_table.at(current_section_number - 1)->pool.push_back(zero);
      if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = "(" + symbol->name + ")";
        rel_rec->addend = 0;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }else{ //local symbol

        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = symbol->name;;
        rel_rec->addend = -1;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (reg2 << 4) | ((pool_offset & 0x00000f00) >> 8);
      third->value = reg1 | 0xf0;
      fourth->value = 0b00111001;
      write_machine_code(first, second, third, fourth);
    }else{ //literal
      int literal = instruction->arguments->op->value;
      section_record* current = this->section_table.at(current_section_number - 1);
      word* literal_word = new word();
      literal_word->value = little_endian_representation(literal);
      current->pool.push_back(literal_word);

      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (reg2 << 4) | ((pool_offset & 0x00000f00) >> 8);
      third->value = reg1 | 0xf0;
      fourth->value = 0b00111001;
      write_machine_code(first, second, third, fourth);
    }
    return 4;
  }else if(instr_name == "bne"){
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    char reg1 = get_register_number(instruction->arguments->registers->name); //gpr1
    char reg2 = get_register_number(instruction->arguments->registers->next->name); //gpr2

    if(instruction->arguments->op->symbol != ""){
      relocation_record* rel_rec = new relocation_record();
      symbol_record* symbol = get_symbol_by_name(instruction->arguments->op->symbol);
      word* zero = new word();
      zero->value = 0;
      this->section_table.at(current_section_number - 1)->pool.push_back(zero);
      if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = "(" + symbol->name + ")";
        rel_rec->addend = 0;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }else{ //local symbol

        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = symbol->name;;
        rel_rec->addend = -1;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (reg2 << 4) | ((pool_offset & 0x00000f00) >> 8);
      third->value = reg1 | 0xf0;
      fourth->value = 0b00111010;
      write_machine_code(first, second, third, fourth);
    }else{ //literal
      int literal = instruction->arguments->op->value;
      section_record* current = this->section_table.at(current_section_number - 1);
      word* literal_word = new word();
      literal_word->value = little_endian_representation(literal);
      current->pool.push_back(literal_word);

      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (reg2 << 4) | ((pool_offset & 0x00000f00) >> 8);
      third->value = reg1 | 0xf0;
      fourth->value = 0b00111010;
      write_machine_code(first, second, third, fourth);
    }
    return 4;
  }else if(instr_name == "bgt"){
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    char reg1 = get_register_number(instruction->arguments->registers->name); //gpr1
    char reg2 = get_register_number(instruction->arguments->registers->next->name); //gpr2

    if(instruction->arguments->op->symbol != ""){
      relocation_record* rel_rec = new relocation_record();
      symbol_record* symbol = get_symbol_by_name(instruction->arguments->op->symbol);
      word* zero = new word();
      zero->value = 0;
      this->section_table.at(current_section_number - 1)->pool.push_back(zero);
      if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = "(" + symbol->name + ")";
        rel_rec->addend = 0;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }else{ //local symbol

        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = symbol->name;;
        rel_rec->addend = -1;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (reg2 << 4) | ((pool_offset & 0x00000f00) >> 8);
      third->value = reg1 | 0xf0;
      fourth->value = 0b00111011;
      write_machine_code(first, second, third, fourth);
    }else{ //literal
      int literal = instruction->arguments->op->value;
      section_record* current = this->section_table.at(current_section_number - 1);
      word* literal_word = new word();
      literal_word->value = little_endian_representation(literal);
      current->pool.push_back(literal_word);

      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (reg2 << 4) | ((pool_offset & 0x00000f00) >> 8);
      third->value = reg1 | 0xf0;
      fourth->value = 0b00111011;
      write_machine_code(first, second, third, fourth);
      }
    return 4;
  }else if(instr_name == "push"){ // 10000001 11100000 cccc1111 11111100, cccc := gpr_number
    //push %gpr <==> sp <= sp - 4; mem32[sp] <= gpr
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    char reg_num = get_register_number(instruction->arguments->registers->name);
    first->value = 0xfc;
    second->value = (reg_num << 4) | 0x0f;
    third->value = 0xe0;
    fourth->value = 0b10000001;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "pop"){ //10010011 aaaa1110 00000000 00000100, aaaa := gpr_number
    //pop %gpr <==> gpr <= mem32[sp(14)]; sp(14) <= sp(14) + 4;
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    char reg_num = get_register_number(instruction->arguments->registers->name);
    first->value = 0x04;
    second->value = 0;
    third->value = (reg_num << 4) | 0x0e;
    fourth->value = 0b10010011;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "xchg"){ // 01000000 0000bbbb cccc0000 00000000, bbbb := destination_reg_number, cccc := source_reg_number
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    source_reg <<= 4;
    char destination_reg = get_register_number(instruction->arguments->registers->next->name); //destination register
    second->value = 0x00 | source_reg;
    third->value = 0x00 | destination_reg;
    fourth->value = 0b01000000;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "add"){ // 01010000 aaaabbbb cccc0000 00000000, aaaa := dest_reg_num, bbbb := dest_reg_num, cccc := source_reg_num
    //add %gprS, %gprD
    //gprA <= gprB + gprC <==> gprD <= gprD + gprS
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    source_reg <<= 4;
    second->value = source_reg;
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01010000;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "sub"){ // 01010001 aaaabbbb cccc0000 00000000, aaaa == bbbb := dest_reg_num, cccc := source_reg_num
    //sub %gprS, gprD
    //gprD <= gprD - gprS <==> gprA <= gprB - gprC
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    source_reg <<= 4;
    second->value = source_reg;
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01010001;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "mul"){ // 01010010 ...
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    source_reg <<= 4;
    second->value = source_reg;
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01010010;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "div"){ // 01010011
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    source_reg <<= 4;
    second->value = source_reg;
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01010011;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "not"){ // 01100000 aaaabbbb 00000000 00000000, aaaa = bbbb := reg_num
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char reg_num = get_register_number(instruction->arguments->registers->name);
    second->value = 0;
    char offset = reg_num;
    reg_num = reg_num << 4;
    reg_num = reg_num | offset;
    third->value = reg_num;
    fourth->value = 0b01100000;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "and"){ // 01100001 aaaabbbb cccc0000 00000000, aaaa = bbbb := dest_reg_num, cccc := source_reg_num
    // and %gprS, %gprD <==> gprD <= gprD & gprS <==> gprA <= gprB & gprC
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    source_reg = source_reg << 4;
    second->value = source_reg;
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01100001;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "or"){ // 01100010
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    source_reg = source_reg << 4;
    second->value = source_reg;
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01100010;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "xor"){ //01100011
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    source_reg = source_reg << 4;
    second->value = source_reg;
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01100011;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "shl"){ // 01110000 aaaabbbb cccc0000 00000000, aaaa = bbbb := dest_reg_num, cccc := source_reg_num
    //shl %gprS, $gprD <==> gprD <= gprD << gprS <==> gprA <= gprB << gprC
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    source_reg = source_reg << 4;
    second->value = source_reg;
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01110000;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "shr"){ // 01110001
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    char source_reg = get_register_number(instruction->arguments->registers->name);
    char destination_reg = get_register_number(instruction->arguments->registers->next->name);
    source_reg = source_reg << 4;
    second->value = source_reg;
    char offset = destination_reg;
    destination_reg <<= 4;
    destination_reg = destination_reg | offset;
    third->value = destination_reg;
    fourth->value = 0b01110001;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "ld"){
    // gpr <= operand
    operand* op = instruction->arguments->op;
    string gpr_name = instruction->arguments->registers->name;
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    switch (resolve_operand_type(op))
    {
    case 1:
      {// ld $literal, %gpr <==> gpr <= operand
      //gprA <= mem32[gpr[B] + gpr[C] + D] <==> gprA <= mem32[PC + r0 + offset_to_pool_where_literal_is_stored] TREBA PC
      // 10010010 aaaa1111 0000dddd dddddddd
      int literal = op->value;
      word* lit_word = new word();
      lit_word->value = little_endian_representation(literal);
      this->section_table.at(current_section_number - 1)->pool.push_back(lit_word);
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (pool_offset & 0x00000f00) >> 8;
      third->value = (get_register_number(gpr_name) << 4) | 0x0f;
      fourth->value = 0b10010010;
      write_machine_code(first, second, third, fourth);
      return 4;
      break;}
    case 2:
      {// ld $simbol, %gpr <==> gpr <= simbol
      //gprA <= mem32[gpr[B] + gpr[C] + D] <==> gprA <= mem32[PC + r0 + offset_to_pool] TREBA PC
      // 10010010 aaaa1111 0000dddd dddddddd
      relocation_record* rel_rec = new relocation_record();
      symbol_record* symbol = get_symbol_by_name(instruction->arguments->op->symbol);
      if(!symbol){
        cout << "ERROR: LD INSTRUCTION -> SYMBOL OPERAND DOES'T EXIST" << endl;
        return 0;
      }
      word* zero = new word();
      zero->value = 0;
      this->section_table.at(current_section_number - 1)->pool.push_back(zero);
      if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = "(" + symbol->name + ")";
        rel_rec->addend = 0;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }else{ //local symbol

        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = symbol->name;;
        rel_rec->addend = -1;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (pool_offset & 0x00000f00) >> 8;
      third->value = (get_register_number(gpr_name) << 4) | 0x0f;
      fourth->value = 0b10010010;
      write_machine_code(first, second, third, fourth);
      return 4;
      break;}
    case 3:
      {
      // ld literal, %gpr <==> gpr <= mem32[literal]
      // gprA <= mem32[pc + r0 + pool_offset], gprA <= mem32[gprA + r0 + 0]
      // 10010010 aaaa1111 0000dddd dddddddd, aaaa := gpr_num | 10010010 aaaabbbb 00000000 00000000, aaaa = bbbb := gpr_num
      int literal = op->value;
      word* lit_word = new word();
      lit_word->value = little_endian_representation(literal);
      this->section_table.at(current_section_number - 1)->pool.push_back(lit_word);
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (pool_offset & 0x00000f00) >> 8;
      third->value = (get_register_number(gpr_name) << 4) | 0x0f;
      fourth->value = 0b10010010;
      write_machine_code(first, second, third, fourth);

      //gpr now holds literal value
      byte_type* first1 = new byte_type();
      byte_type* second1 = new byte_type();
      byte_type* third1 = new byte_type();
      byte_type* fourth1 = new byte_type();
      first1->value = 0;
      second1->value = 0;
      char gpr_num = get_register_number(gpr_name);
      third1->value = (gpr_num << 4) | gpr_num;
      fourth1->value = 0b10010010;
      write_machine_code(first1, second1, third1, fourth1);
      return 8;
      break;}
    case 4: //same thing as case 3, the difference is the making of relocation record
      {
      // ld symbol, %gpr <==> gpr <= mem32[symbol]
      // gprA <= mem32[pc + r0 + pool_offset], gprA <= mem32[gprA + r0 + 0]
      // 10010010 aaaa1111 0000dddd dddddddd, aaaa := gpr_num | 10010010 aaaabbbb 00000000 00000000, aaaa = bbbb := gpr_num
      relocation_record* rel_rec = new relocation_record();
      symbol_record* symbol = get_symbol_by_name(instruction->arguments->op->symbol);
      word* zero = new word();
      zero->value = 0;
      this->section_table.at(current_section_number - 1)->pool.push_back(zero);
      if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = "(" + symbol->name + ")";
        rel_rec->addend = 0;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }else{ //local symbol

        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = symbol->name;;
        rel_rec->addend = -1;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (pool_offset & 0x00000f00) >> 8;
      third->value = (get_register_number(gpr_name) << 4) | 0x0f;
      fourth->value = 0b10010010;
      write_machine_code(first, second, third, fourth);

      //gpr now holds literal value
      byte_type* first1 = new byte_type();
      byte_type* second1 = new byte_type();
      byte_type* third1 = new byte_type();
      byte_type* fourth1 = new byte_type();
      first1->value = 0;
      second1->value = 0;
      char gpr_num = get_register_number(gpr_name);
      third1->value = (gpr_num << 4) | gpr_num;
      fourth1->value = 0b10010010;
      write_machine_code(first1, second1, third1, fourth1);
      return 8;
      break;}
    case 5:
      {//ld %reg, %gpr <==> gpr <= reg
      // 10010001 aaaabbbb 00000000 00000000, aaaa := gpr_num, bbbb := reg_num
      int gpr_num = get_register_number(gpr_name);
      int reg_num = get_register_number(op->reg->name);
      first->value = 0;
      second->value = 0;
      third->value = (gpr_num << 4) | reg_num;
      fourth->value = 0b10010001;
      write_machine_code(first, second, third, fourth);
      return 4;
      break;}
    case 6:
      {
      //simmilar to the previous one, the only difference is instruction modificator
      // gpr <= mem32[reg]
      // 10010010 aaaabbbb 00000000 00000000, aaaa := gpr_num, bbbb := reg_num
      int gpr_num = get_register_number(gpr_name);
      int reg_num = get_register_number(op->reg->name);
      first->value = 0;
      second->value = 0;
      third->value = (gpr_num << 4) | reg_num;
      fourth->value = 0b10010010;
      write_machine_code(first, second, third, fourth);
      return 4;
      break;}
    case 7:
      {
      //value present in memory at [%reg + <literal>] location
      //ld ..., %gpr <==> gpr <= ...
      //if literal cannot be displayed as 12b value report error
      // 10010010 aaaabbbb 0000dddd dddddddd, aaaa := gpr_num, bbbb := reg_num, dddd dddddddd := literal_value
      int literal = op->value;
      int gpr_num = get_register_number(gpr_name);
      int reg_num = get_register_number(op->reg->name);
      if(literal > 2047 || literal < -2048){
        cout << "\nERROR: LD INSTRUCTION ERROR -> VALUE OF LITERAL BIGGER THAN 2^12\n";
        return 0;
      }else{
        first->value = literal & 0x000000ff;
        second->value = (literal & 0x00000f00) >> 8;
        third->value = (gpr_num << 4) | reg_num;
        fourth->value = 0b10010010;
        write_machine_code(first, second, third, fourth);
      }
      return 4;
      break;}
    case 8:
      {cout << "\nERROR: LD INSTRUCTION ERROR -> VALUE OF SYMBOL UNKNWON\n";
      return 0;
      break;}
    default:
      {cout << "\nERROR\n";
      return 0;
      break;}
    }
    return 4;
  }else if(instr_name == "st"){
    // operand <= gpr
    operand* op = instruction->arguments->op;
    string gpr_name = instruction->arguments->registers->name;
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();

    switch (resolve_operand_type(op))
    {
    case 1:
      {
      cout << "\nERROR: ST INSTRUCTION ERROR -> INVALID OPERATOR\n";
      return 0;
      break;}
    case 2:
      {
      cout << "\nERROR: ST INSTRUCTION ERROR -> INVALID OPERATOR\n";
      return 0;
      break;}
    case 3:
      {
      //st %gpr, literal <==> mem32[literal] <= gpr
      //mem32[mem32[pc + r0 + offset]] <= gpr
      // 10000010 11110000 ccccdddd dddddddd, cccc := gpr_num, dddd dddddddd := offsets
      int literal = op->value;
      word* lit_word = new word();
      lit_word->value = little_endian_representation(literal);
      char reg_num = get_register_number(gpr_name);
      this->section_table.at(current_section_number - 1)->pool.push_back(lit_word);
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (reg_num << 4) | ((pool_offset & 0x00000f00) >> 8);
      third->value = 0xf0;
      fourth->value = 0b10000010;
      write_machine_code(first, second, third, fourth);
      return 4;
      break;}
    case 4:
      {
      //st %gpr, symbol <==> mem32[symbol] <= gpr
      //mem32[mem32[pc + r0 + offset]] <= gpr
      // 10000010 11110000 ccccdddd dddddddd, cccc := gpr_num, dddd dddddddd := offsets
      relocation_record* rel_rec = new relocation_record();
      symbol_record* symbol = get_symbol_by_name(instruction->arguments->op->symbol);
      word* zero = new word();
      zero->value = 0;
      this->section_table.at(current_section_number - 1)->pool.push_back(zero);
      if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = "(" + symbol->name + ")";
        rel_rec->addend = 0;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }else{ //local symbol

        rel_rec->offset = get_offset_to_pool();
        rel_rec->type = "PC_REL32";
        rel_rec->symbol = symbol->name;;
        rel_rec->addend = -1;
        this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

      }
      int pool_offset = get_offset_to_pool() - 4 - locationCounter;
      first->value = pool_offset & 0x000000ff;
      second->value = (get_register_number(gpr_name) << 4) | ((pool_offset & 0x00000f00) >> 8);
      third->value = 0xf0;
      fourth->value = 0b10000010;
      write_machine_code(first, second, third, fourth);
      return 4;
      break;}
    case 5:
      {
      cout << "\nERROR: ST INSTRUCTION ERROR -> INVALID OPERATOR\n";
      return 0;
      break;}
    case 6:
      {
      //st %gpr, [%reg] <==> mem32[reg] <= gpr
      // 10000000 aaaa0000 cccc0000 00000000, aaaa := reg, cccc := gpr 
      char gpr_num = get_register_number(gpr_name);
      char reg_num = get_register_number(op->reg->name);
      first->value = 0;
      second->value = gpr_num << 4;
      third->value = reg_num << 4;
      fourth->value = 0b10000000;
      write_machine_code(first, second, third, fourth);
      return 4;
      break;}
    case 7:
      {
      // st %gpr, [%reg + literal] <==> [%reg + literal] <= gpr <==> mem32[reg + literal] <= gpr
      //10000000 aaaa0000 ccccdddd dddddddd, aaaa := reg, cccc := gpr, dddd dddddddd := literal
      int literal = op->value;
      if(literal > 2047 || literal < -2048){
        cout << "\nERROR: ST INSTRUCTION ERROR -> VALUE OF LITERAL BIGGER THAN 2^12\n";
        return 0;
      }else{
        char gpr_num = get_register_number(gpr_name);
        char reg_num = get_register_number(op->reg->name);
        first->value = literal & 0x000000ff;
        second->value = (gpr_num << 4) | ((literal & 0x00000f00) >> 8);
        third->value = reg_num << 4;
        fourth->value = 0b10000000;
        write_machine_code(first, second, third, fourth);
        return 4;
      }
      break;}
    case 8:
      {
      cout << "\nERROR: ST INSTRUCTION ERROR -> VALUE OF SYMBOL UNKNOWN\n";
      return 0;
      break;}
    default:
      {
      cout << "\nERROR\n";
      return 0;
      break;}
    }
    return 4;
  }else if(instr_name == "csrrd"){
    //csrrd %csr, %gpr <==> gpr <= csr
    //gprA <= csrB
    // 10010000 aaaabbbb 00000000 00000000, aaaa := gpr_num, bbbb := csr_num
    char csr_num = get_register_number(instruction->arguments->registers->name);
    char gpr_num = get_register_number(instruction->arguments->registers->next->name);
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    second->value = 0;
    third->value = (gpr_num << 4) | csr_num;
    fourth->value = 0b10010000;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else if(instr_name == "csrwr"){
    //csr <= gpr <==> csrA <= gprB
    // 10010100 aaaabbbb 00000000 00000000, aaaa := csr, bbbb := gpr
    char gpr_num = get_register_number(instruction->arguments->registers->name);
    char csr_num = get_register_number(instruction->arguments->registers->next->name);
    byte_type* first = new byte_type();
    byte_type* second = new byte_type();
    byte_type* third = new byte_type();
    byte_type* fourth = new byte_type();
    first->value = 0;
    second->value = 0;
    third->value = (csr_num << 4) | gpr_num;
    fourth->value = 0b10010100;
    write_machine_code(first, second, third, fourth);
    return 4;
  }else{
    cout << "\n****ERROR NO INSTRUCTION WITH GIVEN NAME****\n";
    return 0;
  }
  cout << "\n****ERROR OCCURED WHILE PROCESSING INSTRUCTION****\n";
  return 0;
}

int Assembler :: get_section_number(string section_name){
  for(section_record* section : this->section_table){
    if(section->name == section_name){
      return section->number;
    }
  }
  return current_section_number + 1; //if section is yet to be created
}

bool Assembler :: section_in_table(string section_name){
  for(section_record* section : this->section_table){
    if(section->name == section_name)
      return true;
  }
  return false;
}

bool Assembler :: symbol_defined(string name){
  for(symbol_record* symbol : this->symbol_table){
    if(symbol->name == name)
      return true;
  }
  return false;
}

void Assembler::first_pass(){
  if(is_first_pass_done)
    return;
  int locationCounter = 0;
  global_struct* line = this->input;
  while(line){
    if(line->dir){
      if(line->dir->directive_name == ".section"){ //.section directive resets locationCounter and adds new section in both symbol and section tables
        if(current_section_name != ""){ //update previous sections size if previous one exists
            section_record* current = this->section_table.at(current_section_number - 1);
            current->size += locationCounter;        
        }
        current_section_name = line->dir->arguments->symbol;
        current_section_number = get_section_number(current_section_name);
        if(!section_in_table(current_section_name)){
          section_record* section = make_section_record(current_section_number, locationCounter, current_section_name);
          add_section(section);
          symbol_record* symbol = make_symbol(this->symbol_table.size(), UND, "SCTN", "LOC", current_section_number, current_section_name);
          add_symbol(symbol);
          }
        locationCounter = 0;
      }else if(line->dir->directive_name == ".skip"){ //update locationCounter with given number of Bytes
        locationCounter += line->dir->arguments->literal;
      }else if(line->dir->directive_name == ".word"){ //update locationCounter with 4 Bytes for each argument of directive
        int number_of_arguments = 0;
        arg* temp = line->dir->arguments;
        while(temp){
          number_of_arguments++;
          temp = temp->next;
        }
        locationCounter += 4*number_of_arguments;
      }else if(line->dir->directive_name == ".extern"){ //import symbols as GLOB type from the given list in directive
            arg* temp = line->dir->arguments;
            while(temp){
              if(symbol_defined(temp->symbol)){
              cout << "\n\nERROR: MULTIPLE DEFINITION OF EXTERN SYMBOL " << temp->symbol << endl << endl;
              break;
            }
            symbol_record* symbol = make_symbol(this->symbol_table.size(), UND, "NOTYP", "GLOB", UND, temp->symbol);
            add_symbol(symbol);
            temp = temp->next;
          }
      }else if(line->dir->directive_name == ".end"){ //directive used to end the process of assembling
        break;
      }else if(line->dir->directive_name == ".ascii"){
        string string_literal = line->dir->arguments->symbol;
        int sz = string_literal.size();
        int addend = sz/4;
        if(sz%4 != 0)
          addend += 1;
        addend *= 4;
        locationCounter += addend;
      }
    }else if(line->lb){ //labels are used to define new symbols
      if(!symbol_defined(line->lb->name)){
        symbol_record* symbol = make_symbol(this->symbol_table.size(), UND, "NOTYP", "LOC", current_section_number, line->lb->name);
        add_symbol(symbol);
      }else{
        cout << "\n\nERROR: REDEFINTION OF SYMBOL " << line->lb->name << endl<<endl;
        break;
      }
    }else if(line->instruction){ //most of the instructions are 4B long, but some types are 2x bigger
      instr* instruction = line->instruction;
      if(instruction->name == "iret")
        {
          locationCounter += 8;
        }
      else if(instruction->name == "call")
        {
          locationCounter += 4;
        }
      else if(instruction->name == "ld")
        {
        if(resolve_operand_type(instruction->arguments->op) == 3 || resolve_operand_type(instruction->arguments->op) == 4)
          {
            locationCounter += 8;
          }
        else
          {
            locationCounter += 4;
          }
        }
      else
        {
          locationCounter += 4;
        }
    }
    line = line->next;
  }
  // update last sections size; set first_pass_done flag to true
  section_record* current = this->section_table.at(current_section_number - 1);
  current->size += locationCounter;
  is_first_pass_done = true;
}

void Assembler :: print_results(){
  cout << "#.sctntab\n";
  cout << "Num\t Addr\t Size\t Name\n";
  for(section_record* record : this->section_table){
    cout << record->number << "\t " << record->address << "\t " << record->size << "\t " << record->name << endl;
  }
  cout << "\n#.symtab\n";
  cout << "Num\t Value\t Size\t Type\t Bind\t Ndx\t Name\n";
  for(symbol_record* record : this->symbol_table){
    cout << record->number << "\t " << record->value << "\t " << 0 << "\t " << record->type << "\t " << record->bind << "\t " << (record->ndx == 0 ? "UND" : to_string(record->ndx)) << "\t " << record->name << endl;
  }
}

void Assembler :: resolve_reloc_records(){
  for(section_record* section : this->section_table){
    for(reloc_record* rel_rec : section->reloc_table){
      if(rel_rec->addend == -1){ //change -1 value to symbol value; only for local symbols
        rel_rec->addend = get_symbol_by_name(rel_rec->symbol)->value;
        rel_rec->symbol = "(" + section->name + ")";
      }
    }
  }
}

void Assembler :: second_pass(){
  if(is_first_pass_done == false)
    first_pass();
  if(is_second_pass_done)
    return;
  int locationCounter = 0;
  global_struct* line = this->input;
  while(line){
    if(line->dir){
      if(line->dir->directive_name == ".global"){ //set bind to GLOB for provided symbols
        arg* temp = line->dir->arguments;
        while(temp){
          symbol_record* symbol = this->get_symbol_by_name(temp->symbol);
          if(!symbol){
            cout << "\n\nERROR: GLOBAL SYMBOL " << temp->symbol<<" WASN'T DEFINED\n" << endl;
            break;
          }else{
            symbol->bind = "GLOB";
            }
          temp = temp->next;
        }
      }else if(line->dir->directive_name == ".extern"){
        //resolved in first pass
      }else if(line->dir->directive_name == ".section"){
        current_section_name = line->dir->arguments->symbol;
        current_section_number = get_section_number(current_section_name);
        locationCounter = 0;
      }else if(line->dir->directive_name == ".word"){
        int number_of_arguments = 0;
        arg* temp = line->dir->arguments;
        while(temp){
          number_of_arguments++;
          temp = temp->next;
        }
        locationCounter += 4*number_of_arguments;
        //eject values of arguments in machine code
        temp = line->dir->arguments;
        while(temp){
          if(temp->symbol != ""){ //for symbol eject 4B of zeros in the data part, not the pool
            
            relocation_record* rel_rec = new relocation_record();
            symbol_record* symbol = get_symbol_by_name(temp->symbol);
            byte_type* zero = new byte_type();
            zero->value = 0;
            write_machine_code(zero, zero, zero, zero);
            if(symbol->ndx == 0 || symbol->bind == "GLOB"){ //extern or global symbol
        
              rel_rec->offset = (this->section_table.at(current_section_number - 1)->data.size()) - 4; // DA LI TREBA SIZE - 1 ???????????????????
              rel_rec->type = "PC_REL32";
              rel_rec->symbol = "(" + symbol->name + ")";
              rel_rec->addend = 0;
              this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

            }else{ //local symbol

              rel_rec->offset = (this->section_table.at(current_section_number - 1)->data.size()) - 4;
              rel_rec->type = "PC_REL32";
              rel_rec->symbol = symbol->name;
              rel_rec->addend = -1;
              this->section_table.at(current_section_number - 1)->reloc_table.push_back(rel_rec);

              }
          }else{ //literal value to be inserted; literal is size of integer -> 32 bits <==> 4 Bytes
            int literal = temp->literal;
            for(int i = 0; i < 4; i++){ //divide literal in 4 separate Bytes, and eject them
              int x = (literal >> (8*i)) & 0xff;
              byte_type* lit_byte = new byte_type();
              lit_byte->value = x;
              this->section_table.at(current_section_number - 1)->data.push_back(lit_byte);
            }
          }
          temp = temp->next;
        }
      }else if(line->dir->directive_name == ".skip"){ //this affects alignment of the whole structure; fill in machine code with the given number of zero bytes
        locationCounter += line->dir->arguments->literal;
        int literal = line->dir->arguments->literal;
        for(int i = 0; i < literal; i++){
          byte_type* zero = new byte_type();
          zero->value = 0;
          this->section_table.at(current_section_number - 1)->data.push_back(zero);
        }
      }else if(line->dir->directive_name == ".end"){
        break;
      }else if(line->dir->directive_name == ".ascii"){
        string string_literal = line->dir->arguments->symbol;
        string_literal = string_literal.substr(1,string_literal.size() - 2);
        int sz = string_literal.size();
        int addend = sz/4;
        if(sz%4 != 0)
          addend += 1;
        addend *= 4;
        for(int i = 0; i < (addend - sz);i++){
          char nop = 0;
          string_literal += nop;
        }
        const char* cstring_literal = string_literal.c_str();
        //cout << cstring_literal << endl;
        int i = 0;
        while(i <= string_literal.size()- 1){
          byte_type* first = new byte_type();
          byte_type* second = new byte_type();
          byte_type* third = new byte_type();
          byte_type* fourth = new byte_type();
          
          first->value = cstring_literal[i];
          second->value = cstring_literal[i + 1];
          third->value = cstring_literal[i + 2];
          fourth->value = cstring_literal[i + 3];

          this->section_table.at(current_section_number - 1)->data.push_back(first);
          this->section_table.at(current_section_number - 1)->data.push_back(second);
          this->section_table.at(current_section_number - 1)->data.push_back(third);
          this->section_table.at(current_section_number - 1)->data.push_back(fourth);
          i += 4;
        }
        locationCounter += addend;
      }
    }else if(line->lb){
      symbol_record* symbol = get_symbol_by_name(line->lb->name);
      if(!symbol){
        cout << "\nERROR: THIS IS NOT POSSIBLE :)\n";
        break;
      }else{ //asign value to the symbol
        //section_record* current = this->section_table.at(current_section_number - 1);
        //symbol->value = locationCounter + (current->address); //TREBA LI OVAKO, ILI NE?
        symbol->value = locationCounter;
      }
    }else if(line->instruction){
      locationCounter += process_instruction_and_update_counter(line->instruction, locationCounter);
    }
    line = line->next;
  }
  this->align_sections();
  this->resolve_reloc_records();
  is_second_pass_done = true;
}

void Assembler :: make_object_file(){
  if(is_second_pass_done == false)
    this->second_pass();
  fprintf(output_file, "%s %ld\n", "#.symtab", this->symbol_table.size());
  fprintf(output_file, "%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n", "Num", "Value", "Size", "Type", "Bind", "Ndx", "Name");
  for(symbol_record* symbol : this->symbol_table){
    fprintf(output_file, "%-15d %-15d %-15d %-15s %-15s %-15s %-15s\n", symbol->number, symbol->value, 0, (symbol->type).c_str(), (symbol->bind).c_str(), (symbol->ndx == 0 ? "UND" : (to_string(symbol->ndx)).c_str()), (symbol->name).c_str());
  }
  fprintf(output_file, "%s %ld\n", "#.sctntab", this->section_table.size());
  fprintf(output_file, "%-15s %-15s %-15s %-15s\n", "Num", "Address", "Size", "Name");
  for(section_record* section : this->section_table){
    fprintf(output_file, "%-15d %-15d %-15d %-15s\n", section->number, section->address, section->size, (section->name).c_str());
  }
  for(section_record* section : this->section_table){
    fprintf(output_file, "%s %ld\n", ("#.rela " + section->name).c_str(), section->reloc_table.size());
    fprintf(output_file, "%-15s %-15s %-15s %-15s\n", "Offset", "Type", "Symbol", "Addend");
    for(reloc_record* rel_rec : section->reloc_table){
      fprintf(output_file, "%-15ld %-15s %-15s %-15d\n", rel_rec->offset, (rel_rec->type).c_str(), (rel_rec->symbol).c_str(), rel_rec->addend);
    }
    fprintf(output_file, "%s %ld\n", ("#." + section->name).c_str(), (section->size + section->pool.size()*4));
    int i = 0;
    for(byte_type* B : section->data){
      if(i == 3){
        fprintf(output_file, "%02x ", (unsigned int)(B->value & 0xff));
        i++;
      }else if(i == 7){
        i = 0;
        fprintf(output_file, "%02x\n", (unsigned int)(B->value & 0xff));
      }else{
        fprintf(output_file, "%02x", (unsigned int)(B->value & 0xff));
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
          fprintf(output_file, "%08x\n", W->value);
        }else{
          fprintf(output_file, "%08x ", W->value);
        }
        i++;
      }
      if(i % 2 == 0)
        fprintf(output_file, "\n");
    }else{
      if(section->size % 8 != 0){
        fprintf(output_file, "\n");
      }
    }
  }
  fclose(output_file);
}

int main(int argc, char* argv[])
{
	if(strcmp(argv[1], "-o") != 0){
		cout << "ERROR: NO INPUT FILE PROVIDED\n";
		exit(-1);
	}
	char* output_file = argv[2];
	char* input_file = argv[3];
	FILE* output = fopen(output_file, "w");
	FILE* input = fopen(input_file, "r");
	if(!input){
		cout << "\nERROR: NO SUCH INPUT FILE\n";
		exit(-1);
	}
	
	yyin = input;

	if (yyparse())
		return 1;
	// cout << "Printing everything:" << endl << endl;
	// print_all(head);

	//init dataStructures
	Assembler* assembler = new Assembler(head, output);
	assembler->first_pass();
	//assembler->print_results();
	assembler->second_pass();
	assembler->make_object_file();
	cout << "****************************************************************\nExiting without errors!\n****************************************************************\n";
	exit(0);
}
