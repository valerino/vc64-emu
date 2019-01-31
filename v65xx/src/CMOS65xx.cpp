//
// Created by valerino on 06/01/19.
//

#include <CMOS65xx.h>
#include <CLog.h>
#include <CBuffer.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

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
#define SET_FLAG_UNUSED(_x_) _regP |= P_FLAG_UNUSED;

// the stack base address
#define STACK_BASE 0x100

// vectors
#define VECTOR_NMI      0xfffa
#define VECTOR_RESET    0xfffc
#define VECTOR_IRQ      0xfffe

// for the builtin debugger
static bool ignoreDebugging = false;
static bool isDebugging = false;
static bool forceBreak = false;
static uint32_t bpAddress = 0;
static int64_t bpCycles = 0;
static bool bpSet = false;
static uint64_t currentTotalCycles = 0;
static bool silenceLog = true;
static bool breakIrq = false;
static bool breakNmi = false;
static bool breakIrqOccured = false;
static bool breakNmiOccurred = false;

/**
 * full cpu status to string
 * @param addressingMode one of the addressing modes
 * @param status status buffer to fill
 * @param size status buffer size
 */
void CMOS65xx::cpuStatusToString(int addressingMode, char *status, int size) {
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
    if (_regP & P_FLAG_UNUSED) {
        strcat(s,"1");
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
    snprintf(status, size, "AddrMode=%s,\tPC=$%04x, A=$%02x, X=$%02x, Y=$%02x, S=$%02x, P=$%02x(%s), cycles=%" PRIu64, mode, _regPC, _regA, _regX, _regY, _regS, _regP, s,
            currentTotalCycles);
}

void CMOS65xx::logStateAfterInstruction(int addressingMode) {
#ifdef DEBUG_LOG_VERBOSE
    char status[128];
    cpuStatusToString(addressingMode, status, sizeof(status));
    CLog::printRaw("\t\tAfter: ------->\t%s\n", status);
#endif
}

/**
 * only for debugging, print execution
 * @param opcodeName name of the opcode
 * @param opcodeByte the opcode
 * @param operand the operand if any
 * @param addressingMode one of the addressing modes
 */
void CMOS65xx::logExecution(const char *name, uint8_t opcodeByte, uint16_t operand, int addressingMode) {
#ifdef DEBUG_LOG_EXECUTION
    bool doPrint = true;
#else
    bool doPrint = false;
#endif
    if (isDebugging) {
        // always show log on debugging
        doPrint = true;
    }
    else {
        // honor DEBUG_LOG_EXECUTION
        silenceLog = false;
    }

    if (doPrint && !silenceLog) {
        // fill status
        char statusString[128];
        cpuStatusToString(addressingMode, statusString, sizeof(statusString));

        if (addressingMode == ADDRESSING_MODE_IMPLIED) {
            // no operand
            CLog::printRaw("\t$%04x: %02X\t\t%s\t\t\t%s\n",
                           _regPC, opcodeByte, name, statusString);
        } else if (addressingMode == ADDRESSING_MODE_ACCUMULATOR) {
            // operand is A
            CLog::printRaw("\t$%04x: %02X\t\t%s A\t\t\t%s\n",
                           _regPC, opcodeByte, name, statusString);
        } else if (addressingMode == ADDRESSING_MODE_ABSOLUTE) {
            // absolute, 2 bytes operand
            uint8_t opByte1 = operand >> 8;
            uint8_t opByte2 = operand & 0xff;
            CLog::printRaw("\t$%04x: %02X %02X %02X\t\t%s $%04x\t\t%s\n",
                           _regPC, opcodeByte, opByte2, opByte1, name, operand, statusString);
        } else if (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_X ||
                   addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_X) {
            // 2 bytes operand, X/Y
            uint8_t opByte1 = operand >> 8;
            uint8_t opByte2 = operand & 0xff;
            CLog::printRaw("\t$%04x: %02X %02X %02X\t\t%s $%04x, X\t\t%s\n",
                           _regPC, opcodeByte, opByte2, opByte1, name, operand, statusString);
        } else if (addressingMode == ADDRESSING_MODE_ABSOLUTE_INDEXED_Y ||
                   addressingMode == ADDRESSING_MODE_ZEROPAGE_INDEXED_Y) {
            // 2 bytes operand, X/Y
            uint8_t opByte1 = operand >> 8;
            uint8_t opByte2 = operand & 0xff;
            CLog::printRaw("\t$%04x: %02X %02X %02X\t\t%s $%04x, Y\t\t%s\n",
                           _regPC, opcodeByte, opByte2, opByte1, name, operand, statusString);
        } else if (addressingMode == ADDRESSING_MODE_INDIRECT) {
            // (2 bytes operand)
            uint8_t opByte1 = operand >> 8;
            uint8_t opByte2 = operand & 0xff;
            CLog::printRaw("\t$%04x: %02X %02X %02X\t\t%s ($%04x)\t\t%s\n",
                           _regPC, opcodeByte, opByte2, opByte1, name, operand, statusString);
        } else if (addressingMode == ADDRESSING_MODE_INDIRECT_INDEXED_X) {
            // (operand, X)
            CLog::printRaw("\t$%04x: %02X %02X\t\t%s ($%02x, X)\t\t%s\n",
                           _regPC, opcodeByte, operand, name, operand, statusString);
        } else if (addressingMode == ADDRESSING_MODE_INDIRECT_INDEXED_Y) {
            // (operand), Y
            CLog::printRaw("\t$%04x: %02X %02X\t\t%s ($%02x), Y\t\t%s\n",
                           _regPC, opcodeByte, operand, name, operand, statusString);
        } else if (addressingMode == ADDRESSING_MODE_RELATIVE) {
            // 1 byte operand, relative to PC
            CLog::printRaw("\t$%04x: %02X %04X\t\t%s $%04x\t\t%s\n",
                           _regPC, opcodeByte, operand, name, operand, statusString);
        } else {
            // immediate / zeropage (1 byte operand)
            CLog::printRaw("\t$%04x: %02X %02X\t\t%s $%02x\t\t\t%s\n",
                           _regPC, opcodeByte, operand, name, operand, statusString);
        }
    }
}

/**
 * parse instruction depending on the addressing mode
 * @param opcodeByte the first byte (opcode)
 * @param functionName __FUNCTION__
 * @param addressingMode one of the addressing modes
 * @param operand on return, the operand if any
 * @param cycles on input, instruction cycles. on output, instruction cycles fixed depending on page crossing and instruction
 * @param on return, the instruction size
 * @return 0, or 1 if we run with debugging (do not execute instruction unless said)
 */
int CMOS65xx::parseInstruction(uint8_t opcodeByte, const char* functionName, int addressingMode, uint16_t* operand, int* size, int* cycles) {
    *operand = 0;
    *size = 0;

    // operand starts at the next byte
    uint16_t operandAddr = _regPC + 1;

    switch (addressingMode) {
        case ADDRESSING_MODE_ACCUMULATOR: {
            logExecution(functionName, opcodeByte, _regA, addressingMode);

            // will be read by the caller via readOperand()
            *operand = 0;
            *size = 1;
            break;
        }

        case ADDRESSING_MODE_ABSOLUTE: {
            uint16_t wd;
            _memory->readWord(operandAddr, &wd);
            logExecution(functionName, opcodeByte, wd, addressingMode);
            *operand = wd;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_ABSOLUTE_INDEXED_X: {
            uint16_t wd;
            _memory->readWord(operandAddr, &wd);
            logExecution(functionName, opcodeByte, wd, addressingMode);
            wd += _regX;

            // cycles may need adjustment
            handlePageCrossing(addressingMode, wd, cycles);
            *operand = wd;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_ABSOLUTE_INDEXED_Y: {
            uint16_t wd;
            _memory->readWord(operandAddr, &wd);
            logExecution(functionName, opcodeByte, wd, addressingMode);
            wd += _regY;

            // cycles may need adjustment
            handlePageCrossing(addressingMode, wd, cycles);
            *operand = wd;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_IMMEDIATE: {
            uint8_t bt;
            _memory->readByte(operandAddr, &bt);
            logExecution(functionName, opcodeByte, bt, addressingMode);
            *operand = operandAddr;
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
            uint16_t wd;
            _memory->readWord(operandAddr, &wd);
            logExecution(functionName, opcodeByte, wd, addressingMode);

            // emulate 6502 access bug on page boundary: if operand falls on page boundary, msb is not fetched correctly
            uint16_t addr = (wd & 0xff00) | (wd & 0x00ff);

            // read the effective address
            _memory->readWord(addr, &wd);
            *operand = wd;
            *size = 3;
            break;
        }

        case ADDRESSING_MODE_INDIRECT_INDEXED_X: {
            // 2nd byte of the instruction is an address in the zeropage
            uint8_t bt;
            _memory->readByte(operandAddr, &bt);
            bt += _regX;

            // read the effective address from zeropage
            uint16_t wd;
            _memory->readWord(bt, &wd);
            logExecution(functionName, opcodeByte, bt, addressingMode);
            *operand = wd;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_INDIRECT_INDEXED_Y: {
            // 2nd byte of the instruction is an address in the zeropage
            uint8_t bt;
            _memory->readByte(operandAddr, &bt);


            // read lsb, add Y, save the carry
            uint8_t lsb;
            _memory->readByte(bt, &lsb);
            uint16_t sum = lsb + _regY;
            lsb = sum & 0xff;

            // read msb and add carry if any
            uint8_t msb;
            _memory->readByte(bt + 1, &msb);
            msb += (sum > 0xff);

            // build the effective address
            uint16_t wd = (msb << 8) | lsb;
            logExecution(functionName, opcodeByte, bt, addressingMode);
            *operand = wd;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_RELATIVE:  {
            // address is relative to PC
            uint8_t bt;
            _memory->readByte(operandAddr, &bt);

            uint16_t wd = bt;
            if (wd & 0x80) {
                // sign extend
                wd |= 0xff00;
            }
            wd += _regPC + 2;
            logExecution(functionName, opcodeByte, wd, addressingMode);

            *operand = wd;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_ZEROPAGE: {
            uint8_t bt;
            _memory->readByte(operandAddr, &bt);
            logExecution(functionName, opcodeByte, bt, addressingMode);

            *operand = bt;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_ZEROPAGE_INDEXED_X: {
            uint8_t bt;
            _memory->readByte(operandAddr, &bt);
            logExecution(functionName, opcodeByte, bt, addressingMode);

            // effective address is also in zeropage
            bt += _regX;
            *operand = bt;
            *size = 2;
            break;
        }

        case ADDRESSING_MODE_ZEROPAGE_INDEXED_Y: {
            uint8_t bt;
            _memory->readByte(operandAddr, &bt);
            logExecution(functionName, opcodeByte, bt, addressingMode);

            // effective address is also in zeropage
            bt += _regY;
            *operand = bt;
            *size = 2;
            break;
        }

        default:
            // bug!
            logExecution(functionName, opcodeByte, *operand, addressingMode);
            CLog::fatal("invalid addressing mode: %d", addressingMode);
            return 0;
    }

    // add to total cycles
    currentTotalCycles += *cycles;

    // run the debugger if requested
    debugger(addressingMode,size);
    if (*size == 0) {
        // running under debugger
        return 1;
    }
    return 0;
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

    uint16_t newPc = _regPC + operand;
    if ((newPc & 0xff00) != (_regPC & 0xff00)) {
        // page transition, add another cycle
        *cycles+=1;

        // add here to total cycles, since this is not called by the parseInstruction() code
        currentTotalCycles += 1;
    }
}

/**
 * to be called post executing certain instructions, write the result back to memory or accumulator
 * @param addressingMode one of the addressing modes
 * @param addr the address to write to
 * @param bt the byte to write
 */
void CMOS65xx::writeOperand(int addressingMode, uint16_t addr, uint8_t bt) {
    if (addressingMode == ADDRESSING_MODE_ACCUMULATOR) {
        _regA = bt;
#ifdef DEBUG_LOG_VERBOSE
        CLog::printRaw("\t\tWrite: ------->\tA=$%02x\n", bt);
#endif
    }
    else {
#ifdef DEBUG_LOG_VERBOSE
        uint8_t m;
        _memory->readByte(addr,&m);
        CLog::printRaw("\t\tWrite(PRE): -->\t($%04x)=$%02x\n", addr, m);
#endif
        if (_callbackW != nullptr) {
            if (!_callbackW(addr, bt)) {
                // write not handled by callback
                _memory->writeByte(addr, bt);
            }
            else {
                // write handled by callback
            }
        }
        else {
            // no callback, write
            _memory->writeByte(addr, bt);
        }
#ifdef DEBUG_LOG_VERBOSE
        CLog::printRaw("\t\tWrite(POST): ->\t($%04x)=$%02x\n", addr, bt);
#endif
    }
}

/**
 * to be called in the beginning of ceratain instructions, read the operand from memory
 * @param addressingMode one of the addressing modes
 * @param addr the address to write to
 * @param bt on return, the operand
 */
void CMOS65xx::readOperand(int addressingMode, uint16_t addr, uint8_t* bt) {
    if (addressingMode == ADDRESSING_MODE_ACCUMULATOR) {
        *bt = _regA;
#ifdef DEBUG_LOG_VERBOSE
        CLog::printRaw("\t\tRead:  ------->\tA=$%02x\n", _regA);
#endif
    }
    else {
        _memory->readByte(addr, bt);
#ifdef DEBUG_LOG_VERBOSE
        CLog::printRaw("\t\tRead: -------->\t($%04x)=$%02x\n", addr, *bt);
#endif
        if (_callbackR != nullptr) {
            // client may change the address and value being read
            _callbackR(addr, bt);
        }
    }
}

int CMOS65xx::step(bool debugging, bool forceDebugging) {
    isDebugging = debugging;
    forceBreak = forceDebugging;

    // get opcode byte
    uint8_t bt;
    _memory->readByte(_regPC,&bt);

    // get opcode from table
    Opcode op = _opcodeMatrix[bt];

    // execute instruction and return the number of occupied cycles
    int occupiedCycles = op.cycles;
    int instructionSize = 0;
    (this->*op.ptr)(bt, op.mode, &occupiedCycles, &instructionSize);
    logStateAfterInstruction(op.mode);

    // next opcode
    _regPC += instructionSize;

#ifdef DEBUG_RUN_FUNCTIONAL_TESTS
    // i.e. detect deadlock in functional test
    if (_regPC != _prevPc) {
        _prevPc = _regPC;
    }
    else {
        // break!
        CLog::fatal("DEADLOCK!");
    }
    if (_regPC == 0x3469) {
        // success and exit!
        CLog::print("!! Klaus 6502 functional test SUCCESS !!");
        return -1;
    }
#endif
    return occupiedCycles;
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
    _regP |= P_FLAG_UNUSED;
    _memory->readWord(VECTOR_RESET,&_regPC);
#ifdef DEBUG_RUN_FUNCTIONAL_TESTS
    // overwrite memory with functional test suite
    dbgLoadFunctionalTest();
#endif
    return 0;
}

CMOS65xx::CMOS65xx(IMemory *mem, CpuCallbackRead callbackRead, CpuCallbackWrite callbackWrite) {
    _memory = mem;
    _callbackR = callbackRead;
    _callbackW = callbackWrite;
}

/**
 * push a word (16 bit) on the stack
 * @param wd the word to be pushed
 */
void CMOS65xx::pushWord(uint16_t wd) {
    // value must be pushed LE
    uint8_t bt = (uint8_t)(wd >> 8);
    pushByte(bt);

    bt = (uint8_t)(wd & 0xff);
    pushByte(bt);
}

/**
 * push a byte on the stack
 * @param bt the byte to be pushed
 */
void CMOS65xx::pushByte(uint8_t bt) {
    _memory->writeByte(STACK_BASE + _regS, bt);

    // decrement stack, beware of overrun (stack memory is $100-$1ff)
    if (_regS == 0) {
        _regS = 0xff;
    }
    else {
        _regS -= 1;
    }
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
    // increment stack, beware of overrun (stack memory is $100-$1ff)
    if (_regS == 0xff) {
        _regS = 0;
    }
    else {
        _regS += 1;
    }
    uint8_t bt;
    _memory->readByte(STACK_BASE + _regS, &bt);
    return bt;
}

void CMOS65xx::ADC_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    uint32_t wd = _regA + bt + (IS_FLAG_CARRY);
    if (IS_FLAG_DECIMAL_MODE) {
        // handle decimal mode
        if ( ((_regA & 0xf) + (bt & 0xF) + (IS_FLAG_CARRY)) > 9 ) {
            wd += 6;
        }
        SET_FLAG_NEGATIVE(wd & 0x80);

        int overflow = ( !((_regA ^ bt) & 0x80) && ((_regA ^ wd) & 0x80) );
        SET_FLAG_OVERFLOW(overflow > 0)
        if (wd > 0x99) {
            wd += 0x60;
        }
        SET_FLAG_CARRY(wd > 0x99);
    }
    else {
        // binary
        SET_FLAG_CARRY(wd > 0xff)
        SET_FLAG_ZERO((wd & 0xff) == 0)
        SET_FLAG_NEGATIVE(wd & 0x80)
        int overflow = ( !((_regA ^ bt) & 0x80) && ((_regA ^ wd) & 0x80) );
        SET_FLAG_OVERFLOW(overflow > 0)
    }

    // NOTE: an additional cycle is added only in 65c02, not in vanilla 6502 !!!
    _regA = wd & 0xff;
}

void CMOS65xx::ADC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        ADC_internal(addressingMode, operand);
    }
}

void CMOS65xx::AND_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    _regA &= bt;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::AND(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        AND_internal(addressingMode, operand);
    }
}

void CMOS65xx::ASL_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    SET_FLAG_CARRY(bt & 0x80)
    bt <<= 1;
    SET_FLAG_ZERO(bt == 0)
    SET_FLAG_NEGATIVE(bt & 0x80)
    writeOperand(addressingMode, operand, bt);
}

void CMOS65xx::ASL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        ASL_internal(addressingMode, operand);
    }
}

void CMOS65xx::BCC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        if (! (IS_FLAG_CARRY)) {
            handlePageCrossingOnBranch(operand, cycles);
            _regPC = operand;
            *size = 0;
        }
    }
}

