#include "../inc/emulator.hpp"

void Emulator :: read_input(){
    int line_size = 40;
    char* line = new char[line_size];
    while(fgets(line, line_size, this->input)){
        string l = line;
        uint address;
        char* address_like = new char[15];
        vector<byte_type*> bytes;
        for(int i = 0; i < 8;i++){
            byte_type* b = new byte_type();
            bytes.push_back(b);
        }
        int value0, value1, value2, value3, value4, value5, value6, value7;
        
        address = strtol(l.substr(0, 8).c_str(), NULL, 16);
        //cout << std::hex <<address << endl;

        value0 = strtol(l.substr(10, 2).c_str(), NULL, 16);
        value1 = strtol(l.substr(13, 2).c_str(), NULL, 16);
        value2 = strtol(l.substr(16, 2).c_str(), NULL, 16);
        value3 = strtol(l.substr(19, 2).c_str(), NULL, 16);
        value4 = strtol(l.substr(22, 2).c_str(), NULL, 16);
        value5 = strtol(l.substr(25, 2).c_str(), NULL, 16);
        value6 = strtol(l.substr(28, 2).c_str(), NULL, 16);
        value7 = strtol(l.substr(31, 2).c_str(), NULL, 16);
        //cout << l.substr(10, 2) << " " << l.substr(13, 2) << " " << l.substr(16, 2)<< " " << l.substr(19, 2)<< " " << l.substr(22, 2)<< " " << l.substr(25, 2)<< " " << l.substr(28, 2) << " " << l.substr(31, 2) << endl;
        //cout << std::hex << (uint)(value0 & 0xff) <<" "<<(uint)(value1 & 0xff)<<" "<<(uint)(value2 & 0xff)<<" "<<(uint)(value3 & 0xff)<<" "<<(uint)(value4 & 0xff)<<" "<<(uint)(value5 & 0xff)<<" "<<(uint)(value6 & 0xff)<<" "<<(uint)(value7 & 0xff) <<endl;

        bytes.at(0)->value = value0;
        bytes.at(1)->value = value1;
        bytes.at(2)->value = value2;
        bytes.at(3)->value = value3;
        bytes.at(4)->value = value4;
        bytes.at(5)->value = value5;
        bytes.at(6)->value = value6;
        bytes.at(7)->value = value7;
        addr_bytes* obj = new addr_bytes();
        obj->address = address;
        obj->bytes = bytes;
        this->input_structure.push_back(obj);
    }
}

void Emulator :: fill_memory(){
    for(auto obj : this->input_structure){
        for(int i = 0; i < obj->bytes.size();i++){
            memory_location* mem_loc = new memory_location();
            mem_loc->address = obj->address + i;
            mem_loc->value = obj->bytes.at(i)->value;
            this->memory.push_back(mem_loc);
        }
    }
    for(int i = 0; i < 8; i++){
        memory_location* mem_loc = new memory_location();;
        mem_loc->address = (uint)TERM_OUT + i;
        //cout << std::hex << mem_loc->address << endl;
        mem_loc->value = 0;
        this->memory.push_back(mem_loc);
    }
}

void Emulator :: print_registers_image(){
   cout << "Emulated processor state:" << endl;
   for(int i = 0; i < 16; i++){
    cout << "r" << std::dec << i << "=0x" << std::hex << std::setfill('0') << std::setw(8) << this->gprs.at(i)->get_value() << (i % 4 == 3 ? "\n" : "\t");
    cout << flush;
   }
   cout << "status=0x" << std::hex << std::setfill('0') << std::setw(8) << this->csrs.at(STATUS)->get_value() << " ";
   cout << "handler=0x" << std::hex << std::setfill('0') << std::setw(8) << this->csrs.at(HANDLER)->get_value() << " ";
   cout << "cause=0x" << std::hex << std::setfill('0') << std::setw(8) << this->csrs.at(CAUSE)->get_value() << endl;
}

