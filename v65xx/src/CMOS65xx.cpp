//
// Created by valerino on 06/01/19.
//

#include <CMOS65xx.h>
#include <CLog.h>
#include <CBuffer.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
#define VECTOR_NMI      0xfffa
#define VECTOR_RESET    0xfffc
#define VECTOR_IRQ      0xfffe

/**
 * only for debugging, print execution
 * @param opcodeName name of the opcode
 * @param opcodeByte the opcode
 * @param operand the operand if any
 * @param addressingMode one of the addressing modes
 */
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
            strncpy(mode,"UNK",sizeof(mode));
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
    if (s[0] == '\0') {
        // empty
        s[0]='-';
    }
    char registers[128] = {};
    snprintf(registers, sizeof(registers), "AddrMode=%s,\tPC=$%04x, A=$%02x, X=$%02x, Y=$%02x, S=$%02x, P=$%02x(%s)", mode, _regPC, _regA, _regX, _regY, _regS, _regP, s);

    if (addressingMode == ADDRESSING_MODE_IMPLIED) {
        // no operand
        CLog::printRaw("\t$%04x: %02X\t\t\t%s\t\t\t\t%s\n",
                    _regPC, opcodeByte, name, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ACCUMULATOR) {
        // operand is A
        CLog::printRaw("\t$%04x: %02X\t\t\t%s A\t\t\t%s\n",
                    _regPC, opcodeByte, name, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE) {
        // absolute, 2 bytes operand
        uint8_t opByte1 = operand >> 8;
        uint8_t opByte2 = operand & 0xff;
        CLog::printRaw("\t$%04x: %02X %02X %02X\t\t%s $%04x\t\t%s\n",
                    _regPC, opcodeByte, opByte2, opByte1, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_X || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_X) {
        // 2 bytes operand, X/Y
        uint8_t opByte1 = operand >> 8;
        uint8_t opByte2 = operand & 0xff;
        CLog::printRaw("\t$%04x: %02X %02X %02X\t\t%s $%04x, X\t%s\n",
                    _regPC, opcodeByte, opByte2, opByte1, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_Y || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_Y) {
        // 2 bytes operand, X/Y
        uint8_t opByte1 = operand >> 8;
        uint8_t opByte2 = operand & 0xff;
        CLog::printRaw("\t$%04x: %02X %02X %02X\t\t%s $%04x, Y\t%s\n",
                    _regPC, opcodeByte, opByte2, opByte1, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_INDIRECT) {
        // (2 bytes operand)
        uint8_t opByte1 = operand >> 8;
        uint8_t opByte2 = operand & 0xff;
        CLog::printRaw("\t$%04x: %02X %02X %02X\t\t%s ($%04x)\t%s\n",
                    _regPC, opcodeByte, opByte2, opByte1, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_INDIRECT_INDEXED_X) {
        // (operand, X)
        CLog::printRaw("\t$%04x: %02X %02X\t\t%s ($%02x, X)\t%s\n",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_INDIRECT_INDEXED_Y) {
        // (operand), Y
        CLog::printRaw("\t$%04x: %02X %02X\t\t%s ($%02x), Y\t%s\n",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else if (addressingMode == ADDRESSING_MODE_RELATIVE) {
        // 1 byte operand, relative to PC
        CLog::printRaw("\t$%04x: %02X %04X\t\t%s $%04x\t\t%s\n",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
    else {
        // immediate / zeropage (1 byte operand)
        CLog::printRaw("\t$%04x: %02X %02X\t\t%s $%02x\t\t\t%s\n",
                    _regPC, opcodeByte, operand, name, operand, registers);
    }
#endif
}

/**
 * parse instruction depending on the addressing mode
 * @param opcodeByte the first byte (opcode)
 * @param functionName __FUNCTION__
 * @param addressingMode one of the addressing modes
 * @param operandAddress on return, address from where the operand is fetched (always = PC + 1)
 * @param operand on return, the operand if any
 * @param cycles on input, instruction cycles. on output, instruction cycles fixed depending on page crossing and instruction
 * @param on return, the instruction size
 * @return
 */
void CMOS65xx::parseInstruction(uint8_t opcodeByte, const char* functionName, int addressingMode, uint16_t* operandAddress, uint16_t* operand, int* size, int* cycles) {
    *operand = 0;
    *size = 0;

    // operand starts at the next byte
    uint16_t operandAddr = _regPC + 1;
    *operandAddress = operandAddr;

    switch (addressingMode) {
        case ADDRESSING_MODE_ACCUMULATOR: {
            uint16_t dw = _regA;
            logExecution(functionName, opcodeByte, dw, addressingMode);
            *operand = dw;
            *size = 1;
            break;
        }

        case ADDRESSING_MODE_ABSOLUTE: {
            uint16_t dw;
            _memory->readWord(operandAddr, &dw);
            logExecution(functionName, opcodeByte, dw, addressingMode);
            *operand = dw;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_ABSOLUTE_INDEXED_X: {
            uint16_t dw;
            _memory->readWord(operandAddr, &dw);
            logExecution(functionName, opcodeByte, dw, addressingMode);
            dw += _regX;

            // cycles may need adjustment
            handlePageCrossing(addressingMode, dw, cycles);
            *operand = dw;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_ABSOLUTE_INDEXED_Y: {
            uint16_t dw;
            _memory->readWord(operandAddr, &dw);
            logExecution(functionName, opcodeByte, dw, addressingMode);
            dw += _regY;

            // cycles may need adjustment
            handlePageCrossing(addressingMode, dw, cycles);
            *operand = dw;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_IMMEDIATE: {
            uint8_t tmp;
            _memory->readByte(operandAddr, &tmp);
            logExecution(functionName, opcodeByte, tmp, addressingMode);
            *operand = tmp;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_IMPLIED: {
            // no operand ...
            logExecution(functionName, opcodeByte, *operand, addressingMode);
            *size = 1;
            break;
        }

        case ADDRESSING_MODE_INDIRECT: {
            // read the target address (it's indirect)
            uint16_t tmp;
            _memory->readWord(operandAddr, &tmp);
            logExecution(functionName, opcodeByte, tmp, addressingMode);

            // emulate 6502 access bug on page boundary: if operand falls on page boundary, msb is not fetched correctly
            uint16_t addr = (tmp & 0xff00) | ((tmp + 1) & 0x00ff);
            uint16_t dw;
            _memory->readWord(addr, &dw);
            *operand = dw;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_INDIRECT_INDEXED_X: {
            // read the target address byte per byte, in zeropage
            uint8_t lsbAddr;
            _memory->readByte(operandAddr, &lsbAddr);
            lsbAddr += _regX;
            uint8_t msbAddr = (lsbAddr + 1);

            // read from the address in zeropage
            uint16_t addr = (msbAddr << 8) | lsbAddr;
            logExecution(functionName, opcodeByte, lsbAddr, addressingMode);
            uint16_t dw;
            _memory->readWord(addr, &dw);
            *operand = dw;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_INDIRECT_INDEXED_Y: {
            // read the target address byte per byte, in zeropage
            uint8_t lsbAddr;
            _memory->readByte(operandAddr, &lsbAddr);
            uint8_t msbAddr = (lsbAddr + 1);

            // read from the address in zeropage
            uint16_t addr = _regY + ((msbAddr << 8) | lsbAddr);
            logExecution(functionName, opcodeByte, lsbAddr, addressingMode);
            uint16_t dw;
            _memory->readWord(addr, &dw);
            *operand = dw;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_RELATIVE:  {
            uint8_t tmp;
            _memory->readByte(operandAddr, &tmp);
            uint16_t dw = tmp;
            if (tmp & 0x80) {
                // sign extend
                dw |= 0xff00;
            }
            dw = dw + _regPC + 2;
            logExecution(functionName, opcodeByte, dw, addressingMode);

            *operand = dw;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_ZEROPAGE: {
            uint8_t tmp;
            _memory->readByte(operandAddr, &tmp);
            logExecution(functionName, opcodeByte, tmp, addressingMode);

            *operand = tmp;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_ZEROPAGE_INDEXED_X: {
            uint8_t tmp;
            _memory->readByte(operandAddr, &tmp);
            logExecution(functionName, opcodeByte, tmp, addressingMode);

            // result address is also in zeropage
            tmp += _regX;
            *operand = tmp;

            *size = 2;
            break;
        }

        case ADDRESSING_MODE_ZEROPAGE_INDEXED_Y: {
            uint8_t tmp;
            _memory->readByte(operandAddr, &tmp);
            logExecution(functionName, opcodeByte, tmp, addressingMode);

            // result address is also in zeropage
            tmp += _regY;
            *operand = tmp;
            *size = 2;
            break;
        }

        default:
            // bug!
            logExecution(functionName, opcodeByte, *operand, addressingMode);
            CLog::fatal("invalid addressing mode: %d", addressingMode);
            return;
    }
}

/**
 * handles page crossing on certain addressing modes, adding +1 to instruction cycles
 * @param operand the operand
 * @param cycles on input, instruction cycles. on output, cycles + 1 (branch taken) or eventually cycles + 2 (page crossing)
 */
void CMOS65xx::handlePageCrossing(int addressingMode, uint16_t operand, int *cycles) {
    uint16_t page = operand & 0xff00;
    uint16_t addr;
    switch (addressingMode) {
        case ADDRESSING_MODE_ABSOLUTE_INDEXED_X:
            addr = operand + _regX;
            if (addr & 0xff00 != page) {
                *cycles += 1;
            }
            break;
        case ADDRESSING_MODE_ABSOLUTE_INDEXED_Y:
        case ADDRESSING_MODE_INDIRECT_INDEXED_Y:
            addr = operand + _regY;
            if (addr & 0xff00 != page) {
                *cycles += 1;
            }
            break;
        default:
            break;
    }
}

/**
 * to be called in branch instructions to increment cycle count on page crossing
 * @param operand the operand (the branch target)
 * @param cycles on input, instruction cycles. on output, cycles + 1 (branch taken) or eventually cycles + 2 (page crossing)
 */
void CMOS65xx::handlePageCrossingOnBranch(uint16_t operand, int *cycles) {
    // branch is taken
    *cycles+=1;

    if (operand > (_regPC % 0xff)) {
        // page transition, add another cycle
        *cycles+=1;
    }
}

/**
 * to be called post executing certain instructions, handle rewriting memory or accumulator with the result
 * @param addressingMode one of the addressing modes
 * @param operandAddress the operand address
 * @param operand the operand
 */
void CMOS65xx::postExecHandleResultOperand(int addressingMode, uint16_t operandAddress, uint16_t operand) {
    if (addressingMode == ADDRESSING_MODE_ZEROPAGE || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_X || addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_Y) {
        // store at address into zeropage
        _memory->writeByte(operandAddress, operand & 0xff);
    }
    else if (addressingMode == ADDRESSING_MODE_ABSOLUTE || addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_X || addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_Y) {
        // store at address
        _memory->writeWord(operandAddress, operand);
    }
    else if (addressingMode == ADDRESSING_MODE_ACCUMULATOR) {
        // store into A
        _regA = operand & 0xff;
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
        if (_regPC == 0x4e4) {
            int bb = 0;
            bb++;
        }
        if (_regPC == 0x581) {
            int bb = 0;
            bb++;
        }
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
    _regP = 0x16;
    _regP |= P_FLAG_BRK_COMMAND;
    _memory->readWord(VECTOR_RESET,&_regPC);
#ifdef DEBUG_RUN_FUNCTIONAL_TESTS
    // overwrite memory with functional test suite
    dbgLoadFunctionalTest();
#endif
    return 0;
}

CMOS65xx::CMOS65xx(IMemory *mem) {
    _memory = mem;
}

/**
 * push a word (16 bit) on the stack
 * @param w the word to be pushed
 */
void CMOS65xx::pushWord(uint16_t w) {
    // value must be pushed LE
    uint8_t b = (uint8_t)(w >> 8);
    pushByte(b);

    b = (uint8_t)(w & 0xff);
    pushByte(b);
}

/**
 * push a byte on the stack
 * @param b the byte to be pushed
 */
void CMOS65xx::pushByte(uint8_t b) {
    _memory->writeByte(STACK_BASE + _regS, b);

    // decrement stack
    _regS -= 1;
}

/**
 * pop a word (16 bit) from the stack
 * @return the word
 */
uint16_t CMOS65xx::popWord() {
    // value must be popped LE
    uint8_t b1 = popByte();
    uint8_t b2 = popByte();
    uint16_t w = (b2 << 8) | b1;
    return w;
}

/**
 * pop a byte from the stack
 * @return the word
 */
uint8_t CMOS65xx::popByte() {
    // increment stack
    _regS += 1;
    uint8_t b;
    _memory->readByte(STACK_BASE + _regS, &b);
    return b;
}

void CMOS65xx::ADC_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    uint16_t res = _regA + operand + (IS_FLAG_CARRY);
    SET_FLAG_CARRY(res > 0xff)
    SET_FLAG_ZERO(res == 0)
    SET_FLAG_NEGATIVE(res & 0x80)
    int overflow = ~((_regA ^ operand) & (_regA ^ res)) & 0x80;
    SET_FLAG_OVERFLOW(overflow)

    if (IS_FLAG_DECIMAL_MODE) {
        // handle decimal mode
        SET_FLAG_CARRY(false);
        if ((res & 0x0f) > 0x09) {
            res += 0x06;
        }
        if ((res & 0xf0) > 0x90) {
            res += 0x60;
            SET_FLAG_CARRY(true);
        }
        // NOTE: an additional cycle is added only in 65c02, not in vanilla 6502 !!!
    }
    _regA = res & 0xff;
}

void CMOS65xx::ADC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ADC_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::AND_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regA &= (operand & 0xff);
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::AND(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    AND_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::ASL_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    SET_FLAG_CARRY(operand & 0x80)
    operand <<= 1;
    postExecHandleResultOperand(addressingMode, operandAddr, operand);
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(operand & 0x80)
}

void CMOS65xx::ASL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ASL_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::BCC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    if (! (IS_FLAG_CARRY)) {
        handlePageCrossingOnBranch(operand, cycles);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::BCS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    if (IS_FLAG_CARRY) {
        handlePageCrossingOnBranch(operand, cycles);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::BEQ(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    if (IS_FLAG_ZERO) {
        handlePageCrossingOnBranch(operand, cycles);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::BIT(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SET_FLAG_ZERO ((_regA & operand) == 0);
    SET_FLAG_NEGATIVE(operand & 0x80)
    SET_FLAG_OVERFLOW(operand & 0x40)
}

void CMOS65xx::BMI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    if (IS_FLAG_NEGATIVE) {
        handlePageCrossingOnBranch(operand, cycles);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::BNE(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    if (! (IS_FLAG_ZERO)) {
        handlePageCrossingOnBranch(operand, cycles);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::BPL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    if (! (IS_FLAG_NEGATIVE)) {
        handlePageCrossingOnBranch(operand, cycles);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::BRK(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    // push PC and P on stack
    pushWord(_regPC + *size);
    pushByte(_regP);

    // set break flag
    SET_FLAG_BRK_COMMAND(true);

    // and set PC to run the IRQ service routine located at the IRQ vector
    _memory->readWord(VECTOR_IRQ, &_regPC);
}

void CMOS65xx::BVC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    if (! (IS_FLAG_OVERFLOW)) {
        handlePageCrossingOnBranch(operand, cycles);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::BVS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    if (IS_FLAG_OVERFLOW) {
        handlePageCrossingOnBranch(operand, cycles);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::CLC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SET_FLAG_CARRY(false)
}

void CMOS65xx::CLD(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SET_FLAG_DECIMAL_MODE(false)
}

void CMOS65xx::CLI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SET_FLAG_IRQ_DISABLE(false)
}

void CMOS65xx::CLV(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SET_FLAG_OVERFLOW(false)
}

void CMOS65xx::CMP_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    operand &= 0xff;
    uint16_t tmp = _regA - operand;
    SET_FLAG_CARRY(tmp <= 0xff)
    SET_FLAG_ZERO((tmp & 0xff) == 0)
    SET_FLAG_NEGATIVE(tmp & 0x80)
}

void CMOS65xx::CMP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    CMP_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::CPX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    operand &= 0xff;
    SET_FLAG_CARRY(_regX >= operand)
    SET_FLAG_ZERO(_regX == operand)
    SET_FLAG_NEGATIVE((_regX - operand) & 0x80)
}

void CMOS65xx::CPY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    operand &= 0xff;
    SET_FLAG_CARRY(_regY >= operand)
    SET_FLAG_ZERO(_regY == operand)
    SET_FLAG_NEGATIVE((_regY - operand) & 0x80)
}

void CMOS65xx::DEC_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    uint8_t v = ((operand & 0xff) - 1) % 256;
    operand = v;
    SET_FLAG_ZERO(operand == 0);
    SET_FLAG_NEGATIVE(operand & 0x80);
    postExecHandleResultOperand(addressingMode, operandAddr, operand);
}

void CMOS65xx::DEC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    DEC_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::DEX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t v = (_regX - 1) % 256;
    _regX = v;
    SET_FLAG_ZERO(_regX == 0);
    SET_FLAG_NEGATIVE(_regX & 0x80);
}

void CMOS65xx::DEY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t v = (_regY - 1) % 256;
    _regY = v;
    SET_FLAG_ZERO(_regY == 0);
    SET_FLAG_NEGATIVE(_regY & 0x80);
}

void CMOS65xx::EOR_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regA ^= (operand & 0xff);
    SET_FLAG_ZERO(_regA == 0);
    SET_FLAG_NEGATIVE(_regA & 0x80);
}

void CMOS65xx::EOR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    EOR_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::INC_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    uint8_t v = ((operand & 0xff) + 1) % 256;
    operand = v;
    SET_FLAG_ZERO(operand == 0);
    SET_FLAG_NEGATIVE(operand & 0x80);
    postExecHandleResultOperand(addressingMode, operandAddr, operand);
}

void CMOS65xx::INC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    INC_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::INX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t v = (_regX + 1) % 256;
    _regX = v;

    SET_FLAG_ZERO(_regX == 0);
    SET_FLAG_NEGATIVE(_regX & 0x80);
}

void CMOS65xx::INY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t v = (_regY + 1) % 256;
    _regY = v;

    SET_FLAG_ZERO(_regY == 0);
    SET_FLAG_NEGATIVE(_regY & 0x80);
}

void CMOS65xx::JMP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    _regPC = operand;
    *size = 0;
}

void CMOS65xx::JSR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    // push return address on the stack
    pushWord(_regPC + *size - 1);
    _regPC = operand;
    *size = 0;
}

void CMOS65xx::LDA_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regA = operand & 0xff;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::LDA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    LDA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::LDX_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regX = operand & 0xff;
    SET_FLAG_ZERO(_regX == 0)
    SET_FLAG_NEGATIVE(_regX & 0x80)
}

void CMOS65xx::LDX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    LDX_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::LDY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    _regY = operand & 0xff;
    SET_FLAG_ZERO(_regY == 0)
    SET_FLAG_NEGATIVE(_regY & 0x80)
}

void CMOS65xx::LSR_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    SET_FLAG_CARRY(operand & 1)
    operand >>= 1;
    SET_FLAG_ZERO(operand == 0)
    SET_FLAG_NEGATIVE(operand & 0x80)
    postExecHandleResultOperand(addressingMode, operandAddr, operand);
}

void CMOS65xx::LSR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    LSR_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::NOP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // nothing, nop!
}

void CMOS65xx::ORA_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regA |= (operand & 0xff);
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::ORA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ORA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::PHA_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    pushByte(_regA);
}

void CMOS65xx::PHA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    PHA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::PHP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    pushByte(_regP);
}

void CMOS65xx::PLA_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regA = popByte();
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::PLA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    PLA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::PLP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    _regP = popByte();
}

void CMOS65xx::ROL_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    uint16_t  res = (operand << 1) | (IS_FLAG_CARRY);
    SET_FLAG_CARRY(operand & 0x80)
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(res & 0x80)
    postExecHandleResultOperand(addressingMode, operandAddr, res);
}

void CMOS65xx::ROL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ROL_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::ROR_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    uint16_t  res = (operand >> 1) | ((IS_FLAG_CARRY) << 7);
    SET_FLAG_CARRY(operand & 1)
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(res & 0x80)
    postExecHandleResultOperand(addressingMode, operandAddr, res);
}

void CMOS65xx::ROR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ROR_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::RTI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    _regP = popByte();
    _regPC = popWord();
    *size = 0;
}

void CMOS65xx::RTS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    _regPC = popWord();
    _regPC += 1;
    *size = 0;
}

void CMOS65xx::SBC_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    uint16_t res = _regA - operand + !(IS_FLAG_CARRY);
    SET_FLAG_CARRY(res <= 0xff)
    SET_FLAG_ZERO(res == 0)
    SET_FLAG_NEGATIVE(res & 0x80)
    int overflow = ((_regA ^ operand) & (_regA ^ res)) & 0x80;
    SET_FLAG_OVERFLOW(overflow)

    if (IS_FLAG_DECIMAL_MODE) {
        // handle decimal mode
        SET_FLAG_CARRY(false);
        res -= 0x66;
        if ((res & 0x0F) > 0x09) {
            res += 0x06;
        }
        if ((res & 0xF0) > 0x90) {
            res += 0x60;
            SET_FLAG_CARRY(true);
        }

        // NOTE: an additional cycle is added only in 65c02, not in vanilla 6502 !!!
    }
    _regA = res & 0xff;
}

void CMOS65xx::SBC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SBC_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::SEC_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    SET_FLAG_CARRY(true)
}

void CMOS65xx::SEC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SEC_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::SED(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SET_FLAG_DECIMAL_MODE(true)
}

void CMOS65xx::SEI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    SET_FLAG_IRQ_DISABLE(true)
}

void CMOS65xx::STA_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _memory->writeByte(operand, _regA);
}