void CMOS65xx::BCS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        if (IS_FLAG_CARRY) {
            handlePageCrossingOnBranch(operand, cycles);
            _regPC = operand;
            *size = 0;
        }
    }
}

void CMOS65xx::BEQ(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        if (IS_FLAG_ZERO) {
            handlePageCrossingOnBranch(operand, cycles);
            _regPC = operand;
            *size = 0;
        }
    }
}

void CMOS65xx::BIT(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t bt;
        readOperand(addressingMode, operand, &bt);
        uint8_t andRes = _regA & bt;

        SET_FLAG_ZERO (andRes == 0);
        SET_FLAG_NEGATIVE(bt & 0x80)
        SET_FLAG_OVERFLOW(bt & 0x40)
    }
}

void CMOS65xx::BMI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        if (IS_FLAG_NEGATIVE) {
            handlePageCrossingOnBranch(operand, cycles);
            _regPC = operand;
            *size = 0;
        }
    }
}

void CMOS65xx::BNE(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        if (! (IS_FLAG_ZERO)) {
            handlePageCrossingOnBranch(operand, cycles);
            _regPC = operand;
            *size = 0;
        }
    }
}

void CMOS65xx::BPL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        if (! (IS_FLAG_NEGATIVE)) {
            handlePageCrossingOnBranch(operand, cycles);
            _regPC = operand;
            *size = 0;
        }
    }
}

