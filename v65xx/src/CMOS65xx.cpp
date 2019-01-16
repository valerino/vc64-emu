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

#define IS_FLAG_CARRY _regP & P_FLAG_CARRY
#define IS_FLAG_ZERO _regP & P_FLAG_ZERO
#define IS_FLAG_IRQ_DISABLE _regP & P_FLAG_IRQ_DISABLE
#define IS_FLAG_DECIMAL_MODE _regP & P_FLAG_DECIMAL_MODE
#define IS_FLAG_BRK_COMMAND _regP & P_FLAG_BRK_COMMAND
#define IS_FLAG_OVERFLOW _regP & P_FLAG_OVERFLOW
#define IS_FLAG_NEGATIVE _regP & P_FLAG_NEGATIVE
#define SET_FLAG_CARRY(_x_) if(_x_) { _regP |= P_FLAG_CARRY; } else { _regP &= ~P_FLAG_CARRY; }
#define SET_FLAG_ZERO(_x_) if(_x_) { _regP |= P_FLAG_ZERO; } else { _regP &= ~P_FLAG_ZERO; }
#define SET_FLAG_IRQ_DISABLE(_x_) if(_x_) { _regP |= P_FLAG_IRQ_DISABLE; } else { _regP &= ~P_FLAG_IRQ_DISABLE; }
#define SET_FLAG_DECIMAL_MODE(_x_) if(_x_) { _regP |= P_FLAG_DECIMAL_MODE; } else { _regP &= ~P_FLAG_DECIMAL_MODE; }
#define SET_FLAG_BRK_COMMAND(_x_) if(_x_) { _regP |= P_FLAG_BRK_COMMAND; } else { _regP &= ~P_FLAG_BRK_COMMAND; }
#define SET_FLAG_OVERFLOW(_x_) if(_x_) { _regP |= P_FLAG_OVERFLOW; } else { _regP &= ~P_FLAG_OVERFLOW; }
#define SET_FLAG_NEGATIVE(_x_) if(_x_) { _regP |= P_FLAG_NEGATIVE; } else { _regP &= ~P_FLAG_NEGATIVE; }

// the stack base address
#define STACK_BASE 0x100

// zeropage
#define ZEROPAGE_DATA_DIRECTION_REGISTER 0
#define ZEROPAGE_OUTPUT_REGISTER 1

