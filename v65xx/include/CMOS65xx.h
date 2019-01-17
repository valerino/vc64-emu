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

// only for debugging
#ifndef NDEBUG
#define DEBUG_LOG_EXECUTION
#endif

class CMOS65xx {
private:
    // the opcode matrix (taken from http://www.oxyron.de/html/opcodes02.html)
    // each opcode handler takds the addressing mode and number of cycles for that addressing mode in input,
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
            { 8, &CMOS65xx::SLO, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::ORA, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::ASL, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::SLO, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::PHP, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::ORA, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::ASL, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::ANC, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::ORA, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::ASL, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::SLO, ADDRESSING_MODE_ABSOLUTE},

            // 0x10-0x1f
            { 2, &CMOS65xx::BPL, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::ORA, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::SLO, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::ORA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ASL, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::SLO, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::CLC, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::ORA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::SLO, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::ORA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ASL, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::SLO, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

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
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::AND, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ROL, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::RLA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::SEC, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::AND, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::RLA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::AND, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ROL, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::RLA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0x40-0x4f
            { 6, &CMOS65xx::RTI, ADDRESSING_MODE_IMPLIED},
            { 6, &CMOS65xx::EOR, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::SRE, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::EOR, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::LSR, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::SRE, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::PHA, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::EOR, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::LSR, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::ALR, ADDRESSING_MODE_IMMEDIATE},
            { 3, &CMOS65xx::JMP, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::EOR, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::LSR, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::SRE, ADDRESSING_MODE_ABSOLUTE},

            // 0x50-0x5f
            { 2, &CMOS65xx::BVC, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::EOR, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::SRE, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::EOR, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::LSR, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::SRE, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::CLI, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::EOR, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::SRE, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::EOR, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::LSR, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::SRE, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0x60-0x6f
            { 6, &CMOS65xx::RTS, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::ADC, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::RRA, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::ADC, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::ROR, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::RRA, ADDRESSING_MODE_ZEROPAGE},
            { 4, &CMOS65xx::PLA, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::ADC, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::ROR, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::ARR, ADDRESSING_MODE_IMMEDIATE},
            { 5, &CMOS65xx::JMP, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::ADC, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::ROR, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::RRA, ADDRESSING_MODE_ABSOLUTE},

            // 0x70-0x7f
            { 2, &CMOS65xx::BVS, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::ADC, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::RRA, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::ADC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ROR, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::RRA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::SEI, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::ADC, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::RRA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::ADC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ROR, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::RRA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0x80-0x8f
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::STA, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::SAX, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::STY, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::STA, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::STX, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::SAX, ADDRESSING_MODE_ZEROPAGE},
            { 2, &CMOS65xx::DEY, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::TXA, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::XAA, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::STY, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::STA, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::STX, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::SAX, ADDRESSING_MODE_ABSOLUTE},

            // 0x90-0x9f
            { 2, &CMOS65xx::BCC, ADDRESSING_MODE_RELATIVE},
            { 6, &CMOS65xx::STA, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 6, &CMOS65xx::AHX, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::STY, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::STA, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::STX, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::SAX, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::TYA, ADDRESSING_MODE_IMPLIED},
            { 5, &CMOS65xx::STA, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::TXS, ADDRESSING_MODE_IMPLIED},
            { 5, &CMOS65xx::TAS, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 5, &CMOS65xx::SHY, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 5, &CMOS65xx::STA, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 5, &CMOS65xx::SHX, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
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
            { 2, &CMOS65xx::TAX, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::LAX, ADDRESSING_MODE_IMMEDIATE},
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
            { 4, &CMOS65xx::LDX, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
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
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMMEDIATE},
            { 8, &CMOS65xx::DCP, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::CPY, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::CMP, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::DEC, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::DCP, ADDRESSING_MODE_ZEROPAGE},
            { 2, &CMOS65xx::INY, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::CMP, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::DEX, ADDRESSING_MODE_ACCUMULATOR},
            { 2, &CMOS65xx::AXS, ADDRESSING_MODE_IMMEDIATE},
            { 4, &CMOS65xx::CPY, ADDRESSING_MODE_ABSOLUTE},
            { 4, &CMOS65xx::CMP, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::DEC, ADDRESSING_MODE_ABSOLUTE},
            { 6, &CMOS65xx::DCP, ADDRESSING_MODE_ABSOLUTE},

            // 0xd0-0xdf
            { 2, &CMOS65xx::BNE, ADDRESSING_MODE_RELATIVE},
            { 5, &CMOS65xx::CMP, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 1, &CMOS65xx::KIL, ADDRESSING_MODE_INVALID},
            { 8, &CMOS65xx::DCP, ADDRESSING_MODE_INDIRECT_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::CMP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::DEC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::DCP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::CLD, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::CMP, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::DCP, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::CMP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::DEC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::DCP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},

            // 0xe0-0xef
            { 2, &CMOS65xx::CPX, ADDRESSING_MODE_IMMEDIATE},
            { 6, &CMOS65xx::SBC, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMMEDIATE},
            { 8, &CMOS65xx::ISC, ADDRESSING_MODE_INDIRECT_INDEXED_X},
            { 3, &CMOS65xx::CPX, ADDRESSING_MODE_ZEROPAGE},
            { 3, &CMOS65xx::SBC, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::INC, ADDRESSING_MODE_ZEROPAGE},
            { 5, &CMOS65xx::ISC, ADDRESSING_MODE_ZEROPAGE},
            { 2, &CMOS65xx::INX, ADDRESSING_MODE_IMPLIED},
            { 2, &CMOS65xx::SBC, ADDRESSING_MODE_IMMEDIATE},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_ACCUMULATOR},
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
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 4, &CMOS65xx::SBC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::INC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 6, &CMOS65xx::ISC, ADDRESSING_MODE_ZEROPAGE_INDEXED_X},
            { 2, &CMOS65xx::SED, ADDRESSING_MODE_IMPLIED},
            { 4, &CMOS65xx::SBC, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 2, &CMOS65xx::NOP, ADDRESSING_MODE_IMPLIED},
            { 7, &CMOS65xx::ISC, ADDRESSING_MODE_ABSOLUTE_INDEXED_Y},
            { 4, &CMOS65xx::NOP, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 4, &CMOS65xx::SBC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::INC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X},
            { 7, &CMOS65xx::ISC, ADDRESSING_MODE_ABSOLUTE_INDEXED_X}
    };