bool Emulator :: resolve_machine_code(char oc, char mod, char regA, char regB, char regC, short displacement){
    switch (oc)
    {
    case 0: 
        {
        //halt
        if(mod == regA == regB == regC == displacement == 0){
            cout << endl << "-----------------------------------------------------------------\nEmulated processor executed halt instruction" << endl;
            this->print_registers_image();
            return true;
        }
        break;}
    case 1: 
        {
        //cout << "Software interrupt" << endl;
        //int: push status; push pc; cause <= 4; status <= status & (~0x1); pc <= handler;
        //push val: sp <= sp - 4; mem32[sp] = val
        if(mod == regA == regB == regC == displacement == 0){
            //push status
            this->gprs.at(SP)->decrease_by_four();
            this->set_word_value(this->gprs.at(SP)->get_value(), this->csrs.at(STATUS)->get_value());
            //push pc
            this->gprs.at(SP)->decrease_by_four();
            this->set_word_value(this->gprs.at(SP)->get_value(), this->gprs.at(PC)->get_value());
            //cause <= 4
            this->csrs.at(CAUSE)->set_value(4);
            //status <= status & (~0x1);
            this->csrs.at(STATUS)->set_value(this->csrs.at(STATUS)->get_value() & ~0x1);
            //pc <= handler
            this->gprs.at(PC)->set_value(this->csrs.at(HANDLER)->get_value());
        }
        break;}
    case 2:
        { 
        //call
        if(mod == 0){ //push pc; pc <= gprA + gprB + displ
            //push pc;
          //  cout << "Call: push pc; pc = gpra + gprb + d" << endl;
            this->gprs.at(SP)->decrease_by_four();
            this->set_word_value(this->gprs.at(SP)->get_value(), this->gprs.at(PC)->get_value());
            //pc <= gprA + gprB + displ
            this->gprs.at(PC)->set_value(this->gprs.at(regA)->get_value() + this->gprs.at(regB)->get_value() + displacement);
        }else if(mod == 1){ //push pc; pc <= mem32[gprA + gprB + displ]
            //push pc;
          //  cout << "Call: push pc; pc = mem32[gpra + gprb + d]" << endl;
            this->gprs.at(SP)->decrease_by_four();
            this->set_word_value(this->gprs.at(SP)->get_value(), this->gprs.at(PC)->get_value());
            //pc <= mem32[gprA + gprB + displ]
            this->gprs.at(PC)->set_value(this->get_word_value((uint)(this->gprs.at(regA)->get_value() + this->gprs.at(regB)->get_value() + displacement)));
            //cout << "CALL: PC <= " << std::hex << this->gprs.at(PC)->get_value() << endl;
        }else{
            //stress
            cout << "ERROR: CALL INSTRUCTION -> UNDEFINED MOD PROVIDED" << endl;
            return true;
        }
        break;}
    case 3:
            {
            //jump instructions
            switch (mod)
            {
            case 0:
                {
                //pc <= gprA + d
              //  cout << "Jump: pc = gpra + d" << endl;
                this->gprs.at(PC)->set_value(this->gprs.at(regA)->get_value() + displacement);
                break;}
            case 1:
                {
                //if (gpr[B] == gpr[C]) pc<=gpr[A]+D;
               // cout << "Jump: pc = gpra + d ako su b i c jednaki" << endl;
                if(this->gprs.at(regB)->get_value() == this->gprs.at(regC)->get_value()){
                    this->gprs.at(PC)->set_value(this->gprs.at(regA)->get_value() + displacement);
                }
                break;}
            case 2:
                {
                //if (gpr[B] != gpr[C]) pc<=gpr[A]+D;
             //   cout << "Jump: pc = gpra + d ako su b i c razliciti" << endl;
                if(this->gprs.at(regB)->get_value() != this->gprs.at(regC)->get_value()){
                    this->gprs.at(PC)->set_value(this->gprs.at(regA)->get_value() + displacement);
                }
                break;}
            case 3:
                {
                //if (gpr[B] signed> gpr[C]) pc<=gpr[A]+D;
              //  cout << "Jump: pc = gpra + d ako je b vece od c signed" << endl;
                if((signed)this->gprs.at(regB)->get_value() > (signed)this->gprs.at(regC)->get_value()){
                    this->gprs.at(PC)->set_value(this->gprs.at(regA)->get_value() + displacement);
                }
                break;}
            case 8:
                {
                //pc<=mem32[gpr[A]+D];
               // cout << "Jump: pc = mem32[gpra + d]" << endl;
                this->gprs.at(PC)->set_value(this->get_word_value(this->gprs.at(regA)->get_value() + displacement));
                break;}
            case 9:
                {
                //if (gpr[B] == gpr[C]) pc <= mem32[gpr[A]+D];
              //  cout << "Jump: pc = mem32[gpra + d] ako su b i c jednaki" << endl;
                if(this->gprs.at(regB)->get_value() == this->gprs.at(regC)->get_value()){
                    this->gprs.at(PC)->set_value(this->get_word_value(this->gprs.at(regA)->get_value() + displacement));
                }
                break;}
            case 10:
                {
                //if (gpr[B] != gpr[C]) pc <= mem32[gpr[A]+D];
              //  cout << "Jump: pc = mem32[gpra + d] ako su b i c razliciti" << endl;
                if(this->gprs.at(regB)->get_value() != this->gprs.at(regC)->get_value()){
                    this->gprs.at(PC)->set_value(this->get_word_value(this->gprs.at(regA)->get_value() + displacement));
                }
                break;}
            case 11:
                {
                //if (gpr[B] signed> gpr[C]) pc <= mem32[gpr[A]+D];
             //   cout << "Jump: pc = mem32[gpra + d] ako je b vece od c signed" << endl;
                if((signed)this->gprs.at(regB)->get_value() > (signed)this->gprs.at(regC)->get_value()){
                    this->gprs.at(PC)->set_value(this->get_word_value(this->gprs.at(regA)->get_value() + displacement));
                }
                break;}
            default:
                {
                cout << "ERROR: JUMP INSTRUCTIONS -> UNDEFINED MOD PROVIDED" << endl;
                return true;
                break;}
            }
            break;}
    case 4:
            {
            //xchg
          //  cout << "Xchg" << endl;
            uint temp = this->gprs.at(regB)->get_value();
            this->gprs.at(regB)->set_value(this->gprs.at(regC)->get_value());
            this->gprs.at(regC)->set_value(temp);
            break;}
    case 5:
            {
            //arithmetic operations
            switch (mod)
            {
            case 0:
                {
              //  cout <<"Arithmetics +" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() + this->gprs.at(regC)->get_value());
                break;}
            case 1:
                {
               // cout <<"Arithmetics -" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() - this->gprs.at(regC)->get_value());
                break;}
            case 2:
                {
                //cout <<"Arithmetics *" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() * this->gprs.at(regC)->get_value());
                break;}
            case 3:
                {
                //cout <<"Arithmetics /" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() / this->gprs.at(regC)->get_value());
                break;}
            default:
                {
                cout << "ERROR: ARITHMETIC INSTRUCTIONS -> UNDEFINED MOD PROVIDED" << endl;
                return true;
                break;}
            }
            break;}
    case 6:
            {
            //logical operations
            switch (mod)
            {
            case 0:
                {
                //cout << "Logical not" << endl;
                this->gprs.at(regA)->set_value(~this->gprs.at(regB)->get_value());
                break;}
            case 1:
                {
                //cout << "Logical and" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() & this->gprs.at(regC)->get_value());
                break;}
            case 2:
                {
                //cout << "Logical or" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() | this->gprs.at(regC)->get_value());
                break;}
            case 3:
                {
                //cout << "Logical xor" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() ^ this->gprs.at(regC)->get_value());
                break;}
            default:
                {
                cout << "ERROR: LOGICAL INSTRUCTIONS -> UNDEFINED MOD PROVIDED" << endl;
                return true;
                break;}
            }
            break;}
    case 7:
            {
            //shift operations
            if(mod == 0){
                //gpr[A]<=gpr[B] << gpr[C];
                //cout << "Shift left" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() << this->gprs.at(regC)->get_value());
            }else if(mod == 1){
                //gpr[A]<=gpr[B] >> gpr[C]
                //cout << "Shift right" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() >> this->gprs.at(regC)->get_value());
            }else{
                cout << "ERROR: SHIFT INSTRUCTIONS -> UNDEFINED MOD PROVIDED" << endl;
                return true;
            }
            break;}
    case 8:
            {
            //store instructions
            if(mod == 0){
                //mem32[gpr[A]+gpr[B]+D]<=gpr[C]
                //cout << "Store mem32[gpr[A]+gpr[B]+D]<=gpr[C]" << endl;
                this->set_word_value(this->gprs.at(regA)->get_value() + this->gprs.at(regB)->get_value() + displacement, this->gprs.at(regC)->get_value());
                if((uint)(this->gprs.at(regA)->get_value() + this->gprs.at(regB)->get_value() + displacement) == TERM_OUT){
                    uint value = this->get_word_value(TERM_OUT);
                    cout << (char)value << std::flush;
                }
            }else if(mod == 2){
                //mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
                //cout << "Store mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C]" << endl;
                this->set_word_value(this->get_word_value(this->gprs.at(regA)->get_value() + this->gprs.at(regB)->get_value() + displacement), this->gprs.at(regC)->get_value());
                if((uint)(this->get_word_value(this->gprs.at(regA)->get_value() + this->gprs.at(regB)->get_value() + displacement)) == TERM_OUT){
                    uint value = this->get_word_value(TERM_OUT);
                    cout << (char)value << std::flush;
                }
            }else if(mod == 1){
                //gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
                //cout << "Store gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C]" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regA)->get_value() + displacement);
                this->set_word_value(this->gprs.at(regA)->get_value(), this->gprs.at(regC)->get_value());
                if(this->gprs.at(regA)->get_value() == TERM_OUT){
                    uint value = this->get_word_value(TERM_OUT);
                    cout << (char)value << std::flush;
                }
            }else{
                cout << "ERROR: STORE INSTRUCTIONS -> UNDEFINED MOD PROVIDED" << endl;
                return true;
            }
            break;}
    case 9:
            {
            //load instructions
            switch (mod)
            {
            case 0:
                {
                //cout << "Load gprA <= csrB" << endl;
                this->gprs.at(regA)->set_value(this->csrs.at(regB)->get_value());    
                break;}
            case 1:
                {
                //cout << "Load gpr[A]<=gpr[B]+D" << endl;
                this->gprs.at(regA)->set_value(this->gprs.at(regB)->get_value() + displacement);
                break;}
            case 2:
                {
                //cout << "Load  gpr[A]<=mem32[gpr[B]+gpr[C]+D]" << endl;
                this->gprs.at(regA)->set_value(this->get_word_value(this->gprs.at(regB)->get_value() + this->gprs.at(regC)->get_value() + displacement));
                break;}
            case 3:
                {
                //cout << "Load gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D" << endl;
                this->gprs.at(regA)->set_value(this->get_word_value(this->gprs.at(regB)->get_value()));
                this->gprs.at(regB)->set_value(this->gprs.at(regB)->get_value() + displacement);
                break;}
            case 4:
                {
                //cout << "Load  csr[A]<=gpr[B]" << endl;
                this->csrs.at(regA)->set_value(this->gprs.at(regB)->get_value());    
                break;}
            case 5:
                {
                //cout << "Load csr[A]<=csr[B]|D" << endl;
                this->csrs.at(regA)->set_value(this->csrs.at(regB)->get_value() | displacement);
                break;}
            case 6:
                {
                //cout << "Load csr[A]<=mem32[gpr[B]+gpr[C]+D]" << endl;
                this->csrs.at(regA)->set_value(this->get_word_value(this->gprs.at(regB)->get_value() + this->gprs.at(regC)->get_value() + displacement));
                break;}
            case 7:
                {
                //cout << "Load  csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D" << endl;
                this->csrs.at(regA)->set_value(this->get_word_value(this->gprs.at(regB)->get_value()));
                this->gprs.at(regB)->set_value(this->gprs.at(regB)->get_value() + displacement);
                break;}
            default:
                {
                cout << "ERROR: LOAD INSTRUCTIONS -> UNDEFINED MOD PROVIDED" << endl;
                return true;
                break;}
            }
            break;}
    default:
        {
        cout << "ERROR: INVALID OPERATION CODE PROVIDED" << endl;
        return true;
        break;}
    }
    


    return false;
}

