//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CCIA2_H
#define VC64_EMU_CCIA2_H

#include "CCIA.h"

/**
 * registers
 * https://www.c64-wiki.com/wiki/CIA
 */
#define CIA2_REGISTERS_START 0xdd00
#define CIA2_REGISTERS_END   0xddff

#define CIA2_REG_DATA           0xdd00
#define CIA2_REG_DIRECTIONAL    0xdd02

/**
 * implements the 2nd CIA 6526, which controls the serial bus, rs232, vic memory and NMI
 */
class CCIA2: public CCIA {
public:
    CCIA2(CMOS65xx* cpu);
    virtual ~CCIA2();

    /**
     * run the cia2
     * @param current cycle count
     * @return number of cycles occupied
     */
    int run(int cycleCount);

    /**
     * get the bank connected to vic-ii by looking at the data register
     * https://www.c64-wiki.com/wiki/VIC_bank
     * @param address on return, the vic base address
     * @return bank 0-3
     */
    int getVicBank(uint16_t* address);

    /**
 * read from chip memory
 * @param address
 * @param bt
 */
    void read(uint16_t address, uint8_t* bt);

    /**
     * write to chip memory
     * @param address
     * @param bt
     */
    void write(uint16_t address, uint8_t bt);

private:

};

#endif //VC64_EMU_CCIA2_H
