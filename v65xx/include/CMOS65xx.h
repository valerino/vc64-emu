//
// Created by valerino on 06/01/19.
//

#ifndef CMOS65XX_H
#define CMOS65XX_H

/**
 * implement MOS 6502/6510 CPU emulator
 */
#include <IMemory.h>

// addressing modes
#define ADDRESSING_MODE_INVALID                 0
#define ADDRESSING_MODE_ACCUMULATOR             1
#define ADDRESSING_MODE_ABSOLUTE                2
#define ADDRESSING_MODE_ABSOLUTE_INDEXED_X      3
#define ADDRESSING_MODE_ABSOLUTE_INDEXED_Y      4
#define ADDRESSING_MODE_IMMEDIATE               5
#define ADDRESSING_MODE_IMPLIED                 6
#define ADDRESSING_MODE_INDIRECT                7
#define ADDRESSING_MODE_INDIRECT_INDEXED_X      8
#define ADDRESSING_MODE_INDIRECT_INDEXED_Y      9
#define ADDRESSING_MODE_RELATIVE                10
#define ADDRESSING_MODE_ZEROPAGE                11
#define ADDRESSING_MODE_ZEROPAGE_INDEXED_X      12
#define ADDRESSING_MODE_ZEROPAGE_INDEXED_Y      13

// callback for clients
#define CPU_MEM_READ 0
#define CPU_MEM_WRITE 1
typedef void (*CpuCallback)(int type, uint16_t address, uint8_t val);

#ifndef NDEBUG
// debug-only flag, disable to toggle debug log
#define DEBUG_LOG_EXECUTION
// debug-only flag, log also read/writes and status after each instruction
//#define DEBUG_LOG_VERBOSE
// debug-only flag, to step functional tests
//#define DEBUG_RUN_FUNCTIONAL_TESTS
#endif

class CMOS65xx {
private:
    // the opcode matrix
    // each opcode handler takes the addressing mode and number of cycles for that addressing mode in input,
    // and returns the number of cycles effectively occupied by the instruction (which may be different than the input
    // cycles) and the instruction size
    typedef void (CMOS65xx::*OpcodePtr)(int opcodeByte, int addressingMode, int* cycles, int* size);
    typedef struct _Opcode {
        // cpu cycles
        int cycles;
        // function handler
        OpcodePtr ptr;
        // addressing mode
        int mode;
    } Opcode;

