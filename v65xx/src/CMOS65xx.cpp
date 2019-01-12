//
// Created by valerino on 06/01/19.
//

#include <CMOS65xx.h>
#include <CLog.h>
#include <string.h>

// flags for the P register
#define P_FLAG_CARRY        0x01
#define P_FLAG_ZERO         0x02
#define P_FLAG_IRQ_DISABLE  0x04
#define P_FLAG_DECIMAL_MODE 0x08
#define P_FLAG_BRK_COMMAND  0x10
#define P_FLAG_UNUSED       0x20
#define P_FLAG_OVERFLOW     0x40
#define P_FLAG_NEGATIVE     0x80

// the stack base address
#define STACK_BASE 0x100

// zeropage
#define ZEROPAGE_DATA_DIRECTION_REGISTER 0
#define ZEROPAGE_OUTPUT_REGISTER 1

// vectors
#define VECTOR_RESET    0xfffc
#define VECTOR_IRQ      0xfffe

void CMOS65xx::irq() {
    // if disable irq flag is NOT set
    if (! (_regP & P_FLAG_IRQ_DISABLE)) {
        // push PC and P on the stack
        pushWord(_regPC);
        pushByte(_regP);

        // set disable irq flag
        _regP |= P_FLAG_IRQ_DISABLE;

        // and set PC to run the IRQ service routine located at the IRQ vector
        _memory->readWord(VECTOR_IRQ, &_regPC);
    }
}

void CMOS65xx::parseInstruction(int addressingMode, uint16_t* operand, int* size) {
    // info taken from https://www.masswerk.at/6502/6502_instruction_set.html
    *operand = 0;
    *size = 0;

    // operand starts at the next byte
    uint16_t operandAddr = _regPC + (uint16_t)1;

    switch (addressingMode) {
        case ADDRESSING_MODE_ACCUMULATOR:
            *size =1;
            *operand = _regA;
            break;

        case ADDRESSING_MODE_ABSOLUTE:
            *size =3;
            _memory->readWord(operandAddr, operand);
            break;

        case ADDRESSING_MODE_ABSOLUTE_INDEXED_X:
            *size =3;
            _memory->readWord(operandAddr, operand);
            break;

        case ADDRESSING_MODE_ABSOLUTE_INDEXED_Y:
            *size =3;
            _memory->readWord(operandAddr, operand);
            break;

        case ADDRESSING_MODE_IMMEDIATE:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            *size =2;
            break;

        case ADDRESSING_MODE_IMPLIED:
            *size =1;
            break;

        case ADDRESSING_MODE_INDIRECT:
            _memory->readWord(operandAddr, operand);
            *size =3;
            break;

        case ADDRESSING_MODE_INDIRECT_INDEXED_X:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            *size =2;
            break;

        case ADDRESSING_MODE_INDIRECT_INDEXED_Y:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            *size =2;
            break;

        case ADDRESSING_MODE_RELATIVE:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            *size =2;
            break;

        case ADDRESSING_MODE_ZEROPAGE:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            *size =2;
            break;

        case ADDRESSING_MODE_ZEROPAGE_INDEXED_X:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            *size =2;
            break;

        case ADDRESSING_MODE_ZEROPAGE_INDEXED_Y:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            *size =2;
            break;

        default:
            // bug!
            CLog::fatal("invalid addressing mode: %d", addressingMode);
    }
}

int CMOS65xx::run(int cyclesToRun) {
    int n = cyclesToRun;
    int remaining = 0;
    while (n) {
        // get opcode byte
        uint8_t b;
        _memory->readByte(_regPC,&b);

        // get opcode from table
        Opcode op = _opcodeMatrix[b];

        // execute instruction and return the number of occupied cycles
        int occupiedCycles = op.cycles;
        int instructionSize = 0;
        (this->*op.ptr)(b, op.mode, &occupiedCycles, &instructionSize);

        // check number of cycles
        if (n >= occupiedCycles) {
            n -= occupiedCycles;
        }
        else {
            // last iteration
            remaining = occupiedCycles - n;
            break;
        }

        // next opcode
        _regPC += instructionSize;
    }
    return remaining;
}

int CMOS65xx::reset() {
    int res = _memory->init();
    if (res != 0) {
        // initialization error
        return res;
    }

    // set registers to initial state
    _regA = 0;
    _regX = 0;
    _regY = 0;
    _regS = 0xfd;
    _regP |= P_FLAG_IRQ_DISABLE;
    _memory->readWord(VECTOR_RESET,&_regPC);
    return 0;
}

CMOS65xx::CMOS65xx(IMemory *mem) {
    _memory = mem;
}

void CMOS65xx::pushWord(uint16_t w) {
    // value must be pushed LE
    uint8_t b = (uint8_t)(w >> 8);
    pushByte(b);

    b = (uint8_t)(w & 0xff);
    pushByte(b);
}

void CMOS65xx::pushByte(uint8_t b) {
    _memory->writeByte(_regS, b);

    // decrement stack
    _regS -= 1;
}

uint16_t CMOS65xx::popWord() {
    // value must be popped LE
    uint8_t b1 = popByte();
    uint8_t b2 = popByte();
    uint16_t w = (b2 << 8) | b1;
    return w;
}

uint8_t CMOS65xx::popByte() {
    uint8_t b;
    _memory->readByte(_regS, &b);

    // increment stack
    _regS += 1;
    return b;
}