void CMOS65xx::STA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    STA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::STX_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _memory->writeByte(operand, _regX);
}

void CMOS65xx::STX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    STX_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::STY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    _memory->writeByte(operand, _regY);
}

void CMOS65xx::TAX_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regX = _regA;
    SET_FLAG_ZERO(_regX == 0)
    SET_FLAG_NEGATIVE(_regX & 0x80)
}

void CMOS65xx::TAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    TAX_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::TAY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    _regY = _regA;
    SET_FLAG_ZERO(_regY == 0)
    SET_FLAG_NEGATIVE(_regY & 0x80)
}

void CMOS65xx::TSX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    _regX = _regS;
    SET_FLAG_ZERO(_regX == 0)
    SET_FLAG_NEGATIVE(_regX & 0x80)
}

void CMOS65xx::TXA_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regA = _regX;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::TXA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    TXA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::TXS_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regS = _regX;
}

void CMOS65xx::TXS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    TXS_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::TYA_internal(int addressingMode, uint16_t operandAddr, uint16_t operand) {
    _regA = _regY;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::TYA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    TYA_internal(addressingMode, operandAddr, operand);
}

/**
 * undocumented instructions: most of these instructions are implemented as combination of legit instructions!
 */

void CMOS65xx::AHX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    // ?????
}