void CMOS65xx::BRK(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        // push PC and P on stack
        pushWord(_regPC + *size + 1);

        // pushed status always have BRK set
        uint8_t bt = _regP | P_FLAG_BRK_COMMAND;
        pushByte(bt);

        // set the irq disable flag
        SET_FLAG_IRQ_DISABLE(true);

        // and set PC to step the IRQ service routine located at the IRQ vector
        _memory->readWord(VECTOR_IRQ, &_regPC);
        *size = 0;
    }
}

void CMOS65xx::BVC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        if (! (IS_FLAG_OVERFLOW)) {
            handlePageCrossingOnBranch(operand, cycles);
            _regPC = operand;
            *size = 0;
        }
    }
}

void CMOS65xx::BVS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        if (IS_FLAG_OVERFLOW) {
            handlePageCrossingOnBranch(operand, cycles);
            _regPC = operand;
            *size = 0;
        }
    }
}

void CMOS65xx::CLC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        SET_FLAG_CARRY(false)
    }
}

void CMOS65xx::CLD(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        SET_FLAG_DECIMAL_MODE(false)
    }
}

void CMOS65xx::CLI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        SET_FLAG_IRQ_DISABLE(false)
    }
}

void CMOS65xx::CLV(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        SET_FLAG_OVERFLOW(false)
    }
}