char Emulator :: get_byte_value(uint address){
    for(auto obj : this->memory){
        if(obj->address == address)
            return obj->value;
    }
    cout << "ERROR: NO VALUE ON PROVIDED ADDRESS: " << std::hex << address << endl;
    return 0;
}

int Emulator :: get_word_value(uint address){ //returns real value (not little endian memory representation)
            int ret;
            for(int i = 0; i <  this->memory.size(); i++){
                if(this->memory.at(i)->address == address){
                    //cout << (uint)(this->memory.at(i)->value & 0xff) << " " << (uint)(this->memory.at(i + 1)->value & 0xff) << " " << (uint)(this->memory.at(i + 2)->value & 0xff) << " " << (uint)(this->memory.at(i + 3)->value & 0xff) << endl;
                    int b1 = this->memory.at(i)->value;
                    int b2 = (this->memory.at(i + 1)->value) << 8;
                    int b3 = (this->memory.at(i + 2)->value) << 16;
                    int b4 = (this->memory.at(i + 3)->value) << 24;
                    //printf("%08x %08x %08x %08x\n", b1, b2, b3, b4);
                    ret = (b1 & 0xff) | (b2 & 0xff00) | (b3 & 0xff0000) | (b4 & 0xff000000);
                    return ret;
                }
            }
            cout << "ERROR: NO VALUE ON PROVIDED ADDRESS: " << std::hex << address << endl;
            return NO_VALUE;
        }