void CMOS65xx::ALR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    AND_internal(addressingMode, operandAddr, operand);
    addressingMode = ADDRESSING_MODE_ACCUMULATOR;
    operand = _regA;
    LSR_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::ANC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    AND_internal(addressingMode, operandAddr, operand);
    SET_FLAG_CARRY(_regA & 0x80);
}

void CMOS65xx::ARR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    AND_internal(addressingMode, operandAddr, operand);
    addressingMode = ADDRESSING_MODE_ACCUMULATOR;
    operand = _regA;
    ROR_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::ASO(int opcodeByte, int addressingMode, int *cycles, int *size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ASL_internal(addressingMode, operandAddr, operand);
    ORA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::AXS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    STX_internal(addressingMode, operandAddr, operand);
    PHA_internal(addressingMode, operandAddr, operand);
    AND_internal(addressingMode, operandAddr, operand);
    STA_internal(addressingMode, operandAddr, operand);
    PLA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::DCP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    DEC_internal(addressingMode, operandAddr, operand);
    CMP_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::ISC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    INC_internal(addressingMode, operandAddr, operand);
    SBC_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::LAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t res = (operand & _regS) & 0xff;
    _regA = res;
    _regX = res;
    _regS = res;
    SET_FLAG_ZERO(res == 0)
    SET_FLAG_NEGATIVE(res & 0x80)
}

