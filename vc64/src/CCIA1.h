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

#define CIA1_REG_DATAPORT_B 0xdc01

#define CIA1_REG_DATAPORT_A 0xdc00

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

    /**
     * set the correspondent key state in the emulated keyboard
     * @param scancode the c64 scancode
     * @param pressed
     */
    void setKeyState(uint8_t scancode, bool pressed);

private:
    uint8_t _keyboard[0x40];
    void processDc01Read(uint8_t* bt);

};


#endif //VC64_EMU_CCIA1_H