void CMOS65xx::CMP_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);
    uint16_t wd = _regA - bt;
    SET_FLAG_CARRY(wd <= 0xff)
    SET_FLAG_ZERO((wd & 0xff) == 0)
    SET_FLAG_NEGATIVE(wd & 0x80)
}

void CMOS65xx::CMP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        CMP_internal(addressingMode, operand);
    }
}

void CMOS65xx::CPX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t bt;
        readOperand(addressingMode, operand, &bt);

        uint16_t wd = _regX - bt;
        SET_FLAG_CARRY(wd <= 0xff);
        SET_FLAG_ZERO((wd & 0xff) == 0);
        SET_FLAG_NEGATIVE(wd & 0x80);
    }
}

void CMOS65xx::CPY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t bt;
        readOperand(addressingMode, operand, &bt);

        uint16_t wd = _regY - bt;
        SET_FLAG_CARRY(wd <= 0xff);
        SET_FLAG_ZERO((wd & 0xff) == 0);
        SET_FLAG_NEGATIVE(wd & 0x80);
    }
}

void CMOS65xx::DEC_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    _memory->readByte(operand, &bt);

    bt -= 1;
    SET_FLAG_ZERO(bt == 0);
    SET_FLAG_NEGATIVE(bt & 0x80);
    writeOperand(addressingMode, operand, bt);
}