// vectors
#define VECTOR_RESET    0xfffc
#define VECTOR_IRQ      0xfffe

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
    snprintf(registers, sizeof(registers), "PC=$%04x, A=$%02x, X=$%02x, Y=$%02x, P=$%02x(%s), S=$%02x, AddrMode=%s", _regPC, _regA, _regX, _regY, _regP, s, _regS, mode);

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
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE) {
        // absolute, 2 bytes operand
        uint8_t opByte1 = operand >> 8;
        uint8_t opByte2 = operand & 0xff;
        CLog::print("$%04x: %02X %02X %02X\t\t%s $%04x\t\t%s",
                    _regPC, opcodeByte, opByte2, opByte1, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_X || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_X) {
        // 2 bytes operand, X/Y
        uint8_t opByte1 = operand >> 8;
        uint8_t opByte2 = operand & 0xff;
        CLog::print("$%04x: %02X %02X %02X\t\t%s $%04x, X\t%s",
                    _regPC, opcodeByte, opByte2, opByte1, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_Y || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_Y) {
        // 2 bytes operand, X/Y
        uint8_t opByte1 = operand >> 8;
        uint8_t opByte2 = operand & 0xff;
        CLog::print("$%04x: %02X %02X %02X\t\t%s $%04x, Y\t%s",
                    _regPC, opcodeByte, opByte2, opByte1, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_INDIRECT) {
        // (2 bytes operand)
        uint8_t opByte1 = operand >> 8;
        uint8_t opByte2 = operand & 0xff;
        CLog::print("$%04x: %02X %02X %02X\t\t%s ($%04x)\t%s",
                    _regPC, opcodeByte, opByte2, opByte1, name, operand, registers);
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
    else if (addressingMode == ADDRESSING_MODE_RELATIVE) {
        // 1 byte operand, relative to PC
        uint16_t op = _regPC + 2 + operand;
        CLog::print("$%04x: %02X %02X\t\t%s $%04x\t\t%s",
                    _regPC, opcodeByte, operand, name, op, registers);
    }
    else {
        // immediate / zeropage (1 byte operand)
        CLog::print("$%04x: %02X %02X\t\t%s $%02x\t\t\t%s",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
#endif
}

void CMOS65xx::parseInstruction(uint8_t opcodeByte, const char* functionName, int addressingMode, uint16_t* operandAddress, uint16_t* operand, int* size) {
    *operand = 0;
    *size = 0;

    // operand starts at the next byte
    uint16_t operandAddr = _regPC + 1;
    *operandAddress = operandAddr;

    switch (addressingMode) {
        case ADDRESSING_MODE_ACCUMULATOR:
            *operand = _regA;
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *size = 1;
            break;

        case ADDRESSING_MODE_ABSOLUTE:
            _memory->readWord(operandAddr, operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *size = 3;
            break;

        case ADDRESSING_MODE_ABSOLUTE_INDEXED_X:
            _memory->readWord(operandAddr, operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *operand += _regX;
            *size = 3;
            break;

        case ADDRESSING_MODE_ABSOLUTE_INDEXED_Y:
            _memory->readWord(operandAddr, operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *operand += _regY;
            *size = 3;
            break;

        case ADDRESSING_MODE_IMMEDIATE:
            _memory->readByte(operandAddr, (uint8_t *) operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *size = 2;
            break;

        case ADDRESSING_MODE_IMPLIED:
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *size = 1;
            break;

        case ADDRESSING_MODE_INDIRECT: {
            _memory->readWord(operandAddr, operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            uint16_t w;
            _memory->readWord(*operand, &w);
            *operand = w;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_INDIRECT_INDEXED_X: {
            _memory->readByte(operandAddr, (uint8_t *) operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            uint16_t w;
            _memory->readWord(*operand + _regX, &w);
            *operand = w;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_INDIRECT_INDEXED_Y: {
            _memory->readByte(operandAddr, (uint8_t *) operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            uint16_t w;
            _memory->readWord(*operand, &w);
            w += _regY;
            *operand = w;
            *size = 2;
            break;
        }
        case ADDRESSING_MODE_RELATIVE:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *operand = _regPC + 2 + *operand;
            *size =2;
            break;

        case ADDRESSING_MODE_ZEROPAGE:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *size =2;
            break;

        case ADDRESSING_MODE_ZEROPAGE_INDEXED_X:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *operand += _regX;
            *size =2;
            break;

        case ADDRESSING_MODE_ZEROPAGE_INDEXED_Y:
            _memory->readByte(operandAddr, (uint8_t*)operand);
            logExecution(functionName, opcodeByte, *operand, addressingMode);

            *operand += _regY;
            *size =2;
            break;

        default:
            // bug!
            logExecution(functionName, opcodeByte, *operand, addressingMode);
            CLog::fatal("invalid addressing mode: %d", addressingMode);
            return;
    }
}

void CMOS65xx::postExecHandlePageCrossing(int addressingMode, int* cycles) {
    if (IS_FLAG_CARRY &&
        (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_X || addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_Y ||
         addressingMode == ADDRESSING_MODE_INDIRECT_INDEXED_Y)) {
        // add a cycle
        *cycles +=1;
    }
}

void CMOS65xx::postExecHandleAccumulatorAddressing(int addressingMode, uint16_t operand) {
    if (addressingMode == ADDRESSING_MODE_ACCUMULATOR) {
        _regA = operand;
    }
}

void CMOS65xx::postExecHandleMemoryOperand(int addressingMode, uint16_t operandAddress, uint16_t operand) {
    if (addressingMode == ADDRESSING_MODE_ZEROPAGE || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_X || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_Y) {
        _memory->writeByte(operandAddress, operand & 0xff);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE || addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_X || addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_Y) {
        _memory->writeWord(operandAddress, operand);
    }
}

void CMOS65xx::irq() {
    // if disable irq flag is NOT set
    if (! (IS_FLAG_IRQ_DISABLE)) {
        // push PC and P on the stack
        pushWord(_regPC);
        pushByte(_regP);

        // set disable irq flag
        SET_FLAG_IRQ_DISABLE(true);

        // and set PC to run the IRQ service routine located at the IRQ vector
        _memory->readWord(VECTOR_IRQ, &_regPC);
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

void CMOS65xx::ADC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
    uint16_t sum = _regA + operand + (IS_FLAG_CARRY);
    SET_FLAG_CARRY(sum > 0xff)
    SET_FLAG_ZERO(sum == 0)
    SET_FLAG_NEGATIVE(sum & 0x80)
    int overflow =  ~(_regA ^ operand) & (_regA ^ sum) & 0x80;
    SET_FLAG_OVERFLOW(overflow)

    // done
    _regA = sum & 0xff;
    postExecHandlePageCrossing(addressingMode, cycles);
}

void CMOS65xx::AND(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
    _regA = _regA & operand;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(operand & 0x80)
}

void CMOS65xx::ASL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
    SET_FLAG_CARRY(operand & 0x80)
    operand <<= 1;
    postExecHandleAccumulatorAddressing(addressingMode, operand);
    postExecHandleMemoryOperand(addressingMode, operandAddr, operand);
}

void CMOS65xx::BCC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BCS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BEQ(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BIT(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BMI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BNE(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BPL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BRK(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BVC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::BVS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::CLC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::CLD(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::CLI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::CLV(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::CMP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::CPX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::CPY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::DEC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::DEX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::DEY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::EOR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::INC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::INX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::INY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::JMP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::JSR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::LDA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::LDX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::LDY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::LSR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::NOP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::ORA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::PHA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::PHP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::PLA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::PLP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::ROL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::ROR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::RTI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::RTS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SBC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SEC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SED(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SEI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::STA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::STX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::STY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::TAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::TAY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::TSX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::TXA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::TXS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::TYA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::AHX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::ANC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::ALR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::ARR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::AXS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::DCP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::ISC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::LAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::LAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::RLA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::RRA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SHX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SHY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SLO(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::SRE(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::TAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::XAA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
}

void CMOS65xx::KIL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size);

    // exec
    CLog::fatal("invalid opcode!");
}