    // accumulator
    uint8_t _regA;

    // addressing registers
    uint8_t _regX;
    uint8_t _regY;

    // program counter
    uint16_t _regPC;

    // stack pointer
    uint8_t _regS;

    // status register
    uint8_t _regP;

    IMemory* _memory;

    /**
     * push a word (16 bit) on the stack
     * @param w the word to be pushed
     */
    void pushWord(uint16_t w);

    /**
     * push a byte on the stack
     * @param b the byte to be pushed
     */
    void pushByte(uint8_t b);

    /**
     * pop a word (16 bit) from the stack
     * @return the word
     */
    uint16_t popWord();

    /**
     * pop a byte from the stack
     * @return the word
     */
    uint8_t popByte();

    // standard instructions
    void ADC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void AND(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ASL(int opcodeByte, int addressingMode, int* cycles, int* size);
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
    void CPX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void CPY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void DEC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void DEX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void DEY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void EOR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void INC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void INX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void INY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void JMP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void JSR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LDA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LDX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LDY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void LSR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void NOP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ORA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void PHA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void PHP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void PLA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void PLP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ROL(int opcodeByte, int addressingMode, int* cycles, int* size);
    void ROR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void RTI(int opcodeByte, int addressingMode, int* cycles, int* size);
    void RTS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void SBC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void SEC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void SED(int opcodeByte, int addressingMode, int* cycles, int* size);
    void SEI(int opcodeByte, int addressingMode, int* cycles, int* size);
    void STA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void STX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void STY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TAX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TAY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TSX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TXA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TXS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void TYA(int opcodeByte, int addressingMode, int* cycles, int* size);

    // undocumented instructions
    void  AHX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ANC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ALR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ARR(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  AXS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  DCP(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  ISC(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  LAX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  LAS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  RLA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  RRA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SAX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SHX(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SHY(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SLO(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  SRE(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  TAS(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  XAA(int opcodeByte, int addressingMode, int* cycles, int* size);
    void  KIL(int opcodeByte, int addressingMode, int* cycles, int* size);

    /**
     * only for debugging, print execution
     * @param opcodeName name of the opcode
     * @param opcodeByte the opcode
     * @param operand the operand if any
     * @param addressingMode one of the addressing modes
     */
    void logExecution(const char *opcodeName,  uint8_t opcodeByte, uint16_t operand, int addressingMode);

    /**
     * parse instruction depending on the addressing mode
     * @param addressingMode one of the addressing modes
     * @param operandAddress on return, address from where the operand is fetched (always = PC + 1)
     * @param operand on return, the operand if any
     * @param on return, the instruction size
     * @return false if there's no operand (implied/accumulator)
     */
    void parseInstruction(uint8_t opcodeByte, const char* functionName, int addressingMode, uint16_t* operandAddress, uint16_t* operand, int* size);

    /**
     * to be called post executing certain instructions, handle page crossing with carry flag
     * @param addressingMode one of the addressing modes
     * @param cycles on input, instruction cycles. on output, eventually cycles + 1
     */
    void postExecHandlePageCrossingWithCarry(int addressingMode, int *cycles);

    /**
     * to be called in branch instructions to increment cycle count on page crossing
     * @param operand the operand (the branch target)
     * @param cycles on input, instruction cycles. on output, cycles + 1 (branch taken) or eventually cycles + 2 (page crossing)
     */
    void handlePageCrossingOnBranch(uint16_t operand, int *cycles);

    /**
     * to be called post executing certain instructions, handle rewriting memory with operand
     * @param addressingMode one of the addressing modes
     * @param operandAddress the operand address
     * @param operand the operand
     */
    void postExecHandleMemoryOperand(int addressingMode, uint16_t operandAddress, uint16_t operand);

    /**
     * to be called post executing certain instructions, handles accumulator addressing (write operand back into accumulator)
     * @param addressingMode one of the addressing modes
     * @param operand the operand
     */
    void postExecHandleAccumulatorAddressing(int addressingMode, uint16_t operand);

public:
    /**
     * run the cpu for the specified number of cycles
     * @param cyclesToRun number of cycles to be run
     * @return number of remaining cycles in the last iteration (to be subtracted to the next iteration cycles)
     */
    int run(int cyclesToRun);

    /**
     * resets the cpu and initializes for a new run
     * @return 0 on success, or errno
     */
    int reset();

    /**
     * constructor
     * @param mem IMemory implementation (emulated memory)
     */
    CMOS65xx(IMemory* mem);
};

#endif //CMOS65XX_H
