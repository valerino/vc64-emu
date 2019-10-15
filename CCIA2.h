//
// Created by valerino on 19/01/19.
//
#pragma once
#pragma once
#include "CCIABase.h"

#define CIA2_REGISTERS_START 0xdd00
#define CIA2_REGISTERS_END 0xdd0f

/**
 * @brief implements the 2nd CIA 6526, which controls serial bus, RS-232, VIC
 * memory, NMI
 */
class CCIA2 : public CCIABase {
  public:
    CCIA2(CMOS65xx *cpu, CPLA *pla);
    void read(uint16_t address, uint8_t *bt);
    void write(uint16_t address, uint8_t bt);
    int vicBank() { return _vicBank; }
    uint16_t vicMemoryAddress() { return _vicMemory; }

  private:
    int _vicBank = 0;
    uint16_t _vicMemory = 0;
    void setVicBank(uint8_t pra);
};