bool Emulator :: address_initialized(uint address){
    for(auto obj : this->memory){
        if(obj->address == address)
            return true;
    }
    return false;
}

bool compareAddresses(memory_location* first, memory_location* second){
    return (((uint)first->address < (uint)second->address));
}

void Emulator :: set_word_value(uint address, int value){ //sets in little endian memory representation
    for(uint i = address; i < address + 4; i++){
        if(!address_initialized(i)){
            memory_location* mem = new memory_location();
            mem->address = i;
            mem->value = 0;
            this->memory.push_back(mem);
            sort(this->memory.begin(), this->memory.end(), compareAddresses);
        }
    }
    int b1 = (value >> (8*0)) & 0xff;
    int b2 = (value >> (8*1)) & 0xff;
    int b3 = (value >> (8*2)) & 0xff;
    int b4 = (value >> (8*3)) & 0xff;
    for(int i = 0; i < this->memory.size();i++){
        if(this->memory.at(i)->address == address){
            this->memory.at(i)->value = b1;
            this->memory.at(i + 1)->value = b2;
            this->memory.at(i + 2)->value = b3;
            this->memory.at(i + 3)->value = b4;
            break;
        }
    }
}

void Emulator :: print_memory(){
    cout << "Emulator memory:" << endl;
    for(int i = 0; i < this->memory.size();i++){
        if(i % 8 == 0){
            cout << std::hex << std::setfill('0') << std::setw(8) << this->memory.at(i)->address << ": " << std::setw(2) << (uint)(this->memory.at(i)->value & 0xff);
        }
        else if(i % 8 == 7){
            cout << std::hex << std::setw(2) << (uint)(this->memory.at(i)->value & 0xff) << endl;
        }else if(i % 4 == 3){
            cout << std::hex << std::setw(2) << (uint)(this->memory.at(i)->value & 0xff) << " ";
        }else{
            cout << std::hex << std::setw(2) << (uint)(this->memory.at(i)->value & 0xff);
        }
    }
}