void CMOS65xx::LAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    LDA_internal(addressingMode, operandAddr, operand);
    LDX_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::LSE(int opcodeByte, int addressingMode, int *cycles, int *size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    LSR_internal(addressingMode, operandAddr, operand);
    EOR_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::OAL(int opcodeByte, int addressingMode, int *cycles, int *size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ORA_internal(addressingMode, operandAddr, 0xee);
    AND_internal(addressingMode, operandAddr, operand);
    TAX_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::RLA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ROL_internal(addressingMode, operandAddr, operand);
    AND_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::RRA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    ROR_internal(addressingMode, operandAddr, operand);
    ADC_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::SAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t tmp = _regA;
    TXA_internal(addressingMode, operandAddr, operand);
    AND_internal(addressingMode, operandAddr, tmp);
    SEC_internal(addressingMode, operandAddr, operand);
    SBC_internal(addressingMode, operandAddr, operand);
    TAX_internal(addressingMode, operandAddr, operand);
    LDA_internal(addressingMode, operandAddr, tmp);
}

void CMOS65xx::SAY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t ab = ((operand & 0xff00) >> 8) + 1;
    PHA_internal(addressingMode, operandAddr, operand);
    TYA_internal(addressingMode, operandAddr, operand);
    AND_internal(addressingMode, operandAddr, ab);
    STA_internal(addressingMode, operandAddr, operand);
    PLA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::SKB(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // a nop (skip byte)
}

void CMOS65xx::SKW(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // a nop which skips 2 bytes (skip word)
}

void CMOS65xx::TAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t ab = ((operand & 0xff00) >> 8) + 1;
    uint8_t tmp = _regX;
    PHA_internal(addressingMode, operandAddr, operand);
    AND_internal(addressingMode, operandAddr, tmp);
    TAX_internal(addressingMode, operandAddr, tmp);
    TXS_internal(addressingMode, operandAddr, tmp);
    AND_internal(addressingMode, operandAddr, ab);
    STA_internal(addressingMode, operandAddr, operand);
    PLA_internal(addressingMode, operandAddr, operand);
    LDX_internal(addressingMode, operandAddr, tmp);
}

