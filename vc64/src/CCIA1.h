//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CCIA1_H
#define VC64_EMU_CCIA1_H

#include "CCIA.h"

/**
 * registers
 * https://www.c64-wiki.com/wiki/CIA
 */
#define CIA1_REGISTERS_START 0xdc00
#define CIA1_REGISTERS_END   0xdcff

/**
 * implements the 1st CIA 6526, which controls keyboard, joystick, paddles, datassette and IRQ
 */
class CCIA1: public CCIA {
public:
    CCIA1(CMOS65xx* cpu);
    virtual ~CCIA1();

    /**
     * run the cia1
     * @param current cycle count
     * @return number of cycles occupied
     */
    int run(int cycleCount);

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


#endif //VC64_EMU_CCIA1_H
