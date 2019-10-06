//
// Created by valerino on 19/01/19.
//
#pragma once
#pragma once
#include "CCIABase.h"

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_CIA2
#endif

#define CIA2_REGISTERS_START 0xdd00
#define CIA2_REGISTERS_END 0xddff

/**
 * @brief implements the 2nd CIA 6526, which controls serial bus, RS-232, VIC
 * memory, NMI
 */
class CCIA2 : public CCIABase {
  public:
    CCIA2(CMOS65xx *cpu);
    void read(uint16_t address, uint8_t *bt);
    void write(uint16_t address, uint8_t bt);
    int vicBank() { return _vicBank;}
    uint16_t vicAddress() { return _vicAddress; }
  
  private:
    int _vicBank = 0;
    uint16_t _vicAddress = 0;
    void setVicBank(uint8_t pra);

};