void Emulator :: emulate(){
    bool finished;
    while(true){
        int word = get_word_value(this->gprs.at(15)->get_value());
        if(word == NO_VALUE){
            cout << "ERROR: FALSE VALUE READ WHILE EMULATING ON PC VALUE " << std::hex << this->gprs.at(15)->get_value() << endl;
            break;
        }
    this->gprs.at(15)->increase_by_four();
    char oc = (word >> (4*7)) & 0xf;
    char mod = (word >> (4*6)) & 0xf;
    char regA = (word >> (4*5)) & 0xf;
    char regB = (word >> (4*4)) & 0xf;
    char regC = (word >> (4*3)) & 0xf;
    short displacement = (((word >> (4*0)) & 0xf) | (((word >> (4*1)) & 0xf) << 4) | (((word >> (4*2)) & 0xf) << 8)) & 0xfff;
    if(((displacement >> 11) & 1) == 1){ //for negative value fill in the missing 4 bits
        displacement |= 0xf000;
    }
    //cout << std::dec << (int)regA << " " << (int)regB << " " << (int)regC << " " << std::hex << setfill('0') << setw(3) <<displacement << endl;
    finished = this->resolve_machine_code(oc, mod, regA, regB, regC, displacement);
    
    //check if there is input from the keyboard
    char keyboard_input;
    if(read(STDIN_FILENO, &keyboard_input, 1) == 1){
        //term_in = (ASCII)keyboard_input;
        uint value = (uint)keyboard_input;
        this->set_word_value(TERM_IN, value);
        //generate hardware(terminal) interrupt

        //push status <==> sp -= 4; mem32[sp] <= status;
        this->gprs.at(SP)->decrease_by_four();
        this->set_word_value(this->gprs.at(SP)->get_value(), this->csrs.at(STATUS)->get_value());
        //push pc;
        this->gprs.at(SP)->decrease_by_four();
        this->set_word_value(this->gprs.at(SP)->get_value(), this->gprs.at(PC)->get_value());
        //cause <= 3;
        this->csrs.at(CAUSE)->set_value(3);
        //status <= status&(~0x1);
        this->csrs.at(STATUS)->set_value(this->csrs.at(STATUS)->get_value() & (~0x1));
        //pc <= handler;
        this->gprs.at(PC)->set_value(this->csrs.at(HANDLER)->get_value());
        //cout <<"Karakter: " << std::hex << (int)keyboard_input << " NAKON KLIKTAJA PC IMA VREDNOT " << std::hex << this->gprs.at(PC)->get_value() << endl;
    }

    if(finished)
        break;
    }
}

