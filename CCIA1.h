//
// Created by valerino on 19/01/19.
//
#pragma once
#include "CCIABase.h"

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_CIA1
#endif

#define CIA1_REGISTERS_START 0xdc00
#define CIA1_REGISTERS_END 0xdcff

/**
 * @brief implements the 1st CIA 6526, which controls keyboard, joystick,
 * paddles, datassette and IRQ
 */
class CCIA1 : public CCIABase {
    friend class CInput;

  public:
    CCIA1(CMOS65xx *cpu);
    void read(uint16_t address, uint8_t *bt);
    void write(uint16_t address, uint8_t bt);

    /**
     * set the correspondent key state in the emulated keyboard
     * @param scancode the c64 scancode
     * @param pressed
     */
    void setKeyState(uint8_t scancode, bool pressed);

    uint8_t readPRA();
    uint8_t readPRB();

    void writePRA(uint8_t pra);
    void writePRB(uint8_t prb);

  private:
    void readKeyboardMatrixColumn(uint8_t *bt, uint8_t pra);
    uint16_t checkShadowAddress(uint16_t address);

    bool _kbMatrix[0x40] = {false};
};
