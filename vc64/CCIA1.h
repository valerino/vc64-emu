//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CCIA1_H
#define VC64_EMU_CCIA1_H

#include <CMOS65xx.h>
#include "CIACommon.h"

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_CIA1
#endif

/**
 * registers
 * https://www.c64-wiki.com/wiki/CIA
 */
#define CIA1_REGISTERS_START 0xdc00
#define CIA1_REGISTERS_END 0xdcff

/**
 * implements the 1st CIA 6526, which controls keyboard, joystick, paddles,
 * datassette and IRQ
 */
class CCIA1 {
    friend class CInput;

  public:
    CCIA1(CMOS65xx *cpu);
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
    void read(uint16_t address, uint8_t *bt);

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
    void readKeyboardMatrixColumn(uint8_t *bt, uint8_t pra);
    uint16_t checkShadowAddress(uint16_t address);

    bool _kbMatrix[0x40] = {false};

    uint16_t _timerALatch = 0;
    uint16_t _timerBLatch = 0;
    uint16_t _timerA = 0;
    uint16_t _timerB = 0;
    int _timerAMode = 0;
    int _timerBMode = 0;
    bool _timerARunning = false;
    bool _timerBRunning = false;
    bool _timerAIrqEnabled = false;
    bool _timerBIrqEnabled = false;
    bool _irqTriggeredA = false;
    bool _irqTriggeredB = false;
    int _prevCycleCount = 0;
    CMOS65xx *_cpu = nullptr;
};

#endif // VC64_EMU_CCIA1_H
