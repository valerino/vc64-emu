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

    /**
     * @brief get the bank connected to the VIC by looking at the data register
     * https://www.c64-wiki.com/wiki/VIC_bank
     * @param address on return, the vic base address
     * @return bank 0-3
     */
    int getVicBank(uint16_t *address);

  private:
};
