//
// Created by valerino on 19/01/19.
//
#pragma once
#include "CCIABase.h"

#define CIA1_REGISTERS_START 0xdc00
#define CIA1_REGISTERS_END 0xdcff

/**
 * @brief implements the 1st CIA 6526, which controls keyboard, joystick,
 * paddles, datassette and IRQ
 */
class CCIA1 : public CCIABase {
    friend class CInput;

  public:
    CCIA1(CMOS65xx *cpu, CPLA* pla);
    void read(uint16_t address, uint8_t *bt);
    void write(uint16_t address, uint8_t bt);

    /**
     * set the correspondent key state in the emulated keyboard
     * @param scancode the c64 scancode
     * @param pressed
     */
    void setKeyState(uint8_t scancode, bool pressed);

    /**
     * @brief enable reading joystick port 2 through keyboard, based on info
     * from https://www.c64-wiki.com/wiki/Joystick
     * @param enable enable/disable the hack
     */
    void enableJoy2Hack(bool enable);

  private:
    void readKeyboardMatrixColumn(uint8_t *bt, uint8_t pra);
    uint16_t checkShadowAddress(uint16_t address);
    bool _joy2Hack = false;
    bool _kbMatrix[0x40] = {false};
};