struct termios stdin_backup_settings; 

void backup_stdin_settings()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &stdin_backup_settings);
}

void Emulator :: set_terminal(){
    if(tcgetattr(STDIN_FILENO, &stdin_backup_settings) < 0)
        exit(1);
	
    static struct termios changed_settings = stdin_backup_settings;
    
    changed_settings.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
    
    changed_settings.c_cc[VMIN] = 0;  
    changed_settings.c_cc[VTIME] = 0; 

    
    changed_settings.c_cflag &= ~(CSIZE | PARENB);
    changed_settings.c_cflag |= CS8;

    
    if(atexit(backup_stdin_settings) != 0)
        exit(1);


    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &changed_settings))
        exit(1);
}

int main(int argc, char* argv[]){
    if(argc != 2){
        cout << "ERROR: INVALID ARGUMENTS PASSED TO EMULATOR" << endl;
        exit(-1);
    }
    char* input = argv[1];
    FILE* input_file = fopen(input, "r");
    if(!input_file){
        cout << "FILE " << input << " DOESN'T EXIST" << endl;
        exit(-1);
    }
    Emulator* emulator = new Emulator(input_file);
    emulator->read_input();
    emulator->set_terminal();
    emulator->fill_memory();
    
    cout << std::flush; 
    emulator->emulate();
    //emulator->print_memory();
    //emulator->print_registers_image();
    exit(0);
}