void CMOS65xx::DEC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        DEC_internal(addressingMode, operand);
    }
}

void CMOS65xx::DEX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regX -= 1;
        SET_FLAG_ZERO(_regX == 0);
        SET_FLAG_NEGATIVE(_regX & 0x80);
    }
}

void CMOS65xx::DEY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regY -= 1;
        SET_FLAG_ZERO(_regY == 0);
        SET_FLAG_NEGATIVE(_regY & 0x80);
    }
}

void CMOS65xx::EOR_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    _regA ^= bt;
    SET_FLAG_ZERO(_regA == 0);
    SET_FLAG_NEGATIVE(_regA & 0x80);
}

void CMOS65xx::EOR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        EOR_internal(addressingMode, operand);
    }
}

void CMOS65xx::INC_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    bt += 1;
    SET_FLAG_ZERO(bt == 0);
    SET_FLAG_NEGATIVE(bt & 0x80);
    writeOperand(addressingMode, operand, bt);
}

void CMOS65xx::INC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        INC_internal(addressingMode, operand);        
    }
}

void CMOS65xx::INX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regX +=1;
        SET_FLAG_ZERO(_regX == 0);
        SET_FLAG_NEGATIVE(_regX & 0x80);
    }
}

void CMOS65xx::INY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regY += 1;
        SET_FLAG_ZERO(_regY == 0);
        SET_FLAG_NEGATIVE(_regY & 0x80);
    }
}

void CMOS65xx::JMP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::JSR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        pushWord(_regPC + *size - 1);
        _regPC = operand;
        *size = 0;
    }
}

void CMOS65xx::LDA_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    _regA = bt;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::LDA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        LDA_internal(addressingMode, operand);
    }
}

void CMOS65xx::LDX_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    _regX = bt;
    SET_FLAG_ZERO(_regX == 0)
    SET_FLAG_NEGATIVE(_regX & 0x80)
}

void CMOS65xx::LDX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        LDX_internal(addressingMode, operand);
    }
}

void CMOS65xx::LDY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t bt;
        readOperand(addressingMode, operand, &bt);

        _regY = bt;
        SET_FLAG_ZERO(_regY == 0)
        SET_FLAG_NEGATIVE(_regY & 0x80)
    }
}

void CMOS65xx::LSR_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    SET_FLAG_CARRY(bt & 1)
    bt >>= 1;
    SET_FLAG_ZERO(bt == 0)
    SET_FLAG_NEGATIVE(bt & 0x80)
    writeOperand(addressingMode, operand, bt);
}

void CMOS65xx::LSR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        LSR_internal(addressingMode, operand);
    }
}

void CMOS65xx::NOP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        // nothing, nop!
    }
}

void CMOS65xx::ORA_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    _regA |= bt;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::ORA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        ORA_internal(addressingMode, operand);
    }
}

void CMOS65xx::PHA_internal(int addressingMode, uint16_t operand) {
    pushByte(_regA);
}

void CMOS65xx::PHA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        PHA_internal(addressingMode, operand);
    }
}

void CMOS65xx::PHP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t tmp = _regP;
        tmp |= (P_FLAG_UNUSED | P_FLAG_BRK_COMMAND);
        pushByte(tmp);
    }
}

void CMOS65xx::PLA_internal(int addressingMode, uint16_t operand) {
    _regA = popByte();
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::PLA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        PLA_internal(addressingMode, operand);
    }
}

void CMOS65xx::PLP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regP = popByte();

        // the unused flag must always be set !
        SET_FLAG_UNUSED(true)
    }
}

