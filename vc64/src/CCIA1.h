//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CCIA1_H
#define VC64_EMU_CCIA1_H

#include <CMOS65xx.h>

/**
 * registers
 * https://www.c64-wiki.com/wiki/CIA
 */
#define CIA1_REGISTERS_START 0xdc00
#define CIA1_REGISTERS_END   0xdcff

#define CIA1_REG_DATAPORT_A 0xdc00
#define CIA1_REG_DATAPORT_B 0xdc01
#define CIA1_REG_TIMER_A_LO 0xdc04
#define CIA1_REG_TIMER_A_HI 0xdc05
#define CIA1_REG_TIMER_B_LO 0xdc06
#define CIA1_REG_TIMER_B_HI 0xdc07
#define CIA1_REG_CONTROL_TIMER_A 0xdc0e
#define CIA1_REG_CONTROL_TIMER_B 0xdc0f

/**
 * implements the 1st CIA 6526, which controls keyboard, joystick, paddles, datassette and IRQ
 */
class CCIA1 {
    friend class CInput;

public:
    CCIA1(CMOS65xx* cpu);
    ~CCIA1();

    /**
     * update the internal state
     * @param current cycle count
     * @return additional cycles used
     */
    int update(int cycleCount);

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
    void processDc01Read(uint8_t* bt);
    uint16_t checkShadowAddress(uint16_t address);

    uint8_t _keyboard[0x40];

    uint16_t _timerA;
    uint16_t _timerB;
    bool _timerARunning;
    bool _timerBRunning;
    CMOS65xx* _cpu;
};


#endif //VC64_EMU_CCIA1_H