void CMOS65xx::logExecution(const char *name, uint8_t opcodeByte, uint16_t operand, int addressingMode) {
#ifdef DEBUG_LOG_EXECUTION
    char mode[16] = {0};
    switch (addressingMode) {
        case ADDRESSING_MODE_ACCUMULATOR:
            strncpy(mode,"ACC",sizeof(mode));
            break;
        case ADDRESSING_MODE_ABSOLUTE:
            strncpy(mode,"ABS",sizeof(mode));
            break;
        case ADDRESSING_MODE_ABSOLUTE_INDEXED_X:
            strncpy(mode,"ABSX",sizeof(mode));
            break;
        case ADDRESSING_MODE_ABSOLUTE_INDEXED_Y:
            strncpy(mode,"ABSY",sizeof(mode));
            break;
        case ADDRESSING_MODE_IMMEDIATE:
            strncpy(mode,"IMM",sizeof(mode));
            break;
        case ADDRESSING_MODE_IMPLIED:
            strncpy(mode,"IMP",sizeof(mode));
            break;
        case ADDRESSING_MODE_INDIRECT:
            strncpy(mode,"IND",sizeof(mode));
            break;
        case ADDRESSING_MODE_INDIRECT_INDEXED_X:
            strncpy(mode,"INDX",sizeof(mode));
            break;
        case ADDRESSING_MODE_INDIRECT_INDEXED_Y:
            strncpy(mode,"INDY",sizeof(mode));
            break;
        case ADDRESSING_MODE_RELATIVE:
            strncpy(mode,"REL",sizeof(mode));
            break;
        case ADDRESSING_MODE_ZEROPAGE:
            strncpy(mode,"ZP",sizeof(mode));
            break;
        case ADDRESSING_MODE_ZEROPAGE_INDEXED_X:
            strncpy(mode,"ZPX",sizeof(mode));
            break;
        case ADDRESSING_MODE_ZEROPAGE_INDEXED_Y:
            strncpy(mode,"ZPY",sizeof(mode));
            break;
        default:
            break;
    }

    // P to string
    char s[16] = {0};
    if (_regP & P_FLAG_CARRY) {
        strcat(s,"C");
    }
    if (_regP & P_FLAG_ZERO) {
        strcat(s,"Z");
    }
    if (_regP & P_FLAG_IRQ_DISABLE) {
        strcat(s,"I");
    }
    if (_regP & P_FLAG_DECIMAL_MODE) {
        strcat(s,"D");
    }
    if (_regP & P_FLAG_BRK_COMMAND) {
        strcat(s,"B");
    }
    if (_regP & P_FLAG_OVERFLOW) {
        strcat(s,"O");
    }
    if (_regP & P_FLAG_NEGATIVE) {
        strcat(s,"N");
    }

    char registers[128] = {};
    snprintf(registers, sizeof(registers), "A=$%02x, X=$%02x, Y=$%02x, P=$%02x(%s), S=$%02x, AddrMode=%s", _regA, _regX, _regY, _regP, s, _regS, mode);

    if (addressingMode == ADDRESSING_MODE_IMPLIED) {
        // no operand
        CLog::print("$%x: %02X\t\t\t%s\t\t\t\t%s",
                _regPC, opcodeByte, name, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ACCUMULATOR) {
        // operand is A
        CLog::print("$%x: %02X\t\t\t%s A\t\t\t%s",
                    _regPC, opcodeByte, name, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_X || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_X) {
        // operand, X/Y
        CLog::print("$%04x: %02X %02X\t\t%s $%04x, X\t%s",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_Y || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_Y) {
        // operand, X/Y
        CLog::print("$%04x: %02X %02X\t\t%s $%04x, Y\t%s",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_INDIRECT) {
        // (operand)
        CLog::print("$%04x: %02X %02X\t\t%s ($%04x)\t%s",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_INDIRECT_INDEXED_X) {
        // (operand, X)
        CLog::print("$%04x: %02X %02X\t\t%s ($%02x, X)\t%s",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_INDIRECT_INDEXED_Y) {
        // (operand), Y
        CLog::print("$%04x: %02X %02X\t\t%s ($%02x), Y\t%s",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE) {
        // absolute, 2 bytes operand
        CLog::print("$%04x: %02X %02X\t\t%s $%04x\t\t%s",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else {
        // immediate / relative / zeropage (1 byte operand)
        CLog::print("$%04x: %02X %02X\t\t%s $%02x\t\t\t%s",
                _regPC, opcodeByte, operand, name, operand, registers);
    }
#endif
}

void CMOS65xx::ADC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::AND(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::ASL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BCC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BCS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BEQ(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BIT(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BMI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BNE(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BPL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BRK(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BVC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::BVS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::CLC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::CLD(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::CLI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::CLV(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::CMP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::CPX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::CPY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::DEC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::DEX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::DEY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::EOR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::INC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::INX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::INY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::JMP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::JSR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::LDA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::LDX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::LDY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::LSR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::NOP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::ORA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::PHA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::PHP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::PLA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::PLP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::ROL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::ROR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::RTI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::RTS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SBC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SEC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SED(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SEI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::STA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::STX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::STY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::TAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::TAY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::TSX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::TXA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::TXS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::TYA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::AHX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::ANC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::ALR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::ARR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::AXS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::DCP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::ISC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::LAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::LAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::RLA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::RRA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SHX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SHY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SLO(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::SRE(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::TAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::XAA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    
    parseInstruction(addressingMode, &operand, size);
    logExecution(__FUNCTION__, opcodeByte, operand, addressingMode);

    // execute
}

void CMOS65xx::KIL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    CLog::fatal("invalid opcode!");
}