void CMOS65xx::ROL_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    uint16_t wd = (bt << 1) | (IS_FLAG_CARRY);
    SET_FLAG_CARRY(wd > 0xff)
    bt = wd & 0xff;
    SET_FLAG_ZERO(bt == 0)
    SET_FLAG_NEGATIVE(bt & 0x80)
    writeOperand(addressingMode, operand, bt);
}

void CMOS65xx::ROL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        // exec
        ROL_internal(addressingMode, operand);
    }
}

void CMOS65xx::ROR_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    uint16_t wd = bt | ((IS_FLAG_CARRY) << 8);
    SET_FLAG_CARRY(wd & 1)
    wd >>= 1;
    bt = wd & 0xff;
    SET_FLAG_ZERO(bt == 0)
    SET_FLAG_NEGATIVE(bt & 0x80)
    writeOperand(addressingMode, operand, bt);
}

void CMOS65xx::ROR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        ROR_internal(addressingMode, operand);
    }
}

void CMOS65xx::RTI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regP = popByte();
        _regPC = popWord();
        *size = 0;
    }
}

void CMOS65xx::RTS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regPC = popWord();
        _regPC += 1;
        *size = 0;
    }
}

void CMOS65xx::SBC_internal(int addressingMode, uint16_t operand) {
    uint8_t bt;
    readOperand(addressingMode, operand, &bt);

    uint32_t wd = _regA - bt - !(IS_FLAG_CARRY);
    SET_FLAG_ZERO((wd & 0xff) == 0)
    SET_FLAG_NEGATIVE(wd & 0x80)
    int overflow = ( ((_regA ^ bt) & 0x80) && ((_regA ^ wd) & 0x80) );
    SET_FLAG_OVERFLOW(overflow > 0)

    if (IS_FLAG_DECIMAL_MODE) {
        // handle decimal mode
        if ( ((_regA & 0xf) - !(IS_FLAG_CARRY)) < (bt & 0xf) ) {
            wd -= 6;
        }
        if (wd > 0x99) {
            wd -= 0x60;
        }
    }
    SET_FLAG_CARRY(wd <= 0xff)

    // NOTE: an additional cycle is added only in 65c02, not in vanilla 6502 !!!
    _regA = wd & 0xff;
}

void CMOS65xx::SBC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        SBC_internal(addressingMode, operand);
    }
}

void CMOS65xx::SEC_internal(int addressingMode, uint16_t operand) {
    SET_FLAG_CARRY(true)
}

void CMOS65xx::SEC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        SEC_internal(addressingMode, operand);
    }
}

void CMOS65xx::SED(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        SET_FLAG_DECIMAL_MODE(true)
    }
}

void CMOS65xx::SEI(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        SET_FLAG_IRQ_DISABLE(true)
    }
}

void CMOS65xx::STA_internal(int addressingMode, uint16_t operand) {
    writeOperand(addressingMode, operand, _regA);
}

void CMOS65xx::STA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        STA_internal(addressingMode, operand);
    }
}

void CMOS65xx::STX_internal(int addressingMode, uint16_t operand) {
    writeOperand(addressingMode, operand, _regX);
}

void CMOS65xx::STX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        STX_internal(addressingMode, operand);
    }
}

void CMOS65xx::STY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        writeOperand(addressingMode, operand, _regY);
    }
}

void CMOS65xx::TAX_internal(int addressingMode, uint16_t operand) {
    _regX = _regA;
    SET_FLAG_ZERO(_regX == 0)
    SET_FLAG_NEGATIVE(_regX & 0x80)
}

void CMOS65xx::TAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        TAX_internal(addressingMode, operand);
    }
}

void CMOS65xx::TAY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regY = _regA;
        SET_FLAG_ZERO(_regY == 0)
        SET_FLAG_NEGATIVE(_regY & 0x80)
    }
}

void CMOS65xx::TSX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        _regX = _regS;
        SET_FLAG_ZERO(_regX == 0)
        SET_FLAG_NEGATIVE(_regX & 0x80)
    }
}

void CMOS65xx::TXA_internal(int addressingMode, uint16_t operand) {
    _regA = _regX;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::TXA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        TXA_internal(addressingMode, operand);
    }
}

void CMOS65xx::TXS_internal(int addressingMode, uint16_t operand) {
    _regS = _regX;
}

void CMOS65xx::TXS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        TXS_internal(addressingMode, operand);
    }
}

void CMOS65xx::TYA_internal(int addressingMode, uint16_t operand) {
    _regA = _regY;
    SET_FLAG_ZERO(_regA == 0)
    SET_FLAG_NEGATIVE(_regA & 0x80)
}

void CMOS65xx::TYA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        TYA_internal(addressingMode, operand);
    }
}

/**
 * undocumented instructions: most of these instructions are implemented as combination of legit instructions!
 */

void CMOS65xx::AHX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        // TODO: implement!!!
        CLog::printRaw("\t!! NOT IMPLEMENTED !!\n");
    }
}

void CMOS65xx::ALR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        AND_internal(addressingMode, operand);
        LSR_internal(ADDRESSING_MODE_ACCUMULATOR, _regA);
    }
}

void CMOS65xx::ANC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        AND_internal(addressingMode, operand);
        SET_FLAG_CARRY(_regA & 0x80);
    }
}