void CMOS65xx::XAA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    TXA_internal(addressingMode, operandAddr, operand);
    AND_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::XAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    uint8_t ab = ((operand & 0xff00) >> 8) + 1;
    PHA_internal(addressingMode, operandAddr, operand);
    TXA_internal(addressingMode, operandAddr, operand);
    AND_internal(addressingMode, operandAddr, ab);
    STA_internal(addressingMode, operandAddr, operand);
    PLA_internal(addressingMode, operandAddr, operand);
}

void CMOS65xx::KIL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    // parse the instruction
    uint16_t operand;
    uint16_t operandAddr;
    parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operandAddr, &operand, size, cycles);

    // exec
    CLog::fatal("invalid opcode!");
}

/**
 * called by both nmi() and irq(), prepare to run an interrupt request
 */
void CMOS65xx::irqInternal() {
    // push pc and status on the stack
    pushWord(_regPC);
    pushByte(_regP);

    // initiate irq request
    SET_FLAG_IRQ_DISABLE(true);
}

void CMOS65xx::irq() {
    if (!(IS_FLAG_IRQ_DISABLE)) {
        irqInternal();

        // set pc to the irq vector address
        _regPC = VECTOR_IRQ;
    }
}

void CMOS65xx::nmi() {
    // same as irq, but forced
    irqInternal();

    // set pc to the irq vector address
    _regPC = VECTOR_NMI;
}

/**
 * load the Klaus 6502 functional test
 */
void CMOS65xx::dbgLoadFunctionalTest() {
   char path[]="./tests/6502_functional_test.bin";
    // load test
    uint8_t* buf;
    uint32_t size;
    int res = CBuffer::fromFile(path,&buf,&size);
    if (res == 0) {
        CLog::print("Loaded Klaus 6502 functional test!");
        // copy to emulated memory at address 0
        _memory->writeBytes(0, buf, size);

        // set PC to 0x400
        _regPC = 0x400;
        free(buf);
    }
}

IMemory *CMOS65xx::memory() {
    return _memory;
}