    Opcode _opcodeMatrix[0x100] {
            // 0x0-0x0xf
            { 7, &CMOS65xx::BRK, ADDRESSING_MODE_IMPLIED},
            { 6, &CMOS65xx::ORA, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::ASO, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::ORA, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::ASL, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::ASO, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::PHP, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::ORA, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::ASL, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::ANC, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::SKW, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::ORA, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::ASL, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::ASO, ADDRESSING_MODE_ABSOLUTE},

            // 0x10-0x1f
            { 2, &CMOS65xx::BPL, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::ORA, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::ASO, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::ORA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ASL, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ASO, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::CLC, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::ORA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::ASO, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::SKW, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::ORA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ASL, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ASO, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0x20-0x2f
            { 6, &CMOS65xx::JSR, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::AND, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::RLA, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::BIT, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::AND, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::ROL, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::RLA, ADDRESSING_MODE_ZEROPAGE},
            { 4, &CMOS65xx::PLP, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::AND, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::ROL, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::ANC, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::BIT, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::AND, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::ROL, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::RLA, ADDRESSING_MODE_ABSOLUTE},

            // 0x30-0x3f
            { 2, &CMOS65xx::BMI, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::AND, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::RLA, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::AND, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ROL, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::RLA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::SEC, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::AND, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::RLA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::SKW, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::AND, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ROL, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::RLA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0x40-0x4f
            { 6, &CMOS65xx::RTI, ADDRESSING_MODE_IMPLIED},
            { 6, &CMOS65xx::EOR, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::LSE, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::EOR, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::LSR, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::LSE, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::PHA, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::EOR, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::LSR, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::ALR, ADDRESSING_MODE_IMMEDIATE},
            { 3, &CMOS65xx::JMP, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::EOR, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::LSR, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::LSE, ADDRESSING_MODE_ABSOLUTE},

            // 0x50-0x5f
            { 2, &CMOS65xx::BVC, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::EOR, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::LSE, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::EOR, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::LSR, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::LSE, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::CLI, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::EOR, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::LSE, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::SKW, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::EOR, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::LSR, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::LSE, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0x60-0x6f
            { 6, &CMOS65xx::RTS, ADDRESSING_MODE_IMPLIED},
            { 6, &CMOS65xx::ADC, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::RRA, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::ADC, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::ROR, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::RRA, ADDRESSING_MODE_ZEROPAGE},
            { 4, &CMOS65xx::PLA, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::ADC, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::ROR, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::ARR, ADDRESSING_MODE_IMMEDIATE},
            { 5, &CMOS65xx::JMP, ADDRESSING_MODE_INDIRECT},
            { 4, &CMOS65xx::ADC, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::ROR, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::RRA, ADDRESSING_MODE_ABSOLUTE},

            // 0x70-0x7f
            { 2, &CMOS65xx::BVS, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::ADC, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::RRA, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::ADC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ROR, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::RRA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::SEI, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::ADC, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::RRA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::SKW, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::ADC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ROR, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::RRA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0x80-0x8f
            { 2, &CMOS65xx::SKB, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::STA, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 2, &CMOS65xx::SKB, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::AXS, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::STY, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::STA, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::STX, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::AXS, ADDRESSING_MODE_ZEROPAGE},
            { 2, &CMOS65xx::DEY, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::TXA, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::XAA, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::STY, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::STA, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::STX, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::AXS, ADDRESSING_MODE_ABSOLUTE},

            // 0x90-0x9f
            { 2, &CMOS65xx::BCC, ADDRESSING_MODE_RELATIVE},
            { 6, &CMOS65xx::STA, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 6, &CMOS65xx::AHX, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::STY, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::STA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::STX, ADDRESSING_MODE_ZEROPAGE_INDEXED_Y},
            { 4, &CMOS65xx::AXS, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::TYA, ADDRESSING_MODE_IMPLIED},
            { 5, &CMOS65xx::STA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::TXS, ADDRESSING_MODE_IMPLIED},
            { 5, &CMOS65xx::TAS, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 5, &CMOS65xx::SAY, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 5, &CMOS65xx::STA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 5, &CMOS65xx::XAS, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 5, &CMOS65xx::AHX, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},

            // 0Xa0-0xaf
            { 2, &CMOS65xx::LDY, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::LDA, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 2, &CMOS65xx::LDX, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::LAX, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::LDY, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::LDA, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::LDX, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::LAX, ADDRESSING_MODE_ZEROPAGE},
            { 2, &CMOS65xx::TAY, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::LDA, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::TAX, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::OAL, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::LDY, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::LDA, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::LDX, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::LAX, ADDRESSING_MODE_ABSOLUTE},

            // 0xb0-0xbf
            { 2, &CMOS65xx::BCS, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::LDA, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 5, &CMOS65xx::LAX, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::LDY, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::LDA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::LDX, ADDRESSING_MODE_ZEROPAGE_INDEXED_Y},
            { 4, &CMOS65xx::LAX, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::CLV, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::LDA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::TSX, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::LAS, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::LDY, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::LDA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::LDX, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::LAX, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},

            // 0xc0-0xcf
            { 2, &CMOS65xx::CPY, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::CMP, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 2, &CMOS65xx::SKB, ADDRESSING_MODE_IMMEDIATE},
            { 8, &CMOS65xx::DCP, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::CPY, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::CMP, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::DEC, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::DCP, ADDRESSING_MODE_ZEROPAGE},
            { 2, &CMOS65xx::INY, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::CMP, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::DEX, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::SAX, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::CPY, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::CMP, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::DEC, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::DCP, ADDRESSING_MODE_ABSOLUTE},

            // 0xd0-0xdf
            { 2, &CMOS65xx::BNE, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::CMP, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::DCP, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::CMP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::DEC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::DCP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::CLD, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::CMP, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::DCP, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::SKW, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::CMP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::DEC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::DCP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0xe0-0xef
            { 2, &CMOS65xx::CPX, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::SBC, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 2, &CMOS65xx::SKB, ADDRESSING_MODE_IMMEDIATE},
            { 8, &CMOS65xx::ISC, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::CPX, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::SBC, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::INC, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::ISC, ADDRESSING_MODE_ZEROPAGE},
            { 2, &CMOS65xx::INX, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::SBC, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::SBC, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::CPX, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::SBC, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::INC, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::ISC, ADDRESSING_MODE_ABSOLUTE},

            // 0xf0-0xff
            { 2, &CMOS65xx::BEQ, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::SBC, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::ISC, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::SKB, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::SBC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::INC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ISC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::SED, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::SBC, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::ISC, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::SKW, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::SBC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::INC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ISC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X}
    };

    /**
     * registers
     */
    uint8_t _regA;
    uint8_t _regX;
    uint8_t _regY;
    uint16_t _regPC;
    uint8_t _regS;
    uint8_t _regP;

    // the emulated memory
    IMemory* _memory;

    uint16_t prevPc;
    void pushWord(uint16_t wd);
    void pushByte(uint8_t bt);
    uint16_t popWord();
    uint8_t popByte();
    CpuCallback _callback;

    // standard instructions
    void ADC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ADC_internal(int addressingMode, uint16_t operand);
    void AND(int opcodeByte, int addressingMode, int* cycles, int* size);
    void AND_internal(int addressingMode, uint16_t operand);
    void ASL(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ASL_internal(int addressingMode, uint16_t operand);
    void BCC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BCS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BEQ(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BIT(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BMI(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BNE(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BPL(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BRK(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BVC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void BVS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void CLC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void CLD(int opcodeByte, int addressingMode, int* cycles, int* size);
    void CLI(int opcodeByte, int addressingMode, int* cycles, int* size);
    void CLV(int opcodeByte, int addressingMode, int* cycles, int* size);
    void CMP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void CMP_internal(int addressingMode, uint16_t operand);
    void CPX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void CPY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void DEC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void DEC_internal(int addressingMode, uint16_t operand);
    void DEX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void DEY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void EOR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void EOR_internal(int addressingMode, uint16_t operand);
    void INC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void INC_internal(int addressingMode, uint16_t operand);
    void INX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void INY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void JMP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void JSR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LDA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LDA_internal(int addressingMode, uint16_t operand);
    void LDX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LDX_internal(int addressingMode, uint16_t operand);
    void LDY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LSR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LSR_internal(int addressingMode, uint16_t operand);
    void NOP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ORA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ORA_internal(int addressingMode, uint16_t operand);
    void PHA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void PHA_internal(int addressingMode, uint16_t operand);
    void PHP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void PLA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void PLA_internal(int addressingMode, uint16_t operand);
    void PLP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ROL(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ROL_internal(int addressingMode, uint16_t operand);
    void ROR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ROR_internal(int addressingMode, uint16_t operand);
    void RTI(int opcodeByte, int addressingMode, int* cycles, int* size);
    void RTS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void SBC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void SBC_internal(int addressingMode, uint16_t operand);
    void SEC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void SEC_internal(int addressingMode, uint16_t operand);
    void SED(int opcodeByte, int addressingMode, int* cycles, int* size);
    void SEI(int opcodeByte, int addressingMode, int* cycles, int* size);
    void STA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void STA_internal(int addressingMode, uint16_t operand);
    void STX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void STX_internal(int addressingMode, uint16_t operand);
    void STY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TAX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TAX_internal(int addressingMode, uint16_t operand);
    void TAY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TSX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TXA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TXA_internal(int addressingMode, uint16_t operand);
    void TXS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TXS_internal(int addressingMode, uint16_t operand);
    void TYA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TYA_internal(int addressingMode, uint16_t operand);

    // undocumented instructions
    void  AHX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ALR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ANC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ARR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ASO(int opcodeByte, int addressingMode, int *cycles, int *size);
    void  AXS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  DCP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ISC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  LAS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  LAX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  LSE(int opcodeByte, int addressingMode, int *cycles, int *size);
    void  OAL(int opcodeByte, int addressingMode, int *cycles, int *size);
    void  RLA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  RRA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SAX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SAY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SKB(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SKW(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  TAS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  XAA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  XAS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  KIL(int opcodeByte, int addressingMode, int* cycles, int* size);

    void cpuStatusToString(int addressingMode, char *status, int size);
    void logStateAfterInstruction(int addressingMode);
    void logExecution(const char *opcodeName,  uint8_t opcodeByte, uint16_t operand, int addressingMode);
    void parseInstruction(uint8_t opcodeByte, const char* functionName, int addressingMode, uint16_t* operand, int* size, int* cycles);
    void readOperand(int addressingMode, uint16_t addr, uint8_t* bt);
    void writeOperand(int addressingMode, uint16_t addr, uint8_t bt);
    void handlePageCrossingOnBranch(uint16_t operand, int *cycles);
    void handlePageCrossing(int addressingMode, uint16_t operand, int* cycles);
    void irqInternal();
    void dbgLoadFunctionalTest();

public:
    /**
     * run the cpu for an instruction
     * @return instruction cycles, or -1 to stop
     */
    int step();

    /**
     * resets the cpu and initializes for a new run
     * @return 0 on success, or errno
     */
    int reset();

    /**
     * constructor
     * @param mem IMemory implementation (emulated memory)
     * @param callback callback for clients
     */
    CMOS65xx(IMemory* mem, CpuCallback callback);

    /**
     * process an interrupt request
     */
    void irq();

    /**
     * process a nonmaskable interrupt request
     */
    void nmi();

    /**
     * return the memory interface
     * @return IMemory ptr
     */
    IMemory *memory();

};

#endif //CMOS65XX_H