void CMOS65xx::ARR(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        AND_internal(addressingMode, operand);
        addressingMode = ADDRESSING_MODE_ACCUMULATOR;
        ROR_internal(addressingMode, operand);
    }
}

void CMOS65xx::ASO(int opcodeByte, int addressingMode, int *cycles, int *size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        ASL_internal(addressingMode, operand);
        ORA_internal(addressingMode, operand);
    }
}

void CMOS65xx::AXS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        STX_internal(addressingMode, operand);
        PHA_internal(addressingMode, operand);
        AND_internal(addressingMode, operand);
        STA_internal(addressingMode, operand);
        PLA_internal(addressingMode, operand);
    }
}

void CMOS65xx::DCP(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        DEC_internal(addressingMode, operand);
        CMP_internal(addressingMode, operand);
    }
}

void CMOS65xx::ISC(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        INC_internal(addressingMode, operand);
        SBC_internal(addressingMode, operand);
    }
}

void CMOS65xx::LAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t bt;
        readOperand(addressingMode, operand, &bt);
        bt &= _regS;
        _regA = bt;
        _regX = bt;
        _regS = bt;
        SET_FLAG_ZERO(bt == 0)
        SET_FLAG_NEGATIVE(bt & 0x80)
    }
}

void CMOS65xx::LAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        LDA_internal(addressingMode, operand);
        LDX_internal(addressingMode, operand);
    }
}

void CMOS65xx::LSE(int opcodeByte, int addressingMode, int *cycles, int *size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        LSR_internal(addressingMode, operand);
        EOR_internal(addressingMode, operand);
    }
}

void CMOS65xx::OAL(int opcodeByte, int addressingMode, int *cycles, int *size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        ORA_internal(addressingMode, 0xee);
        AND_internal(addressingMode, operand);
        TAX_internal(addressingMode, operand);
    }
}

void CMOS65xx::RLA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        ROL_internal(addressingMode, operand);
        AND_internal(addressingMode, operand);
    }
}

void CMOS65xx::RRA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        ROR_internal(addressingMode, operand);
        ADC_internal(addressingMode, operand);
    }
}

void CMOS65xx::SAX(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        TXA_internal(addressingMode, operand);
        AND_internal(addressingMode, _regA);
        SEC_internal(addressingMode, operand);
        SBC_internal(addressingMode, operand);
        TAX_internal(addressingMode, operand);
        LDA_internal(addressingMode,  _regA);
    }
}

void CMOS65xx::SAY(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t bt = ((operand & 0xff00) >> 8) + 1;
        PHA_internal(addressingMode, operand);
        TYA_internal(addressingMode, operand);
        AND_internal(addressingMode, bt);
        STA_internal(addressingMode, operand);
        PLA_internal(addressingMode, operand);
    }
}

void CMOS65xx::SKB(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        // a nop (skip byte)
    }
}

void CMOS65xx::SKW(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        // a nop which skips 2 bytes (skip word)
    }
}

void CMOS65xx::TAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t bt = ((operand & 0xff00) >> 8) + 1;
        PHA_internal(addressingMode, operand);
        AND_internal(addressingMode, _regX);
        TAX_internal(addressingMode, _regX);
        TXS_internal(addressingMode, _regX);
        AND_internal(addressingMode, bt);
        STA_internal(addressingMode, operand);
        PLA_internal(addressingMode, operand);
        LDX_internal(addressingMode, _regX);
    }
}

void CMOS65xx::XAA(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        TXA_internal(addressingMode, operand);
        AND_internal(addressingMode, operand);
    }
}

void CMOS65xx::XAS(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
        uint8_t bt = ((operand & 0xff00) >> 8) + 1;
        PHA_internal(addressingMode, operand);
        TXA_internal(addressingMode, operand);
        AND_internal(addressingMode, bt);
        STA_internal(addressingMode, operand);
        PLA_internal(addressingMode, operand);
    }
}

void CMOS65xx::KIL(int opcodeByte, int addressingMode, int* cycles, int* size) {
    uint16_t operand;
    if (parseInstruction(opcodeByte, __FUNCTION__, addressingMode, &operand, size, cycles) == 0) {
    
    }
    CLog::fatal("invalid opcode!");
}

/**
 * called by both nmi() and irq(), prepare to run an interrupt request
 */
void CMOS65xx::irqInternal() {
    // push PC and P on stack
    pushWord(_regPC);

    // irq do not have the brk flag set!
    SET_FLAG_BRK_COMMAND(false)
    pushByte(_regP);

    // set the irq disable and break flag
    SET_FLAG_BRK_COMMAND(true)
    SET_FLAG_IRQ_DISABLE(true)

    // and set PC to step the IRQ service routine located at the IRQ vector
    _memory->readWord(VECTOR_IRQ, &_regPC);

}

void CMOS65xx::irq() {
    if (!(IS_FLAG_IRQ_DISABLE)) {
        irqInternal();
    }

    if (breakIrq) {
        // break on irq requested
        breakIrqOccured = true;
    }
}

void CMOS65xx::nmi() {
    // same as irq, but forced
    irqInternal();

    // set pc to the nmi vector address
    _memory->readWord(VECTOR_NMI, &_regPC);

    if (breakNmi) {
        // break on nmi requested
        breakNmi = true;
    }
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
        _isTest = true;
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

/**
 * poor's man debugger implementation, called by the instruction parser
 * @param addressingMode the addressing mode
 * @param size on return, *size=0 indicates the debugger triggered so the instruction parser may tell the outer loop
 * to not advance PC
 */
void CMOS65xx::debugger(int addressingMode, int* size) {
    // check if we need to break
    bool doBreak = false;
    if (forceBreak || isDebugging && !ignoreDebugging) {
        // requested break
        doBreak = true;
        if (forceBreak) {
            ignoreDebugging = false;
        }
    }
    if (bpSet) {
        // breakpoint is set
        if (bpCycles != 0 && currentTotalCycles >= bpCycles) {
            // cycles bp hit!
            doBreak = true;
        }
        if (_regPC == bpAddress) {
            // address bp hit!
            doBreak = true;
        }
        // check breakpoints on irq/nmi
        if (breakIrqOccured) {
            doBreak = true;
            CLog::printRaw("\tIRQ occurred!\n");
            breakIrqOccured = false;

        }
        if (breakNmiOccurred) {
            doBreak = true;
            CLog::printRaw("\tNMI occurred!\n");
            breakNmiOccurred = false;
        }
    }

    if (doBreak) {
        // poor's man debugger, take input from command line!
        char *line = nullptr;
        size_t lineSize = 0;
        printf("DBG > ");
        getline(&line, &lineSize, stdin);

        // inhibit standard debugprint of instruction parser by default
        silenceLog = true;

        // parse line
        switch (line[0]) {
            case 'p':
                // step
                ignoreDebugging = false;
                silenceLog = false;
                break;

            case 'r':
                // print registers
                silenceLog = true;
                char statusString[128];
                cpuStatusToString(addressingMode, statusString, sizeof(statusString));
                CLog::printRaw("\t%s\n", statusString);

                // do not advance
                *size = 0;
                break;

            case 'g':
                // go
                silenceLog = false;
                ignoreDebugging = true;
                break;

            case 'c':
                // clear breakpoints
                silenceLog = true;
                bpSet = false;
                bpAddress = 0;
                bpCycles = 0;
                breakIrq = false;
                breakNmi = false;
                ignoreDebugging = false;
                CLog::printRaw("\tbreakpoint clear!\n");

                // do not advance
                *size = 0;
                break;

            case 'b':
                // check breakpoint type, if on cycle or address
                if (line[1] == 'p') {
                    // on address
                    sscanf(line, "bp $%x", &bpAddress);
                    CLog::printRaw("\tset breakpoint at address $%x !\n", bpAddress);
                } else if (line[1] == 'c') {
                    // on cycles > n
                    sscanf(line, "bc %" PRIu64, &bpCycles);
                    CLog::printRaw("\tset breakpoint at cycles >= %lld !\n", bpCycles);
                } else if (line[1] == 'q') {
                    // on irq
                    CLog::printRaw("\tset breakpoint on IRQ !\n");
                    breakIrq = true;
                } else if (line[1] == 'n') {
                    // break on nmi
                    CLog::printRaw("\tset breakpoint on NMI !\n");
                    breakNmi = true;
                } else {
                    // unknown!
                    CLog::printRaw("\t ERROR, unknown command: %s\n", line);
                    break;
                }
                // do not advance
                *size = 0;
                bpSet = true;
                silenceLog = true;
                break;

            case 'd': {
                // dump address (d $1234 nbytes)
                silenceLog = true;
                uint32_t address;
                int numBytes;
                sscanf(line, "d $%x %d", &address, &numBytes);
                uint32_t memSize;
                uint8_t *mem = _memory->raw(&memSize);
                if (address + numBytes > memSize) {
                    CLog::printRaw("\t ERROR, invalid address/size (max address = $x!\n", memSize);
                    break;
                }
                CLog::printRaw("\tdumping %d bytes at address $%x (%d):\n", numBytes, address, address);
                mem += address;
                CBuffer::hexDump(mem, numBytes);

                // do not advance
                *size = 0;
                break;
            }

            case 'h': {
                CLog::printRaw("built-in 65xx debugger\n");
                CLog::printRaw("\tavailable commands:\n");
                CLog::printRaw("\tp:\t\t\t\tstep instruction\n");
                CLog::printRaw("\tr:\t\t\t\tdisplay registers\n");
                CLog::printRaw("\td <$address> <num_bytes>:\tdisplay num_bytes at address\n");
                CLog::printRaw("\tbp <$address>:\t\t\tbreak at address (overwrite existent)\n");
                CLog::printRaw("\tbc <num_cycles>:\t\tbreak when the cpu reaches (or exceeds) the given cyclecount\n");
                CLog::printRaw("\tbq:\t\t\t\tbreakpoint on IRQ\n");
                CLog::printRaw("\tbn:\t\t\t\tbreakpoint on NMI\n");
                CLog::printRaw("\tc:\t\t\t\tclear all breakpoints\n");
                break;
            }
            default:
                // unknown!
                silenceLog = true;
                CLog::printRaw("\t ERROR, unknown command: %s\n", line);

                // do not advance
                *size = 0;
                break;
        }
        free(line);
    }
}

bool CMOS65xx::isTestMode() {
    return _isTest;
}
